#include <pebble.h>
#include "main_window.h"

// Define colours if on a colour pebble, otherwise everything's white
#ifdef PBL_COLOR
    #define MINUTE_COLOR GColorBlueMoon
    #define HOUR_COLOR GColorIslamicGreen
    #define TEXT_COLOR GColorBlueMoon
#else
    #define MINUTE_COLOR GColorWhite
    #define HOUR_COLOR GColorWhite
    #define TEXT_COLOR GColorWhite
#endif
    
// Select font and size based on which platform we're on
#ifdef PBL_PLATFORM_BASALT
    #define TEXT_FONT FONT_KEY_LECO_32_BOLD_NUMBERS
    #define TEXT_HEIGHT 40
#else
    #define TEXT_FONT FONT_KEY_BITHAM_34_MEDIUM_NUMBERS
    #define TEXT_HEIGHT 42
#endif
    
static void main_window_load(Window *);
static void main_window_unload(Window *);
static void update_clock(Layer *, GContext *);
static void draw_hand(GContext *, int, int, int, GColor);
    
struct MainWindow {
    Window *window;
    Layer *clock_layer;
    TextLayer *text_layer;
    int clock_size, clock_hours, clock_mins;
    char text_buffer[6];
};

MainWindow *main_window_create() {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Creating main window");
    
    // Allocate structure space
    MainWindow *mw = (MainWindow *)malloc(sizeof(MainWindow));
    
    // Create window
    mw->window = window_create();
    window_set_background_color(mw->window, GColorBlack);
    window_set_user_data(mw->window, mw);
    window_set_window_handlers(mw->window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });
    
    return mw;
}

Window *main_window_get_window(MainWindow *mw) {
    return mw->window;
}

void main_window_set_time(MainWindow *mw, struct tm *time) {
    // Set time
    mw->clock_hours = time->tm_hour;
    mw->clock_mins = time->tm_min;
    
    // Update text
    strftime(mw->text_buffer, sizeof(mw->text_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", time);
    text_layer_set_text(mw->text_layer, mw->text_buffer);
    
    // Force redraw of hands
    layer_mark_dirty(mw->clock_layer);
}

void main_window_destroy(MainWindow *mw) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Destroying main window");
    
    window_destroy(mw->window);
    free(mw);
}

static void main_window_load(Window *window) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Main window loaded");
    
    // Obtain main window object
    MainWindow *mw = (MainWindow *)window_get_user_data(window);
    
    // Get window root
    Layer *root = window_get_root_layer(mw->window);
    GRect window_bounds = layer_get_bounds(root);
    
    // Calculate clock size and bounds
    mw->clock_size = window_bounds.size.w < window_bounds.size.h ? window_bounds.size.w : window_bounds.size.h;
    GRect clock_bounds = GRect((window_bounds.size.w - mw->clock_size) / 2,
                               (window_bounds.size.h - mw->clock_size) / 2,
                               mw->clock_size,
                               mw->clock_size);
    
    // Set clock time
    time_t t = time(NULL);
    struct tm *clock_time = localtime(&t);
    mw->clock_hours = clock_time->tm_hour;
    mw->clock_mins = clock_time->tm_min;
    strftime(mw->text_buffer, sizeof(mw->text_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", clock_time);
    
    // Create clock layer
    mw->clock_layer = layer_create_with_data(clock_bounds, sizeof(void *));
    MainWindow **mw_data = (MainWindow **)layer_get_data(mw->clock_layer);
    *mw_data = mw;
    layer_set_update_proc(mw->clock_layer, update_clock);
    layer_add_child(root, mw->clock_layer);
    
    // Create text layer
    mw->text_layer = text_layer_create(GRect(0, (window_bounds.size.h - TEXT_HEIGHT) / 2, window_bounds.size.w, TEXT_HEIGHT));
    text_layer_set_background_color(mw->text_layer, GColorClear);
    text_layer_set_text_color(mw->text_layer, TEXT_COLOR);
    text_layer_set_text_alignment(mw->text_layer, GTextAlignmentCenter);
    text_layer_set_font(mw->text_layer, fonts_get_system_font(TEXT_FONT));
    text_layer_set_text(mw->text_layer, mw->text_buffer);
    layer_add_child(root, text_layer_get_layer(mw->text_layer));
}

static void main_window_unload(Window *window) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Main window unloaded");
    
    // Get main window structure
    MainWindow *mw = (MainWindow *)window_get_user_data(window);
    
    // Destroy clock layer
    layer_destroy(mw->clock_layer);
    text_layer_destroy(mw->text_layer);
}

static void update_clock(Layer *layer, GContext *ctx) {
    // Get main window structure
    MainWindow *mw = *((MainWindow **)(layer_get_data(layer)));
    
    // Calculate angles
    int minute_angle = TRIG_MAX_ANGLE * mw->clock_mins / 60;
    int hour_angle = (TRIG_MAX_ANGLE * (mw->clock_hours % 12) / 12) + ((minute_angle * (TRIG_MAX_ANGLE / 12)) / TRIG_MAX_ANGLE);
    
    // Draw hands
    draw_hand(ctx, mw->clock_size, minute_angle, (mw->clock_size / 2) - 5, MINUTE_COLOR);
    draw_hand(ctx, mw->clock_size, hour_angle, (mw->clock_size / 2) - 15, HOUR_COLOR);
}

static void draw_hand(GContext *ctx, int clock_size, int angle, int radius, GColor color) {
    // Find center point
    GPoint center = GPoint(clock_size / 2, clock_size / 2);
    
    // Set stroke and antialiasing if available
    #ifdef PBL_PLATFORM_BASALT
    graphics_context_set_stroke_width(ctx, 2);
    graphics_context_set_antialiased(ctx, true);
    #endif
    
    // Draw ring
    graphics_context_set_stroke_color(ctx, color);
    graphics_draw_circle(ctx, center, radius);
    
    // Calculate hand position
    int32_t x = (sin_lookup(angle) * radius / TRIG_MAX_RATIO) + center.x;
    int32_t y = (-cos_lookup(angle) * radius / TRIG_MAX_RATIO) + center.y;
    
    // Draw hand
    graphics_context_set_fill_color(ctx, color);
    graphics_fill_circle(ctx, GPoint(x, y), 5);
}