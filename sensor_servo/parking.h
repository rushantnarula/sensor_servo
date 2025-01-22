#ifndef PARKING_H
#define PARKING_H

#include <stdint.h> // For standard integer types

#define PIN_LENGTH 3      // Length of the reservation PIN

/* Extern declarations for shared variables */
extern int car_count;     // Global car count from sensor_servo_new.c
#define MAX_CARS 4  // Maximum number of cars allowed (defined in sensor_servo_new.c)

// #define MAX_CARS 4        // use this for removing MAX_CARS ERROR

/* Parking system state */
struct parking_state {
    char pin_codes[MAX_CARS][PIN_LENGTH + 1]; // General PINs for reservations
};

/* Parking system state object */
extern struct parking_state parking_state;

/* Initialize parking state */
void init_parking_state();

/* Reserve a parking spot (general, not specific) */
int reserve_general_spot(const char *pin);

/* Check if a general PIN is valid */
int check_general_pin(const char *entered_pin);

/* Free parking without reservation */
int free_parking_entry();

/* Generate a random 3-digit PIN */
void generate_pin(char *pin);

#endif /* PARKING_H */
