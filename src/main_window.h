#pragma once
#include <pebble.h>
    
struct MainWindow;
typedef struct MainWindow MainWindow;

MainWindow *main_window_create();
Window *main_window_get_window(MainWindow *);
void main_window_set_time(MainWindow *, struct tm *);
void main_window_destroy(MainWindow *);