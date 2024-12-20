// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>

// Input/output
#include <stdio.h>
#include <bl_gpio.h>    // For GPIO handling
#include <bl_pwm.h>     // For PWM handling
#include <bl_timer.h>   // For precise microsecond timing

// Define ultrasonic sensor pins
#define TRIG_PIN 4                    // GPIO pin for the ultrasonic trigger
#define ECHO_PIN 5                    // GPIO pin for the ultrasonic echo

// Define the GPIO pin and PWM channel for the servo motor
#define SERVO_PIN 11                  // GPIO pin connected to the servo signal wire
#define SERVO_PWM_CHANNEL 1           // PWM channel for the servo

// Define the base PWM frequency and clock divider for 50Hz effective frequency
#define BASE_PWM_FREQUENCY 6400       // Base PWM frequency in Hz
#define PWM_CLOCK_DIVIDER 128         // Divider to achieve 50Hz (6400 / 128 = 50)

// Servo control parameters (duty cycle values)
#define SERVO_MIN_DUTY 5              // 5% duty cycle for 0° (1ms pulse)
#define SERVO_MAX_DUTY 10             // 10% duty cycle for 180° (2ms pulse)

// Function to measure distance using the ultrasonic sensor
float measure_distance(void) {
    uint8_t value;
    uint32_t start_time, echo_start, echo_end;

    // Send a 10µs pulse on TRIG_PIN
    bl_gpio_output_set(TRIG_PIN, 1);
    bl_timer_delay_us(10); // Delay for 10 microseconds
    bl_gpio_output_set(TRIG_PIN, 0);

    // Wait for the ECHO_PIN to go high (Start of Echo)
    start_time = bl_timer_now_us();
    while (1) {
        bl_gpio_input_get(ECHO_PIN, &value);
        if (value == 1) {
            break;
        }
        if ((bl_timer_now_us() - start_time) > 30000) { // Timeout after 30ms
            printf("Timeout waiting for ECHO_PIN to go high\r\n");
            return -1; // No valid distance
        }
    }

    // Record the start time of the echo
    echo_start = bl_timer_now_us();

    // Wait for the ECHO_PIN to go low (End of Echo)
    while (1) {
        bl_gpio_input_get(ECHO_PIN, &value);
        if (value == 0) {
            break;
        }
    }

    // Record the end time of the echo
    echo_end = bl_timer_now_us();

    // Calculate the duration of the echo pulse in microseconds
    uint32_t duration = echo_end - echo_start;

    // Convert duration to distance in cm
    // Speed of sound: 343 m/s or 0.0343 cm/µs
    // Distance = (duration / 2) * Speed of sound
    float distance = (duration * 0.0343) / 2;

    // Ignore invalid distances
    if (distance < 2.0 || distance > 400.0) { // Sensor range is 2cm to 400cm
        return -1; // Invalid distance
    }

    return distance;
}

// Task to handle ultrasonic sensor and servo motor logic
void task_sensor_servo(void *pvParameters) {
    printf("Sensor and Servo task started\r\n");

    // Initialize ultrasonic sensor pins
    bl_gpio_enable_output(TRIG_PIN, 0, 0); // Set TRIG_PIN as output
    bl_gpio_enable_input(ECHO_PIN, 1, 0);  // Set ECHO_PIN as input with pull-up

    // Initialize the PWM for the servo motor at the base frequency
    bl_pwm_init(SERVO_PWM_CHANNEL, SERVO_PIN, BASE_PWM_FREQUENCY);

    // Set the clock divider to achieve 50Hz frequency
    PWM_Channel_Set_Div(SERVO_PWM_CHANNEL, PWM_CLOCK_DIVIDER);

    // Start PWM operations
    bl_pwm_start(SERVO_PWM_CHANNEL);

    while (1) {
        // Measure the distance
        float distance = measure_distance();

        // Handle distance logic
        if (distance > 0 && distance < 20) {
            // Object detected within 20 cm: Open the barrier
            printf("Object detected at distance: %.2f cm\r\n", distance);
            printf("Opening the barrier...\r\n");
            bl_pwm_set_duty(SERVO_PWM_CHANNEL, SERVO_MIN_DUTY); // Move to 180°
            vTaskDelay(3000 / portTICK_PERIOD_MS);              // Keep barrier open for 3 seconds
        } else {
            // No object detected: Keep the barrier closed
            printf("No object detected or out of range\r\n");
            printf("Closing the barrier...\r\n");
            bl_pwm_set_duty(SERVO_PWM_CHANNEL, SERVO_MAX_DUTY); // Move to 0°
        }

        // Wait for 3 seconds before the next measurement
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL); // Should never happen
}