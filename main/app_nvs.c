/**
 * @file app_nvs.c
 * @author Daniel Albu (albud456@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-12-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <esp_log.h>
#include <nvs_flash.h>

#include <wifi_app.h>
#include "app_nvs.h"


static const char *TAG = "app_nvs";

#define NVS_KEY_SSID     "ssid"
#define NVS_KEY_PASSWORD "password"
static const char *const sta_cred_keys[] = { NVS_KEY_SSID, NVS_KEY_PASSWORD };
const char app_nvs_sta_creds_namespace[] = "stacreds";

_Static_assert(MAXIMUM_SSID_LENGTH == sizeof(((wifi_config_t *)0)->sta.ssid),
               "SSID length constant must match wifi_config_t");
_Static_assert(MAXIMUM_PASSWORD_LENGTH == sizeof(((wifi_config_t *)0)->sta.password),
               "password length constant must match wifi_config_t");

esp_err_t app_nvs_save_sta_creds(void)
{
    nvs_handle handle;
    esp_err_t esp_err;
    ESP_LOGI(TAG, "app_nvs_save_sta_credentials: Saving STA creds to Flash");

    wifi_config_t *wifi_sta_config = wifi_app_get_wifi_config();
    if(wifi_sta_config == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err = nvs_open(app_nvs_sta_creds_namespace, NVS_READWRITE, &handle);
    if(esp_err != ESP_OK)
    {
        ESP_LOGE(TAG, "error opening nvs handle: %s", esp_err_to_name(esp_err));
        return esp_err;
    }
    esp_err = nvs_set_blob(handle, NVS_KEY_SSID, wifi_sta_config->sta.ssid, MAXIMUM_SSID_LENGTH);
    if(esp_err == ESP_OK)
    {
        esp_err = nvs_set_blob(handle, NVS_KEY_PASSWORD, wifi_sta_config->sta.password, MAXIMUM_PASSWORD_LENGTH);
    }
    if(esp_err == ESP_OK)
    {
        //commit credentials to nvs
        esp_err = nvs_commit(handle);
    }
    nvs_close(handle);    

    if(esp_err != ESP_OK)
    {
        ESP_LOGE(TAG, "saving credentials failed: %s", esp_err_to_name(esp_err));
    }
    else
    {
        ESP_LOGI(TAG, "saved credentials for SSID '%.32s'", wifi_sta_config->sta.ssid);
    }

    return esp_err;
}

bool app_nvs_load_sta_creds(void)
{
    nvs_handle handle;
    esp_err_t esp_err;
    ESP_LOGI(TAG, "app_nvs_load_sta_creds: loading sta creds from flash");

    wifi_config_t *wifi_sta_config = wifi_app_get_wifi_config();
    if(wifi_sta_config == NULL)
    {
        ESP_LOGE(TAG, "wifi config not allocated yet, call wifi_app_start first");
        return false;
    }
    if(nvs_open(app_nvs_sta_creds_namespace, NVS_READONLY, &handle) == ESP_OK)
    {

        memset(wifi_sta_config, 0x00, sizeof(wifi_config_t));
        size_t wifi_config_size = sizeof(wifi_config_t);
        uint8_t *wifi_config_buff = (uint8_t*)malloc(sizeof(uint8_t) * wifi_config_size);
        if(wifi_config_buff == NULL)
        {
            nvs_close(handle);
            return false;
        }
        memset(wifi_config_buff, 0x00, wifi_config_size);

        //load ssid
        wifi_config_size = sizeof(wifi_sta_config->sta.ssid);
        esp_err = nvs_get_blob(handle, NVS_KEY_SSID, wifi_config_buff, &wifi_config_size);
        if(esp_err != ESP_OK)
        {
            free(wifi_config_buff);
            ESP_LOGW(TAG, "app_nvs_load_sta_creds: %s no station SSID found in NVS", esp_err_to_name(esp_err));
            nvs_close(handle);
            memset(wifi_sta_config, 0x00, sizeof(wifi_config_t));
            return false;
        }
        memcpy(wifi_sta_config->sta.ssid, wifi_config_buff, wifi_config_size);

        //load password
        wifi_config_size = sizeof(wifi_sta_config->sta.password);
        esp_err = nvs_get_blob(handle, NVS_KEY_PASSWORD, wifi_config_buff, &wifi_config_size);
        if(esp_err != ESP_OK)
        {
            free(wifi_config_buff);
            ESP_LOGW(TAG, "app_nvs_load_sta_creds: %s no station password found in NVS", esp_err_to_name(esp_err));
            memset(wifi_sta_config, 0x00, sizeof(wifi_config_t));
            nvs_close(handle);
            return false;
        }
        memcpy(wifi_sta_config->sta.password, wifi_config_buff, wifi_config_size);

        free(wifi_config_buff);
        nvs_close(handle);
        if(wifi_sta_config->sta.ssid[0] == '\0')
        {
            // Both reads succeeded but the stored SSID is empty. Treat it as no credentials
            memset(wifi_sta_config, 0x00, sizeof(wifi_config_t));
            return false;
        }
        return true;
    }
    else{
        ESP_LOGI(TAG, "no saved credentials");
        return false;
    }
}

esp_err_t app_nvs_clear_sta_creds(void)
{
    nvs_handle handle;
    esp_err_t esp_err;
    ESP_LOGI(TAG, "app_nvs_clear_sta_creds: clearing sta credentials from flash");
    
    esp_err = nvs_open(app_nvs_sta_creds_namespace, NVS_READWRITE, &handle);
    if(esp_err != ESP_OK)
    {
        ESP_LOGE(TAG, "error opening nvs handle: %s", esp_err_to_name(esp_err));
        return esp_err;
    }
    //erase specific key credentials. may add data to NVS later on, do not wipe all.
    for(size_t i = 0; i < sizeof(sta_cred_keys) / sizeof(sta_cred_keys[0]) && esp_err == ESP_OK; i++)
    {
        esp_err = nvs_erase_key(handle, sta_cred_keys[i]);
        if(esp_err == ESP_ERR_NVS_NOT_FOUND)
        {
            esp_err = ESP_OK; 
        }
        else if(esp_err != ESP_OK)
        {
            ESP_LOGE(TAG, "unable to delete '%s': %s -- check for a corrupted NVS partition",
                    sta_cred_keys[i], esp_err_to_name(esp_err));
        }
    }
    if(esp_err == ESP_OK)
    {
        esp_err = nvs_commit(handle);
        if(esp_err != ESP_OK)
        {
            ESP_LOGE(TAG, "nvs_commit failed: %s", esp_err_to_name(esp_err));
        }
    }
    nvs_close(handle); 
    if(esp_err == ESP_OK)
    {
        ESP_LOGI(TAG, "cleared NVS credentials");
    }
    return esp_err;
}
