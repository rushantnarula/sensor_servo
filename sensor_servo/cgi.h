#ifndef TT_CGI_H
#define TT_CGI_H 1

/* Ensure required dependencies are included */
#if !LWIP_HTTPD_CUSTOM_FILES
#error This needs LWIP_HTTPD_CUSTOM_FILES
#endif

#if !LWIP_HTTPD_DYNAMIC_HEADERS
#error This needs LWIP_HTTPD_DYNAMIC_HEADERS
#endif

#if !LWIP_HTTPD_CGI
#error This needs LWIP_HTTPD_CGI

#endif

/* Endpoints for the parking system */
// #define ENTER_UI_ENDPOINT "/enter.html"
#define PARKING_UI_ENDPOINT "/parking.html"
#define ERROR_404_ENDPOINT "/404.html"

#define PARKING_STATUS_ENDPOINT "/check_availability"
#define RESERVE_SPOT_ENDPOINT "/reserve_spot"
#define ENTER_PARKING_ENDPOINT "/enter_parking"

#define TEST_ENDPOINT "/test" /* Optional: This just for testing, no need for this */

/* Initialization functions */
void custom_files_init(void);
void cgi_init(void);

#endif /* TT_CGI_H */
