#include <pebble.h>
#include "main_window.h"

// Global variables
static MainWindow *main_window;
    
// Function predeclarations
static void initialize();
static void shutdown();
static void tick_handler(struct tm *, TimeUnits);

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
    
    // Subscribe to second ticks
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void shutdown() {
    APP_LOG(APP_LOG_LEVEL_INFO, "Shutting down");
    
    // Destroy main window
    main_window_destroy(main_window);
    
    // Unsubscribe from tick service
    tick_timer_service_unsubscribe();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    // Set time on main window clock display
    main_window_set_time(main_window, tick_time);
}