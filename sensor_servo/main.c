// FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>

// Input/Output
#include <stdio.h>

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
    /* Initialize UART
     * Ports: 16+7 (TX+RX)
     * Baudrate: 2 million
     */
    bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);

    /* (Re)define Heap */
    vPortDefineHeapRegions(xHeapRegions);

    /* Set up the Sensor and Servo task */
    static StackType_t sensor_servo_stack[SENSOR_SERVO_STACK_SIZE];
    static StaticTask_t sensor_servo_task;

    /* Create the task */
    xTaskCreateStatic(
        task_sensor_servo,        // Function implementing the task
        (char*)"sensor_servo",    // Human-readable name
        SENSOR_SERVO_STACK_SIZE,  // Stack size
        NULL,                     // Parameters for the function
        15,                       // Task priority
        sensor_servo_stack,       // Stack to use for the task
        &sensor_servo_task        // Task handle
    );

    /* Start the scheduler */
    vTaskStartScheduler();
}