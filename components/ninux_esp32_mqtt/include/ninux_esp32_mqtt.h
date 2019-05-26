
#include "ninux_esp32_mqtt.h"
#include "mqtt_client.h"

//static const char *TAG = "MQTT_DATA";

char mqtt_data[512];
char mqtt_topic[512];

void ninux_mqtt_publish(char* topic, char* data);
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);
static void mqtt_app_start(void);
