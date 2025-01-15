#include "lwip/apps/httpd.h"
#include "lwip/opt.h"

#include "lwip/apps/fs.h"
#include "lwip/def.h"
#include "lwip/mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bl_gpio.h>

#include "cJSON.h"

#include "cgi.h"

#include "parking.h"
#include <time.h>
// #include <cgi.i>

static char last_message[256] = {0};  // Holds the latest message for display

static const char *cgi_handler_check_availability(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    printf("[DEBUG] Check availability handler called.\r\n");

    // 1. Build JSON
    cJSON *json_response = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_response, "available_spots", parking_state.available_spots);

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

    // 4. Return the parking page
    return PARKING_UI_ENDPOINT;  
}

static const char *cgi_handler_reserve(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    printf("[DEBUG] Reserve spot handler called.\r\n");
    char pin[PIN_LENGTH + 1] = {0};

    if (parking_state.available_spots > 0) {
        generate_pin(pin);
        if (reserve_general_spot(pin)) {
            printf("Spot reserved with PIN: %s\r\n", pin);

            // Store success message
            snprintf(last_message, sizeof(last_message),
                     "Reservation successful! Your PIN is %s", pin);
        } else {
            snprintf(last_message, sizeof(last_message),
                     "Error reserving a spot.");
        }
    } else {
        snprintf(last_message, sizeof(last_message),
                 "No spots available for reservation.");
    }

    // Return the main UI
    return PARKING_UI_ENDPOINT;  
}

static const char *cgi_handler_enter(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    printf("[DEBUG] Enter parking handler called.\r\n");
    char entered_pin[PIN_LENGTH + 1] = {0};

    // Parse query params
    for (int i = 0; i < iNumParams; i++) {
        if (!strcmp(pcParam[i], "pin")) {
            strncpy(entered_pin, pcValue[i], PIN_LENGTH);
            entered_pin[PIN_LENGTH] = '\0';
        }
    }

    // Check if the PIN matches
    if (check_general_pin(entered_pin)) {
        control_barrier(OPEN);
        snprintf(last_message, sizeof(last_message),
                 "Barrier opened. Welcome!");
    } else {
        snprintf(last_message, sizeof(last_message),
                 "Invalid PIN.");
    }

    // Return the main UI
    return PARKING_UI_ENDPOINT; 
}

static const tCGI cgi_handlers[] = {
    {"/check_availability", cgi_handler_check_availability},
    {"/reserve_spot", cgi_handler_reserve},
    {"/enter_parking", cgi_handler_enter}
};


int fs_open_custom(struct fs_file *file, const char *name) {
    printf("[DEBUG] fs_open_custom called for: %s\\r\\n", name);

    const char *response = NULL;
    char *html_page = NULL;

    // Page-specific message variable
    static char enter_message[256] = {0};

    // Serve main.html (static reservation query page)
    if (!strcmp(name, "/main.html")) {
        html_page = (char *)calloc(2048, 1);
        if (!html_page) {
            return 0;
        }

        strcat(html_page,
            "<!DOCTYPE html>"
            "<html>"
            "<head><title>Welcome to Smart Parking</title>"
            "<style>"
            "body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 0; display: flex; flex-direction: column; justify-content: center; height: 100vh; }"
            "button { margin: 10px; padding: 10px 20px; font-size: 16px; }"
            "</style>"
            "</head>"
            "<body>"
            "<h1>Welcome to the Smart Parking System</h1>"
            "<p>This system is designed for reserved users. If you do not have a reservation, click 'No' to check available spots and make a reservation.</p>"
            "<h2>Do you have a reservation?</h2>"
            "<button onclick=\"window.location.href='/enter.html'\">Yes</button>"
            "<button onclick=\"window.location.href='/parking.html'\">No</button>"
            "</body>"
            "</html>");

        response = html_page;
    }
    // Serve enter.html (PIN entry form with messages)
    else if (!strcmp(name, "/enter.html")) {
        html_page = (char *)calloc(2048, 1);
        if (!html_page) {
            return 0;
        }

        strcat(html_page,
            "<!DOCTYPE html>"
            "<html>"
            "<head><title>Enter Parking</title>"
            "<style>"
            "body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 0; display: flex; flex-direction: column; justify-content: center; height: 100vh; }"
            "form { margin: 20px auto; }"
            "a { margin: 10px; display: inline-block; }"
            "</style>"
            "</head>"
            "<body>"
            "<h1>Enter Your PIN</h1>");

        if (strlen(enter_message) > 0) {
            strcat(html_page,
                "<div style='border: 2px solid green; padding: 10px; margin-bottom: 10px;'>");
            strcat(html_page, enter_message);
            strcat(html_page, "</div>");
            memset(enter_message, 0, sizeof(enter_message)); // Clear the message after displaying
        }

        strcat(html_page,
            "<form action='/enter_parking' method='GET'>"
            "PIN: <input type='text' name='pin'> "
            "<input type='submit' value='Enter'>"
            "</form>"
            "<a href='/main.html'>Back to Main</a>"
            "<a href='/parking.html'>Back to Parking</a>"
            "</body>"
            "</html>");

        response = html_page;
    }
    // Handle PIN submission for enter.html
    // else if (!strcmp(name, "/enter_parking")) {
    //     const char *entered_pin = "123"; // Example: Replace with actual query parsing
    //     if (check_general_pin(entered_pin)) {
    //         snprintf(enter_message, sizeof(enter_message), "Barrier opened. Welcome!");
    //     } else {
    //         snprintf(enter_message, sizeof(enter_message), "Invalid PIN.");
    //     }
    //     return fs_open_custom(file, "/enter.html"); // Reload enter.html with the message
    // }
    else if (!strcmp(name, "/enter_parking")) {
        const char *entered_pin = "123"; // Example: Replace with actual query parsing
        if (check_general_pin(entered_pin)) {
            snprintf(last_message, sizeof(last_message), "Barrier opened. Welcome!");
            return fs_open_custom(file, PARKING_UI_ENDPOINT); // Redirect to parking.html
        } else {
            snprintf(last_message, sizeof(last_message), "Invalid PIN. Please try again or reserve a new spot.");
            return fs_open_custom(file, PARKING_UI_ENDPOINT); // Redirect to parking.html
        }
    }
    // Serve parking.html (main UI with navigation)
    else if (!strcmp(name, PARKING_UI_ENDPOINT)) {
        html_page = (char *)calloc(4096, 1);
        if (!html_page) {
            return 0;
        }

        strcat(html_page,
            "<!DOCTYPE html>"
            "<html>"
            "<head><title>Smart Parking System UI</title>"
            "<style>"
            "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }"
            "a { margin: 10px; display: inline-block; }"
            "</style>"
            "</head>"
            "<body>");

        if (strlen(last_message) > 0) {
            strcat(html_page,
                "<div style='border:2px solid green; padding:10px; margin-bottom:10px;'>");
            strcat(html_page, last_message);
            strcat(html_page, "</div>");
            memset(last_message, 0, sizeof(last_message)); // Clear the message after displaying
        }

        strcat(html_page,
            "<h1>Smart Parking System App</h1>"
            "<p>Use the links below to interact with the parking system.</p>"
            "<hr>"
            "<h2>Check Availability</h2>"
            "<p><a href='/check_availability'>Check Available Spots</a></p>"
            "<hr>"
            "<h2>Reserve a Spot</h2>"
            "<p><a href='/reserve_spot'>Reserve a Spot</a></p>"
            "<hr>"
            "<h2>Enter Parking</h2>"
            "<form action='/enter_parking' method='GET'>"
            "PIN: <input type='text' name='pin'> "
            "<input type='submit' value='Enter'>"
            "</form>"
            "<hr>"
            "<a href='/main.html'>Back to Main</a>"
            "<a href='/enter.html'>Back to Enter Page</a>"
            "</body></html>");

        response = html_page;
    } else {
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



//////////////////////////////////////////////////////////
// --> Another version for handling fs custom open <-- //
////////////////////////////////////////////////////////
// int fs_open_custom(struct fs_file *file, const char *name) {
//     printf("[DEBUG] fs_open_custom called for: %s\r\n", name);

//     const char *response = NULL;
//     char *html_page = NULL;

//     // Page-specific message variables
//     static char enter_message[256] = {0};

//     // Serve main.html
//     if (!strcmp(name, "/main.html")) {
//         html_page = (char *)calloc(2048, 1);
//         if (!html_page) {
//             return 0;
//         }

//         strcat(html_page,
//             "<!DOCTYPE html>"
//             "<html>"
//             "<head><title>Welcome to Smart Parking</title>"
//             "<style>"
//             "body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 0; display: flex; flex-direction: column; justify-content: center; height: 100vh; }"
//             "button { margin: 10px; padding: 10px 20px; font-size: 16px; }"
//             "</style>"
//             "</head>"
//             "<body>"
//             "<h1>Welcome to the Smart Parking System</h1>"
//             "<p>This system is designed for reserved users.</p>"
//             "<h2>Do you have a reservation?</h2>"
//             "<button onclick=\"window.location.href='/enter.html'\">Yes</button>"
//             "<button onclick=\"window.location.href='/parking.html'\">No</button>"
//             "</body>"
//             "</html>");

//         response = html_page;
//     }
//     // Serve enter.html
//     else if (!strcmp(name, "/enter.html")) {
//         html_page = (char *)calloc(2048, 1);
//         if (!html_page) {
//             return 0;
//         }

//         strcat(html_page,
//             "<!DOCTYPE html>"
//             "<html>"
//             "<head><title>Enter Parking</title>"
//             "<style>"
//             "body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 0; display: flex; flex-direction: column; justify-content: center; height: 100vh; }"
//             "form { margin: 20px auto; }"
//             "a { margin: 10px; display: inline-block; }"
//             "</style>"
//             "</head>"
//             "<body>"
//             "<h1>Enter Your PIN</h1>");

//         if (strlen(enter_message) > 0) {
//             strcat(html_page,
//                 "<div style='border: 2px solid red; padding: 10px; margin-bottom: 10px;'>");
//             strcat(html_page, enter_message);
//             strcat(html_page, "</div>");
//             memset(enter_message, 0, sizeof(enter_message)); // Clear the message after displaying
//         }

//         strcat(html_page,
//             "<form action='/enter_parking' method='GET'>"
//             "PIN: <input type='text' name='pin'> "
//             "<input type='submit' value='Enter'>"
//             "</form>"
//             "<a href='/main.html'>Back to Main</a>"
//             "<a href='/parking.html'>Back to Parking</a>"
//             "</body>"
//             "</html>");

//         response = html_page;
//     }
//     // Handle PIN submission
//     else if (!strcmp(name, "/enter_parking")) {
//         const char *entered_pin = "123"; // Example: Replace with actual query parsing
//         if (check_general_pin(entered_pin)) {
//             snprintf(enter_message, sizeof(enter_message), "Barrier opened. Welcome!");
//         } else {
//             snprintf(enter_message, sizeof(enter_message), "Invalid PIN. Please try again.");
//         }
//         return fs_open_custom(file, "/enter.html"); // Reload enter.html with the message
//     }
//     // Serve parking.html
//     else if (!strcmp(name, PARKING_UI_ENDPOINT)) {
//         html_page = (char *)calloc(4096, 1);
//         if (!html_page) {
//             return 0;
//         }

//         strcat(html_page,
//             "<!DOCTYPE html>"
//             "<html>"
//             "<head><title>Smart Parking System UI</title>"
//             "<style>"
//             "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }"
//             "a { margin: 10px; display: inline-block; }"
//             "</style>"
//             "</head>"
//             "<body>");

//         if (strlen(last_message) > 0) {
//             strcat(html_page,
//                 "<div style='border:2px solid green; padding:10px; margin-bottom:10px;'>");
//             strcat(html_page, last_message);
//             strcat(html_page, "</div>");
//             memset(last_message, 0, sizeof(last_message)); // Clear the message after displaying
//         }

//         strcat(html_page,
//             "<h1>Welcome to Smart Parking System App</h1>"
//             "<p>Use the links below to interact with the parking system.</p>"
//             "<hr>"
//             "<h2>Check Availability</h2>"
//             "<p><a href='/check_availability'>Check Available Spots</a></p>"
//             "<hr>"
//             "<h2>Reserve a Spot</h2>"
//             "<p><a href='/reserve_spot'>Reserve a Spot</a></p>"
//             "<hr>"
//             "<h2>Enter Parking</h2>"
//             "<form action='/enter_parking' method='GET'>"
//             "PIN: <input type='text' name='pin'> "
//             "<input type='submit' value='Enter'>"
//             "</form>"
//             "<hr>"
//             "<a href='/main.html'>Back to Main</a>"
//             "<a href='/enter.html'>Back to Enter Page</a>"
//             "</body></html>");

//         response = html_page;
//     } else {
//         return 0; // Unknown path
//     }

//     // Handle dynamic response
//     if (!response) {
//         if (html_page) {
//             free(html_page);
//         }
//         return 0;
//     }

//     int response_size = strlen(response);

//     memset(file, 0, sizeof(struct fs_file));
//     file->pextension = mem_malloc(response_size);
//     if (!file->pextension) {
//         if (html_page) {
//             free(html_page);
//         }
//         return 0;
//     }

//     memcpy(file->pextension, response, response_size);
//     file->data = (const char *)file->pextension;
//     file->len = response_size;
//     file->index = response_size;
//     file->flags = FS_FILE_FLAGS_HEADER_PERSISTENT;

//     if (html_page) {
//         free(html_page);
//     }

//     return 1;
// }



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
