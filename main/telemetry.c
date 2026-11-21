#include "telemetry.h"
#include "mqtt_client.h"
#include "esp_crt_bundle.h"
#include "esp_mac.h"    

static const char *TAG = "mqtt_client";

static esp_mqtt_client_config_t mqtt_cfg;
static char s_client_id[48]; 

/**
 * @brief configures the mqtt client, pulls data from esp menu config.
 * 
 */
static void mqtt_client_config(void)
{
    //https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/protocols/mqtt.html
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(s_client_id, sizeof(s_client_id),
             CONFIG_APP_MQTT_CLIENT_ID_PREFIX "-%02x%02x%02x%02x%02x%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    mqtt_cfg.broker.address.uri = CONFIG_APP_MQTT_BROKER_URI; //"mqtts://broker.emqx.io:8883"   
    mqtt_cfg.credentials.client_id = s_client_id;
}

/**
 * @brief Initializes the mqtt event handler
 * 
 */
static void mqtt_event_handler_init(void)
{
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
}

/**
 * @brief Callback to handle mqtt client events
 * 
 * @param handler_args 
 * @param base 
 * @param event_id 
 * @param event_data 
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_t *event = event_data;

    switch(event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_CONNECTED");
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT_DISCONNECTED");
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "RECIEVED_TOPIC: %.*s", event->topic_len, event->topic);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_ERROR");
            break;
        
        default:
            break;
    }
}

static void mqtt_client_task()
{

    mqtt_client_config();

    mqtt_event_handler_init();

    for(;;)
    {
        
    }
}

void mqtt_client_start(void)
{
    //start the mqtt client task
    xTaskCreatePinnedToCore(&mqtt_client_task, 
                            "wifi_app_task", 
                            MQTT_CLIENT_TASK_STACK_SIZE, 
                            NULL, 
                            MQTT_CLIENT_TASK_PRIORITY, 
                            NULL, 
                            MQTT_CLIENT_TASK_CORE_ID);
}