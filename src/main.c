#include <pebble.h>
#include "main_window.h"
#include "weather_service.h"
        
// Global variables
static MainWindow *main_window;
    
// Function predeclarations
static void initialize();
static void shutdown();
static void tick_handler(struct tm *, TimeUnits);
static void weather_handler(int, char *);

int main() {
    initialize();
    app_event_loop();
    shutdown();
}

static void initialize() {
    APP_LOG(APP_LOG_LEVEL_INFO, "Initializing");
    
    // Create main window and push onto stack
    main_window = main_window_create();
    window_stack_push(main_window_get_window(main_window), false);
    
    // Subscribe to services
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    weather_service_subscribe(weather_handler);
    
    // Request initial weather
    weather_service_request();
}

static void shutdown() {
    APP_LOG(APP_LOG_LEVEL_INFO, "Shutting down");
    
    // Destroy main window
    main_window_destroy(main_window);
    
    // Unsubscribe from services
    tick_timer_service_unsubscribe();
    weather_service_unsubscribe();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    // Set time on main window clock display
    main_window_set_time(main_window, tick_time);
    
    if (tick_time->tm_min % 30 == 0) {
        weather_service_request();
    }
}

static void weather_handler(int temperature, char *conditions) {
    main_window_set_temperature(main_window, temperature);
    main_window_set_conditions(main_window, conditions);
}