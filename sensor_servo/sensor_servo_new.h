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

// Function to measure distance using an ultrasonic sensor
float measure_distance(uint8_t trig_pin, uint8_t echo_pin);

void initialize_servo();


void open_barrier();

// Function to initialize GPIO pins for sensors
void initialize_sensors();

// Function to handle car entry logic
void handle_car_entry();

// Function to handle car exit logic
void handle_car_exit();

// Main sensor and servo handling function
void sensor_servo_logic();



// Task to handle entry and exit sensors and barrier logic
void task_sensor_servo(void *pvParameters);
