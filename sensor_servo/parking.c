#include "parking.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sensor_servo_new.h" // Include real backend header

/* Include external variables and functions from sensor_servo_new.c */
extern int car_count;                          // Real-time car count from the backend
#define MAX_CARS 4                             // Maximum cars (matches backend)
#define SERVO_PWM_CHANNEL 1                    // PWM channel for the servo motor
#define SERVO_MIN_DUTY 5                       // Duty cycle to open the barrier
#define SERVO_MAX_DUTY 10                      // Duty cycle to close the barrier

/* Parking system state */
struct parking_state parking_state;

// /* Initialize parking state */
// void init_parking_state() {
//     parking_state.available_spots = MAX_CARS; // Sync available spots with backend MAX_CARS
//     for (int i = 0; i < MAX_CARS; i++) {
//         parking_state.pin_codes[i][0] = '\0'; // Clear all PINs
//     }
//     printf("Parking system initialized with %d available spots.\n", parking_state.available_spots);
// }

/* Initialize parking state */
void init_parking_state() {
    for (int i = 0; i < MAX_CARS; i++) {
        parking_state.pin_codes[i][0] = '\0'; // Clear all PINs
    }
    printf("Parking system initialized with a capacity of %d cars.\n", MAX_CARS);
}

/* Reserve a spot (general, not specific) */
int reserve_general_spot(const char *pin) {
    if (car_count < MAX_CARS) { // Check if spots are available based on the real car count
        for (int i = 0; i < MAX_CARS; i++) {
            if (parking_state.pin_codes[i][0] == '\0') { // Find an empty PIN slot
                strncpy(parking_state.pin_codes[i], pin, PIN_LENGTH);
                parking_state.pin_codes[i][PIN_LENGTH] = '\0';
                printf("Spot reserved with PIN: %s. Available spots: %d\n", pin, MAX_CARS - car_count - 1);
                return 1; // Success
            }
        }
    }
    printf("No available spots for reservation.\n");
    return 0; // Failure
}

/* Check if a general PIN is valid */
int check_general_pin(const char *entered_pin) {
    for (int i = 0; i < MAX_CARS; i++) {
        if (strcmp(parking_state.pin_codes[i], entered_pin) == 0) {
            parking_state.pin_codes[i][0] = '\0'; // Clear the PIN upon successful entry
            printf("Valid PIN entered: %s. Opening barrier.\n", entered_pin);

            // Open the barrier using real backend logic
            open_barrier();

            return 1; // Valid PIN
        }
    }
    printf("Invalid PIN entered: %s.\n", entered_pin);
    return 0; // Invalid PIN
}

/* Free parking without reservation */
int free_parking_entry() {
    if (car_count < MAX_CARS) { // Check if there are spots available
        printf("Free parking entry allowed. Available spots: %d\n", MAX_CARS - car_count - 1);

        // Open the barrier using real backend logic
        open_barrier();


        return 1; // Entry successful
    } else {
        printf("Free parking entry denied. No available spots.\n");
        return 0; // Entry denied
    }
}

/* Generate a random 3-digit PIN */
void generate_pin(char *pin) {
    snprintf(pin, PIN_LENGTH + 1, "%03d", rand() % 1000);
}
