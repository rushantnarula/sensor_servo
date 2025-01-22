// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>

// Input/output
#include <stdio.h>
#include <bl_gpio.h>    // For GPIO handling
#include <bl_pwm.h>     // For PWM handling
#include <bl_timer.h>   // For precise microsecond timing

#include <stdio.h>

#include <FreeRTOS.h>
#include <task.h>
#include <string.h>

// Define entry sensor pins
#define TRIG_PIN_ENTRY 4             // GPIO pin for the entry sensor trigger
#define ECHO_PIN_ENTRY 5             // GPIO pin for the entry sensor echo

// Define exit sensor pins
#define TRIG_PIN_EXIT 12              // GPIO pin for the exit sensor trigger
#define ECHO_PIN_EXIT 14              // GPIO pin for the exit sensor echo

// Define the GPIO pin and PWM channel for the servo motor
#define SERVO_PIN 11                 // GPIO pin connected to the servo signal wire
#define SERVO_PWM_CHANNEL 1          // PWM channel for the servo

// Define the base PWM frequency and clock divider for 50Hz effective frequency
#define BASE_PWM_FREQUENCY 6400      // Base PWM frequency in Hz
#define PWM_CLOCK_DIVIDER 128        // Divider to achieve 50Hz (6400 / 128 = 50)

// Servo control parameters (duty cycle values)
#define SERVO_MIN_DUTY 5             // 5% duty cycle for 0° (1ms pulse)
#define SERVO_MAX_DUTY 10            // 10% duty cycle for 180° (2ms pulse)

// Maximum number of cars allowed
#define MAX_CARS 4

// Global counter for cars
int car_count = 0;

// Function to measure distance using an ultrasonic sensor
float measure_distance(uint8_t trig_pin, uint8_t echo_pin) {
    uint8_t value;
    uint32_t start_time, echo_start, echo_end;

    // Send a 10µs pulse on the TRIG_PIN
    bl_gpio_output_set(trig_pin, 1);
    bl_timer_delay_us(10); // Delay for 10 microseconds
    bl_gpio_output_set(trig_pin, 0);

    // Wait for the ECHO_PIN to go high (Start of Echo)
    start_time = bl_timer_now_us();
    while (1) {
        bl_gpio_input_get(echo_pin, &value);
        if (value == 1) {
            break;
        }
        if ((bl_timer_now_us() - start_time) > 30000) { // Timeout after 30ms
            return -1; // No valid distance
        }
    }

    // Record the start time of the echo
    echo_start = bl_timer_now_us();

    // Wait for the ECHO_PIN to go low (End of Echo)
    while (1) {
        bl_gpio_input_get(echo_pin, &value);
        if (value == 0) {
            break;
        }
    }

    // Record the end time of the echo
    echo_end = bl_timer_now_us();

    // Calculate the duration of the echo pulse in microseconds
    uint32_t duration = echo_end - echo_start;

    // Convert duration to distance in cm
    float distance = (duration * 0.0343) / 2;

    // Ignore invalid distances
    if (distance < 2.0 || distance > 400.0) { // Sensor range is 2cm to 400cm
        return -1; // Invalid distance
    }

    return distance;
}

void initialize_servo() {
    bl_pwm_init(SERVO_PWM_CHANNEL, SERVO_PIN, BASE_PWM_FREQUENCY);
    PWM_Channel_Set_Div(SERVO_PWM_CHANNEL, PWM_CLOCK_DIVIDER);
    bl_pwm_start(SERVO_PWM_CHANNEL);
    printf("Servo motor initialized.\r\n");
}


void open_barrier() {
    printf("Opening barrier...\r\n");
    bl_pwm_set_duty(SERVO_PWM_CHANNEL, SERVO_MIN_DUTY); 
    vTaskDelay(3000 / portTICK_PERIOD_MS);            
    printf("Closing barrier...\r\n");
    bl_pwm_set_duty(SERVO_PWM_CHANNEL, SERVO_MAX_DUTY); 
}

// Function to initialize GPIO pins for sensors
void initialize_sensors() {
    // Initialize entry sensor pins
    bl_gpio_enable_output(TRIG_PIN_ENTRY, 0, 0); // Set TRIG_PIN_ENTRY as output
    bl_gpio_enable_input(ECHO_PIN_ENTRY, 1, 0);  // Set ECHO_PIN_ENTRY as input with pull-up

    // Initialize exit sensor pins
    bl_gpio_enable_output(TRIG_PIN_EXIT, 0, 0);  // Set TRIG_PIN_EXIT as output
    bl_gpio_enable_input(ECHO_PIN_EXIT, 1, 0);   // Set ECHO_PIN_EXIT as input with pull-up

    printf("Sensors initialized.\r\n");
}

// Function to handle car entry logic
void handle_car_entry() {
    float distance_entry = measure_distance(TRIG_PIN_ENTRY, ECHO_PIN_ENTRY);
    if (distance_entry > 0 && distance_entry < 20) {
        car_count++; // Increment car count
        printf("Car entered. Current count: %d\r\n", car_count);
        open_barrier();
    }
}

// Function to handle car exit logic
void handle_car_exit() {
    float distance_exit = measure_distance(TRIG_PIN_EXIT, ECHO_PIN_EXIT);
    if (distance_exit > 0 && distance_exit < 20) {
        if (car_count > 0) {
            car_count--; // Decrement car count
            printf("Car exited. Current count: %d\r\n", car_count);
        } else {
            printf("Error: Exit detected but no cars inside.\r\n");
        }
    }
}

// Main sensor and servo handling function
void sensor_servo_logic() {
    if (car_count >= MAX_CARS) {
        printf("Parking Full! No more cars allowed.\r\n");
    } else {
        handle_car_entry();
    }
    handle_car_exit();
}



// Task to handle entry and exit sensors and barrier logic
void task_sensor_servo(void *pvParameters) {

    initialize_sensors();
    // Initialize the PWM for the servo motor at the base frequency
    initialize_servo();

    while (1) {
        sensor_servo_logic();
        // Wait for 3 seconds before the next measurement
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL); // Should never happen
}

