#include "telemetry.h"
#include "mqtt_client.h"
#include "esp_crt_bundle.h"
#include "esp_mac.h"    
#include "esp_log.h"
#include "tasks_common.h"
#include "DHT22.h"

static const char *TAG = "mqtt_client";

static esp_mqtt_client_config_t s_mqtt_cfg;
static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static char s_client_id[48]; 

static EventGroupHandle_t s_mqtt_event_group = NULL;
static const EventBits_t MQTT_CONNECTED_BIT = BIT0;


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_t *event = event_data;

    switch(event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_CONNECTED");
            if (s_mqtt_event_group != NULL) {
                xEventGroupSetBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT_DISCONNECTED");
            if (s_mqtt_event_group != NULL) {
                xEventGroupClearBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            }
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

void mqtt_client_init_and_start(void)
{
    s_mqtt_client = esp_mqtt_client_init(&s_mqtt_cfg);
    if(s_mqtt_client == NULL) 
    {
        ESP_LOGW(TAG, "Failed to initialize MQTT client");
        return;
    }
    esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, s_mqtt_client);
    esp_mqtt_client_start(s_mqtt_client);

}


static void mqtt_client_config(void)
{
    //https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/protocols/mqtt.html
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(s_client_id, sizeof(s_client_id),
             CONFIG_APP_MQTT_CLIENT_ID_PREFIX "-%02x%02x%02x%02x%02x%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    s_mqtt_cfg.broker.address.uri = CONFIG_APP_MQTT_BROKER_URI; //"mqtts://broker.emqx.io:8883"   
    s_mqtt_cfg.credentials.client_id = s_client_id;
}


static int mqtt_publish_data(const char *subtopic, float value)
{
    if (s_mqtt_client == NULL || subtopic == NULL) {
        return -1;
    }

    char topic[160];
    int topic_len = snprintf(topic, sizeof(topic),
                             "%s/%s/%s",
                             CONFIG_APP_MQTT_TOPIC_PREFIX,
                             s_client_id,
                             subtopic);
    if (topic_len < 0 || topic_len >= (int)sizeof(topic)) {
        ESP_LOGE(TAG, "Topic string truncated or invalid");
        return -1;
    }

    char payload[32];
    int payload_len = snprintf(payload, sizeof(payload), "%.2f", value);
    if (payload_len < 0 || payload_len >= (int)sizeof(payload)) {
        ESP_LOGE(TAG, "Payload format error");
        return -1;
    }
    int msg_id = esp_mqtt_client_publish(s_mqtt_client,
                                         topic,
                                         payload,
                                         payload_len,
                                         CONFIG_APP_MQTT_QOS,
                                         CONFIG_APP_MQTT_RETAIN ? 1 : 0);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish to %s", topic);
    } else {
        ESP_LOGI(TAG, "Published msg_id=%d topic=%s payload=%s", msg_id, topic, payload);
    }
    return msg_id;
}

static int mqtt_publish_telemetry(void)
{
    // Get data from the DHT
    int temp_rc = 0;
    int hum_rc = 0;
    int ret = readDHT();

    errorHandler(ret);
    if (ret == DHT_OK) {
        float temp_c = getTemperature();
        float humidity_pct = getHumidity();
        temp_rc = mqtt_publish_data("temperature", temp_c);
        hum_rc = mqtt_publish_data("humidity", humidity_pct);
    }

    return (temp_rc < 0 || hum_rc < 0) ? -1 : 0;
}


static void mqtt_client_task(void *pvParameters)
{
    // Initialize and start the client connection
    mqtt_client_config();
    // mqtt_client_init_and_start();

    // Initialize sensor, called directly for simplicity
    setDHTgpio(DHT_GPIO);

    for(;;)
    {   
        EventBits_t eventBits = xEventGroupWaitBits(s_mqtt_event_group,
                                                    MQTT_CONNECTED_BIT,
                                                    pdFALSE,
                                                    pdTRUE,
                                                    pdMS_TO_TICKS(10000));

        if((eventBits & MQTT_CONNECTED_BIT) == 0) {
            ESP_LOGW(TAG, "Waiting for MQTT connection...");
            continue;
        }
        int ret = mqtt_publish_telemetry();
        if(ret){
            ESP_LOGW(TAG, "Failed to publish data to topic...");
        }
        vTaskDelay(pdMS_TO_TICKS(CONFIG_APP_MQTT_PUBLISH_INTERVAL_S * 1000U));
    }
}

void mqtt_client_start(void)
{
    // Create the event group to signal client connection state
    s_mqtt_event_group = xEventGroupCreate();
    
    // Start the mqtt client task
    xTaskCreatePinnedToCore(&mqtt_client_task, 
                            "mqtt_client_task",
                            MQTT_CLIENT_TASK_STACK_SIZE, 
                            NULL, 
                            MQTT_CLIENT_TASK_PRIORITY, 
                            NULL, 
                            MQTT_CLIENT_TASK_CORE_ID);
}