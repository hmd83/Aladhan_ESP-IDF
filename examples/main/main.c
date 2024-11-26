#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "prayer_times.h"

static const char *TAG = "main";

/* WiFi configuration */
#define WIFI_SSID      "FRITZ!Box 7590 HX"
#define WIFI_PASSWORD  "44661640478758102078"
#define WIFI_MAXIMUM_RETRY  5

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
* - we are connected to the AP with an IP
* - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                       int32_t event_id, void* event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
      esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
      if (s_retry_num < WIFI_MAXIMUM_RETRY) {
          esp_wifi_connect();
          s_retry_num++;
          ESP_LOGI(TAG, "retry to connect to the AP");
      } else {
          xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
      }
      ESP_LOGI(TAG,"connect to the AP fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
      ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
      ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
      s_retry_num = 0;
      xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

void wifi_init_sta(void)
{
  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                    ESP_EVENT_ANY_ID,
                                                    &event_handler,
                                                    NULL,
                                                    &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                    IP_EVENT_STA_GOT_IP,
                                                    &event_handler,
                                                    NULL,
                                                    &instance_got_ip));

  wifi_config_t wifi_config = {
      .sta = {
          .ssid = WIFI_SSID,
          .password = WIFI_PASSWORD,
          /* Setting a password implies station will connect to all security modes including WEP/WPA.
           * However these modes are deprecated and not advisable to be used. Incase your Access point
           * doesn't support WPA2, these mode can be enabled by commenting below line */
          .threshold.authmode = WIFI_AUTH_WPA2_PSK,
      },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
  ESP_ERROR_CHECK(esp_wifi_start() );

  ESP_LOGI(TAG, "wifi_init_sta finished.");

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
   * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
          pdFALSE,
          pdFALSE,
          portMAX_DELAY);

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
   * happened. */
  if (bits & WIFI_CONNECTED_BIT) {
      ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
               WIFI_SSID, WIFI_PASSWORD);
  } else if (bits & WIFI_FAIL_BIT) {
      ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
               WIFI_SSID, WIFI_PASSWORD);
  } else {
      ESP_LOGE(TAG, "UNEXPECTED EVENT");
  }
}
void print_prayer_times(prayer_times_t *times)
{
  ESP_LOGI(TAG, "Prayer Times:");
  ESP_LOGI(TAG, "Imsak:    %s", times->imsak);
  ESP_LOGI(TAG, "Fajr:     %s", times->fajr);
  ESP_LOGI(TAG, "Sunrise:  %s", times->sunrise);
  ESP_LOGI(TAG, "Dhuhr:    %s", times->dhuhr);
  ESP_LOGI(TAG, "Asr:      %s", times->asr);
  ESP_LOGI(TAG, "Sunset:   %s", times->sunset);
  ESP_LOGI(TAG, "Maghrib:  %s", times->maghrib);
  ESP_LOGI(TAG, "Isha:     %s", times->isha);
  ESP_LOGI(TAG, "Midnight: %s", times->midnight);

  ESP_LOGI(TAG, "\nHijri Date: %02d-%02d-%04d", 
           times->hijri.day,
           times->hijri.month,
           times->hijri.year);
}
void app_main(void)
{
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
  wifi_init_sta();

  // Initialize prayer times component
  ESP_ERROR_CHECK(prayer_times_init());

  // Configure prayer times settings
  prayer_times_config_t config = {
      .latitude = 48.673194,  // Mecca latitude
      .longitude = 8.2475241, // Mecca longitude
      .timezone = 0,          // UTC+3
      .method = PRAYER_METHOD_MWL,
      .adjustment_days = 0,
      .school = "SHAFI",
      .midnightMode = "STANDARD",
      .latitudeAdjustmentMethod = "ANGLE_BASED",
      .tune_imsak = 0,
      .tune_fajr = 0,
      .tune_sunrise = 0,
      .tune_dhuhr = 0,
      .tune_asr = 0,
      .tune_sunset = 0,
      .tune_maghrib = 0,
      .tune_isha = 0,
      .tune_midnight = 0
  };

  // Get prayer times
  prayer_times_t times;
  while (1) {
      esp_err_t err = prayer_times_get(&config, 2024, 11, 26, &times);
      if (err == ESP_OK) {
          print_prayer_times(&times);
      } else {
          ESP_LOGE(TAG, "Failed to get prayer times: %s", esp_err_to_name(err));
      }

      // Wait for 1 hour before next update
      vTaskDelay(pdMS_TO_TICKS(3600000));
  }
}