// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Input/Output
#include <stdio.h>

#include <bl_dma.h>
#include <bl_gpio.h>
#include <bl_irq.h>
#include <bl_sec.h>
#include <bl_sys_time.h>
#include <hal_boot2.h>
#include <hal_board.h>
#include <hal_hwtimer.h>

#include <blog.h>
#include <lwip/tcpip.h>

// UART library
#include <bl_uart.h>

/* Define heap regions */
extern uint8_t _heap_start;
extern uint8_t _heap_size;
extern uint8_t _heap_wifi_start;
extern uint8_t _heap_wifi_size;

static HeapRegion_t xHeapRegions[] = {
    { &_heap_start, (unsigned int) &_heap_size },
    { &_heap_wifi_start, (unsigned int) &_heap_wifi_size },
    { NULL, 0 },
    { NULL, 0 }
};

/* Size of the stack for the task */
#define SENSOR_SERVO_STACK_SIZE 512

extern void task_sensor_servo(void *pvParameters);

void bfl_main(void) {

    /* Define information containers for tasks */
    static StackType_t wifi_stack[1024];
    static StaticTask_t wifi_task;

    static StackType_t httpd_stack[512];
    static StaticTask_t httpd_task;
    /* Initialize UART
     * Ports: 16+7 (TX+RX)
     * Baudrate: 2 million
     */
    bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);

    /* (Re)define Heap */
    vPortDefineHeapRegions(xHeapRegions);

    blog_init();
    bl_irq_init();
    bl_sec_init();
    bl_dma_init();
    hal_boot2_init();
    hal_board_cfg(0);

    /* Set up the Sensor and Servo task */
    static StackType_t sensor_servo_stack[SENSOR_SERVO_STACK_SIZE];
    static StaticTask_t sensor_servo_task;

    /* Create the task */
    extern void task_httpd(void *pvParameters);
    printf("[SYSTEM] Starting httpd task\r\n");
    xTaskCreateStatic(task_httpd, (char *)"httpd", 512, NULL, 15, httpd_stack, &httpd_task);

    printf("[SYSTEM] Starting WiFi task\r\n");
    extern void task_wifi(void *pvParameters);
    xTaskCreateStatic(task_wifi, (char *)"wifi", 1024, NULL, 20, wifi_stack, &wifi_task);

    xTaskCreateStatic(
        task_sensor_servo,        // Function implementing the task
        (char*)"sensor_servo",    // Human-readable name
        SENSOR_SERVO_STACK_SIZE,  // Stack size
        NULL,                     // Parameters for the function
        15,                       // Task priority
        sensor_servo_stack,       // Stack to use for the task
        &sensor_servo_task        // Task handle
    );

    /* Start TCP/IP stack */
    printf("[SYSTEM] Starting TCP/IP stack\r\n");
    tcpip_init(NULL, NULL);

    /* Start the scheduler */
    vTaskStartScheduler();
}
