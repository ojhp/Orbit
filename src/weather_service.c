#include <pebble.h>
#include "weather_service.h"
  
// Dictionary entry keys
enum {
    KEY_REQUEST = 0,
    KEY_TEMPERATURE = 1,
    KEY_CONDITIONS = 2
};

// Persistence keys
enum {
    PROPERTY_KEY_TEMPERATURE = 0,
    PROPERTY_KEY_CONDITIONS = 1,
    PROPERTY_KEY_TIME = 2
};

static void inbox_received_callback(DictionaryIterator *, void *);
static void inbox_dropped_callback(AppMessageResult, void *);
static void outbox_sent_callback(DictionaryIterator *, void *);
static void outbox_failed_callback(DictionaryIterator *, AppMessageResult, void *);
    
void weather_service_subscribe(WeatherCallback callback) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Registering weather service");
    
    // Register callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    
    // Set context to callback and start message service
    app_message_set_context(callback);
    app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
}

void weather_service_unsubscribe() {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Unsubscribing from weather service");
    
    // Unregister callbacks
    app_message_deregister_callbacks();
}

void weather_service_request() {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Requesting weather information");
    
    // Get time from cache if available
    if (persist_exists(PROPERTY_KEY_TIME)) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Cached data found");
        
        // Get cache time and current time to compare
        time_t cache_time;
        persist_read_data(PROPERTY_KEY_TIME, &cache_time, sizeof(time_t));
        time_t current_time = time(NULL);
        
        // If less than 30 mins has elapsed since cached
        int diff = current_time - cache_time;
        if (diff < (29 * 60) && persist_exists(PROPERTY_KEY_TEMPERATURE) && persist_exists(PROPERTY_KEY_CONDITIONS)) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Cached data in date");
            
            // Get cached data
            int temperature = persist_read_int(PROPERTY_KEY_TEMPERATURE);
            char conditions[16];
            persist_read_string(PROPERTY_KEY_CONDITIONS, conditions, sizeof(conditions));
            
            // Get and call callback method and return
            WeatherCallback callback = (WeatherCallback)app_message_get_context();
            callback(temperature, conditions);
            return;
        } else {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Cached data out of date");
        }
    } else {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "No cache found");
    }
    
    // Send request message if cache data missing or invalid
    DictionaryIterator *dict;
    app_message_outbox_begin(&dict);
    dict_write_uint8(dict, KEY_REQUEST, 0);
    app_message_outbox_send();
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Weather information received");
    
    // Declare defaults
    int temperature = -1;
    char conditions[16] = "Unknown";
    
    // Loop through dictionary items
    Tuple *t = dict_read_first(iterator);
    while (t != NULL) {
        // Set details based on tuple key
        switch (t->key) {
            case KEY_TEMPERATURE:
                temperature = t->value->int16;
                break;
            case KEY_CONDITIONS:
                strncpy(conditions, t->value->cstring, sizeof(conditions));
                break;
            default:
                APP_LOG(APP_LOG_LEVEL_WARNING, "Unknown dictionary key: %u", (unsigned int)t->key);
                break;
        }
        
        // Get next tuple
        t = dict_read_next(iterator);
    }
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Temperature: %dC, Conditions: %s", temperature, conditions);
    
    // Call callback method
    WeatherCallback callback = (WeatherCallback)context;
    callback(temperature, conditions);
    
    // Persist weather data and timestamp
    time_t tm = time(NULL);
    persist_write_int(PROPERTY_KEY_TEMPERATURE, temperature);
    persist_write_string(PROPERTY_KEY_CONDITIONS, conditions);
    persist_write_data(PROPERTY_KEY_TIME, &tm, sizeof(time_t));
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to get weather information");
    
    // Call callback with error details
    WeatherCallback callback = (WeatherCallback)context;
    callback(-1, "Unknown");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Weather request sent");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Weather request failed to send");
}