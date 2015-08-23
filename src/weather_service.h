#pragma once

typedef void (* WeatherCallback)(int, char *);
void weather_service_subscribe(WeatherCallback);
void weather_service_unsubscribe();
void weather_service_request();