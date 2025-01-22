#include "lwip/apps/httpd.h"
#include "lwip/opt.h"
#include "lwip/apps/fs.h"
#include "lwip/def.h"
#include "lwip/mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bl_gpio.h>
#include <bl_pwm.h>
#include <stdbool.h>
#include "cJSON.h"
#include "cgi.h"
#include "parking.h"
#include "sensor_servo_new.h" // Include real backend header

static char last_message[256] = {0};  // Holds the latest message for display

typedef struct {
    char pin[PIN_LENGTH + 1];
    bool is_valid;
} Reservation;

#define MAX_RESERVATIONS 10
Reservation reservations[MAX_RESERVATIONS] = {0}; // Array to store reservations

static int add_reservation(const char *new_pin) {
    for (int i = 0; i < MAX_RESERVATIONS; i++) {
        if (!reservations[i].is_valid) { // Find an empty slot
            strncpy(reservations[i].pin, new_pin, PIN_LENGTH);
            reservations[i].pin[PIN_LENGTH] = '\0';
            reservations[i].is_valid = true;
            return 1; // Successfully added
        }
    }
    return 0; // No space left for new reservations
}


static int validate_and_remove_reservation(const char *entered_pin) {
    for (int i = 0; i < MAX_RESERVATIONS; i++) {
        if (reservations[i].is_valid && strcmp(reservations[i].pin, entered_pin) == 0) {
            reservations[i].is_valid = false; // Mark the reservation as used
            return 1; // PIN is valid
        }
    }
    return 0; // PIN is invalid
}


/* Check availability handler */
static const char *cgi_handler_check_availability(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {

    // 1. Build JSON
    cJSON *json_response = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_response, "available_spots", MAX_CARS - car_count);

    // 2. Convert JSON to a string
    char *json_str = cJSON_PrintUnformatted(json_response);
    cJSON_Delete(json_response);

    // 3. Store it into last_message
    if (json_str) {
        snprintf(last_message, sizeof(last_message), 
                 "Available spots: %s", json_str);
        free(json_str);
    } else {
        snprintf(last_message, sizeof(last_message),
                 "Error generating availability response!");
    }

    return PARKING_UI_ENDPOINT;  
}

char pin[PIN_LENGTH + 1] = {0};


/* Reserve spot handler */
static const char *cgi_handler_reserve(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    //char pin[PIN_LENGTH + 1] = {0};
    if (car_count < MAX_CARS) {
    generate_pin(pin);  // Generate a random PIN
    add_reservation(pin);
    car_count++;
    }
    else{
    return "/reserve_spot.html";    
    }

    return "/reserve_spot.html";

    // Return the dynamically generated response
    //return response;
}

/* Enter parking handler */
static const char *cgi_handler_enter(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    printf("[DEBUG] Enter parking handler called.\r\n");
    char entered_pin[PIN_LENGTH + 1] = {0};
    int has_reservation = 0;

    // Parse query params
    for (int i = 0; i < iNumParams; i++) {
        if (!strcmp(pcParam[i], "pin")) {
            strncpy(entered_pin, pcValue[i], PIN_LENGTH);
            entered_pin[PIN_LENGTH] = '\0';
        }
        if (!strcmp(pcParam[i], "reserved")) {
            has_reservation = atoi(pcValue[i]);
        }
    }

    // If the user has no reservation, check availability
    if (!has_reservation) {
        if (car_count < MAX_CARS) {
        
            car_count++;  // Decrease available spots
            snprintf(last_message, sizeof(last_message), "Spot available. Welcome!");
            open_barrier();
        } else {
            snprintf(last_message, sizeof(last_message), "No spots available. Please try later.");
        }
    }
    else {  // User has a reservation
        if (validate_and_remove_reservation(entered_pin)) {
            snprintf(last_message, sizeof(last_message), "Valid PIN. Welcome!");
            open_barrier();
        } else {
            snprintf(last_message, sizeof(last_message), "Invalid PIN. Please try again.");
        }
    }

    return "/screen.html";
}

/* CGI Handlers Array */
static const tCGI cgi_handlers[] = {
    {"/check_availability", cgi_handler_check_availability},
    {"/reserve_spot", cgi_handler_reserve},
    {"/enter_parking", cgi_handler_enter}
};



int fs_open_custom(struct fs_file *file, const char *name) {
    printf("[DEBUG] fs_open_custom called for: %s\r\n", name);

    const char *response = NULL;
    char *html_page = NULL;

    // Serve main.html (static reservation query page)
    if (!strcmp(name, "/main.html")) {
        html_page = (char *)calloc(2048, 1);
        if (!html_page) {
            return 0;
        }

        snprintf(html_page, 2048,
            "<!DOCTYPE html>"
            "<html>"
            "<head><title>Smart Parking System</title></head>"
            "<body>"
            "<h1>Welcome to the Smart Parking System</h1>"
            "<p>Available Spots: %d</p>"
            "<form action=\"%s\" method=\"GET\">"
            "<button type='submit' %s>Reserve a Spot</button>"
            "</form>"
            "<script>"
            "setInterval(() => { location.reload(); }, 5000);"
            "</script>"
            "</body></html>",
            MAX_CARS - car_count,
            RESERVE_SPOT_ENDPOINT,
            (car_count >= MAX_CARS) ? "disabled" : ""
        );

        response = html_page;
    } 
    else if (!strcmp(name, "/reserve_spot.html")) {
        /*
        const char *dynamic_response = cgi_handler_reserve(0, 0, NULL, NULL);
        if (!dynamic_response) {
        return 0; // Handle error gracefully
        }
        */
            char *response = (char *)calloc(512, 1);

    if (!response) {
        return 0; //<html><body><h1>Internal Server Error</h1></body></html>";
    }


    if (car_count <= MAX_CARS) {  // Check real backend availability
       snprintf(response, 512,
                 "<html><body>"
                 "<h1>Reservation Successful</h1>"
                 "<p>Your PIN is: %s</p>"
                 "<a href='/main.html'>Back to Main Page</a>"
                 "</body></html>", pin);
    } else {
        snprintf(response, 512,
                 "<html><body>"
                 "<h1>No Spots Available</h1>"
                 "<a href='/main.html'>Back to Main Page</a>"
                 "</body></html>");
    }

     int response_size = strlen(response);
    file->pextension = mem_malloc(response_size);
    if (!file->pextension) {
        return 0; // Memory allocation failure
    }

    memcpy(file->pextension, response, response_size);
    file->data = (const char *)file->pextension;
    file->len = response_size;
    file->index = response_size;
    file->flags = FS_FILE_FLAGS_HEADER_PERSISTENT;

    return 1;  // Indicate success
    } 
    // Serve enter.html (PIN entry form with messages)
    else if (!strcmp(name, "/screen.html")) {
        html_page = (char *)calloc(2048, 1);
        if (!html_page) {
            return 0;
        }

        snprintf(html_page, 2048,
            "<!DOCTYPE html>"
            "<html>"
            "<head><title>Parking Screen</title></head>"
            "<body>"
            "<h1>Parking Screen</h1>"
            "<p>Available Spots: %d</p>"
            "<button onclick='handleCarDetection(true)'>Yes</button>"
            "<button onclick='handleCarDetection(false)'>No</button>"
            "<script>"
            "let userInteracted = false;"
            "function handleCarDetection(reserved) {"
            "let userInteracted = false;"
            "  if (reserved) {"
            "    const pin = prompt('Enter Reservation PIN:');"
            "    window.location.href = `/enter_parking?reserved=1&pin=${pin}`;"
            "  } else {"
            "    window.location.href = `/enter_parking?reserved=0`;"
            "  }"
            "}"
            "</script>"
            "</body></html>",
            MAX_CARS - car_count
        );

        response = html_page;
    } 
    else {
        return 0; // Unknown path
    }

    // Handle dynamic response
    if (!response) {
        if (html_page) {
            free(html_page);
        }
        return 0;
    }

    int response_size = strlen(response);

    memset(file, 0, sizeof(struct fs_file));
    file->pextension = mem_malloc(response_size);
    if (!file->pextension) {
        if (html_page) {
            free(html_page);
        }
        return 0;
    }

    memcpy(file->pextension, response, response_size);
    file->data = (const char *)file->pextension;
    file->len = response_size;
    file->index = response_size;
    file->flags = FS_FILE_FLAGS_HEADER_PERSISTENT;

    if (html_page) {
        free(html_page);
    }

    return 1;
}

void fs_close_custom(struct fs_file *file) {
    if (file && file->pextension) {
        mem_free(file->pextension);
        file->pextension = NULL;
    }
}

int fs_read_custom(struct fs_file *file, char *buffer, int count) {
    LWIP_UNUSED_ARG(file);
    LWIP_UNUSED_ARG(buffer);
    LWIP_UNUSED_ARG(count);
    return FS_READ_EOF;
}

void custom_files_init(void) {
    printf("Initializing custom files for dynamic content.\r\n");
}

void cgi_init(void) {
    printf("Initializing module for CGI\r\n");
    http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));
}
