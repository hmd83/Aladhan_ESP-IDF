#include "prayer_times.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "prayer_times";

#define MAX_HTTP_RESPONSE_SIZE 8192
#define BASE_URL "http://api.aladhan.com/v1/timings"

static char *http_response_buffer = NULL;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
  switch (evt->event_id) {
      case HTTP_EVENT_ON_DATA:
          if (!http_response_buffer) {
              http_response_buffer = malloc(MAX_HTTP_RESPONSE_SIZE);
              if (!http_response_buffer) {
                  ESP_LOGE(TAG, "Failed to allocate memory for HTTP response");
                  return ESP_ERR_NO_MEM;
              }
              memset(http_response_buffer, 0, MAX_HTTP_RESPONSE_SIZE);
          }
          if (evt->data_len < (MAX_HTTP_RESPONSE_SIZE - strlen(http_response_buffer))) {
              strncat(http_response_buffer, (char*)evt->data, evt->data_len);
          }
          break;
      case HTTP_EVENT_ON_FINISH:
          break;
      case HTTP_EVENT_DISCONNECTED:
          if (http_response_buffer) {
              free(http_response_buffer);
              http_response_buffer = NULL;
          }
          break;
      default:
          break;
  }
  return ESP_OK;
}

esp_err_t prayer_times_init(void)
{
  return ESP_OK;
}
static esp_err_t parse_hijri_date(cJSON *date, hijri_date_t *hijri)
{
  if (!date || !hijri) return ESP_FAIL;

  // Get day directly
  cJSON *day = cJSON_GetObjectItem(date, "day");
  if (day && day->valuestring) {
      hijri->day = atoi(day->valuestring);
  }

  // Get month number from the month object
  cJSON *month = cJSON_GetObjectItem(date, "month");
  if (month && cJSON_IsObject(month)) {
      cJSON *month_number = cJSON_GetObjectItem(month, "number");
      if (month_number && cJSON_IsNumber(month_number)) {
          hijri->month = month_number->valueint;
      }
  }

  // Get year directly
  cJSON *year = cJSON_GetObjectItem(date, "year");
  if (year && year->valuestring) {
      hijri->year = atoi(year->valuestring);
  }

  return ESP_OK;
}
static esp_err_t parse_prayer_times(const char *json_response, prayer_times_t *times)
{
  cJSON *root = cJSON_Parse(json_response);
  if (!root) {
      ESP_LOGE(TAG, "Failed to parse JSON");
      return ESP_FAIL;
  }

  cJSON *data = cJSON_GetObjectItem(root, "data");
  if (!data) {
      cJSON_Delete(root);
      return ESP_FAIL;
  }

  // Parse timings
  cJSON *timings = cJSON_GetObjectItem(data, "timings");
  if (timings) {
      cJSON *time;

      time = cJSON_GetObjectItem(timings, "Fajr");
      if (time && time->valuestring) strncpy(times->fajr, time->valuestring, 9);

      time = cJSON_GetObjectItem(timings, "Sunrise");
      if (time && time->valuestring) strncpy(times->sunrise, time->valuestring, 9);

      time = cJSON_GetObjectItem(timings, "Dhuhr");
      if (time && time->valuestring) strncpy(times->dhuhr, time->valuestring, 9);

      time = cJSON_GetObjectItem(timings, "Asr");
      if (time && time->valuestring) strncpy(times->asr, time->valuestring, 9);

      time = cJSON_GetObjectItem(timings, "Sunset");
      if (time && time->valuestring) strncpy(times->sunset, time->valuestring, 9);

      time = cJSON_GetObjectItem(timings, "Maghrib");
      if (time && time->valuestring) strncpy(times->maghrib, time->valuestring, 9);

      time = cJSON_GetObjectItem(timings, "Isha");
      if (time && time->valuestring) strncpy(times->isha, time->valuestring, 9);

      time = cJSON_GetObjectItem(timings, "Imsak");
      if (time && time->valuestring) strncpy(times->imsak, time->valuestring, 9);

      time = cJSON_GetObjectItem(timings, "Midnight");
      if (time && time->valuestring) strncpy(times->midnight, time->valuestring, 9);

      time = cJSON_GetObjectItem(timings, "Firstthird");
      if (time && time->valuestring) strncpy(times->firstthird, time->valuestring, 9);

      time = cJSON_GetObjectItem(timings, "Lastthird");
      if (time && time->valuestring) strncpy(times->lastthird, time->valuestring, 9);
  }

  // Parse Hijri date
  cJSON *date = cJSON_GetObjectItem(data, "date");
  if (date) {
      cJSON *hijri = cJSON_GetObjectItem(date, "hijri");
      if (hijri) {
          parse_hijri_date(hijri, &times->hijri);
      }
  }

  cJSON_Delete(root);
  return ESP_OK;
}

esp_err_t prayer_times_get(const prayer_times_config_t *config, 
                        int year, int month, int day,
                        prayer_times_t *times)
{
  if (!config || !times) {
      return ESP_ERR_INVALID_ARG;
  }

  // Build URL with all parameters
  char url[512];
  snprintf(url, sizeof(url), 
           "%s/%d-%d-%d?latitude=%.6f&longitude=%.6f&method=%d"
           "&adjustment=%d&tune=0,0,0,0,0,0,0,0&school=%s"
           "&midnightMode=%s&latitudeAdjustmentMethod=%s"
           "&tune=%d,%d,%d,%d,%d,%d,%d,%d,%d",
           BASE_URL, year, month, day,
           config->latitude, config->longitude,
           config->method, config->adjustment_days,
           config->school, config->midnightMode,
           config->latitudeAdjustmentMethod,
           config->tune_imsak, config->tune_fajr,
           config->tune_sunrise, config->tune_dhuhr,
           config->tune_asr, config->tune_sunset,
           config->tune_maghrib, config->tune_isha,
           config->tune_midnight);

  esp_http_client_config_t http_config = {
      .url = url,
      .method = HTTP_METHOD_GET,
      .event_handler = http_event_handler,
      .timeout_ms = CONFIG_PRAYER_TIMES_TIMEOUT_MS,
  };

  esp_http_client_handle_t client = esp_http_client_init(&http_config);
  if (!client) {
      return ESP_FAIL;
  }

  esp_err_t err = ESP_FAIL;
  int retry_count = 0;

  while (retry_count < CONFIG_PRAYER_TIMES_MAX_RETRY) {
      err = esp_http_client_perform(client);
      if (err == ESP_OK) {
          int status_code = esp_http_client_get_status_code(client);
          if (status_code == 200 && http_response_buffer) {
              err = parse_prayer_times(http_response_buffer, times);
              break;
          }
      }
      retry_count++;
      vTaskDelay(pdMS_TO_TICKS(1000));
  }

  if (http_response_buffer) {
      free(http_response_buffer);
      http_response_buffer = NULL;
  }

  esp_http_client_cleanup(client);
  return err;
}