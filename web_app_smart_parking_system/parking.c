#include "parking.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Parking system state */
struct parking_state parking_state;

/* Initialize parking state */
void init_parking_state() {
    parking_state.available_spots = MAX_SPOTS; // Initialize total available spots
    for (int i = 0; i < MAX_SPOTS; i++) {
        parking_state.pin_codes[i][0] = '\0'; // Clear all PINs
    }
    printf("Parking system initialized with %d available spots.\n", MAX_SPOTS);
}

/* Reserve a spot (general, not specific) */
int reserve_general_spot(const char *pin) {
    if (parking_state.available_spots > 0) {
        for (int i = 0; i < MAX_SPOTS; i++) {
            if (parking_state.pin_codes[i][0] == '\0') { // Find an unassigned PIN slot
                strncpy(parking_state.pin_codes[i], pin, PIN_LENGTH);
                parking_state.pin_codes[i][PIN_LENGTH] = '\0';
                parking_state.available_spots--;
                printf("Spot reserved with PIN: %s. Available spots: %d\n", pin, parking_state.available_spots);
                return 1; // Success
            }
        }
    }
    printf("No available spots for reservation.\n");
    return 0; // Failure
}

/* Check if a general PIN is valid */
int check_general_pin(const char *entered_pin) {
    for (int i = 0; i < MAX_SPOTS; i++) {
        if (strcmp(parking_state.pin_codes[i], entered_pin) == 0) {
            parking_state.pin_codes[i][0] = '\0'; // Clear the PIN upon successful entry
            printf("Valid PIN entered: %s. Barrier opened.\n", entered_pin);
            return 1; // Valid PIN
        }
    }
    printf("Invalid PIN entered: %s.\n", entered_pin);
    return 0; // Invalid PIN
}


/* Free parking without reservation */
int free_parking_entry() {
    if (parking_state.available_spots > 0) {
        parking_state.available_spots--;
        printf("Free parking entry allowed. Available spots: %d\n", parking_state.available_spots);
        control_barrier(OPEN);
        return 1; // Entry successful
    } else {
        printf("Free parking entry denied. No available spots.\n");
        control_barrier(CLOSE);
        return 0; // Entry denied
    }
}


/* Control the barrier */
void control_barrier(int open) {
    if (open) {
        printf("Barrier opened.\n");
        // Simulate barrier opening logic here (e.g., GPIO control)
    } else {
        printf("Barrier closed.\n");
        // Simulate barrier closing logic here (e.g., GPIO control)
    }
}

/* Generate a random 3-digit PIN */
void generate_pin(char *pin) {
    snprintf(pin, PIN_LENGTH + 1, "%03d", rand() % 1000);
}
