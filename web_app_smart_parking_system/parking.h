#ifndef PARKING_H
#define PARKING_H

#define MAX_SPOTS 20      // Total number of parking spots
#define PIN_LENGTH 3      // Length of the reservation PIN

#define OPEN 1            // Barrier open state
#define CLOSE 0           // Barrier closed state

/* Spot status */
enum spot_status {
    AVAILABLE,
    OCCUPIED
};

/* Parking system state */
struct parking_state {
    int available_spots;                   // Total number of available spots
    char pin_codes[MAX_SPOTS][PIN_LENGTH + 1]; // General PINs for reservations
};

/* Parking system state object */
extern struct parking_state parking_state;

/* Initialize parking state */
void init_parking_state();

/* Reserve a parking spot (general, not specific) */
int reserve_general_spot(const char *pin);

/* Check if a general PIN is valid */
int check_general_pin(const char *entered_pin);

/* Control the barrier (1 = open, 0 = close) */
void control_barrier(int open);

/* Generate a random 3-digit PIN */
void generate_pin(char *pin);

#endif
