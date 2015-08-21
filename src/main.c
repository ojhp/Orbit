#include <pebble.h>
    
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

// UI elements and data
static Window *main_window;
static int clock_size;
static Layer *clock_layer;
static int clock_hours, clock_mins;
static TextLayer *text_layer;
static char text_buffer[6];

// Function predeclarations
static void initialize();
static void shutdown();
static void main_window_load(Window *);
static void main_window_unload(Window *);
static void update_clock(Layer *, GContext *);
static void draw_hand(GContext *, int, int, GColor);
static void tick_handler(struct tm *, TimeUnits);

int main() {
    initialize();
    app_event_loop();
    shutdown();
}

static void initialize() {
    APP_LOG(APP_LOG_LEVEL_INFO, "Initializing");
    
    // Create main window and push onto stack
    main_window = window_create();
    window_set_background_color(main_window, GColorBlack);
    window_set_window_handlers(main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });
    window_stack_push(main_window, false);
}

static void shutdown() {
    APP_LOG(APP_LOG_LEVEL_INFO, "Shutting down");
    
    // Destroy main window
    window_destroy(main_window);
}

static void main_window_load(Window *window) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Main window loaded");
    
    // Get window root
    Layer *root = window_get_root_layer(main_window);
    GRect window_bounds = layer_get_bounds(root);
    
    // Calculate clock size and bounds
    clock_size = window_bounds.size.w < window_bounds.size.h ? window_bounds.size.w : window_bounds.size.h;
    GRect clock_bounds = GRect((window_bounds.size.w - clock_size) / 2, (window_bounds.size.h - clock_size) / 2, clock_size, clock_size);
    
    // Set clock time
    time_t t = time(NULL);
    struct tm *clock_time = localtime(&t);
    clock_hours = clock_time->tm_hour;
    clock_mins = clock_time->tm_min;
    strftime(text_buffer, sizeof(text_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", clock_time);
    
    // Create clock layer
    clock_layer = layer_create(clock_bounds);
    layer_set_update_proc(clock_layer, update_clock);
    layer_add_child(root, clock_layer);
    
    // Create text layer
    text_layer = text_layer_create(GRect(0, (window_bounds.size.h - TEXT_HEIGHT) / 2, window_bounds.size.w, TEXT_HEIGHT));
    text_layer_set_background_color(text_layer, GColorClear);
    text_layer_set_text_color(text_layer, TEXT_COLOR);
    text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
    text_layer_set_font(text_layer, fonts_get_system_font(TEXT_FONT));
    text_layer_set_text(text_layer, text_buffer);
    layer_add_child(root, text_layer_get_layer(text_layer));
    
    // Subscribe to second ticks
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void main_window_unload(Window *window) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Main window unloaded");
    
    // Destroy clock layer
    layer_destroy(clock_layer);
    text_layer_destroy(text_layer);
    
    // Unsubscribe from tick service
    tick_timer_service_unsubscribe();
}

static void update_clock(Layer *layer, GContext *ctx) {
    // Calculate angles
    int minute_angle = TRIG_MAX_ANGLE * clock_mins / 60;
    int hour_angle = (TRIG_MAX_ANGLE * (clock_hours % 12) / 12) + ((minute_angle * (TRIG_MAX_ANGLE / 12)) / TRIG_MAX_ANGLE);
    
    // Draw hands
    draw_hand(ctx, minute_angle, (clock_size / 2) - 5, MINUTE_COLOR);
    draw_hand(ctx, hour_angle, (clock_size / 2) - 15, HOUR_COLOR);
}

static void draw_hand(GContext *ctx, int angle, int radius, GColor color) {
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

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    // Set time
    clock_hours = tick_time->tm_hour;
    clock_mins = tick_time->tm_min;
    
    // Update text
    strftime(text_buffer, sizeof(text_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
    text_layer_set_text(text_layer, text_buffer);
    
    // Force redraw of hands
    layer_mark_dirty(clock_layer);
}