#pragma once

#include "esp_err.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Hijri date structure
*/
typedef struct {
  int day;
  int month;
  int year;
} hijri_date_t;

/**
* @brief Prayer times data structure
*/
typedef struct {
  char fajr[10];
  char sunrise[10];
  char dhuhr[10];
  char asr[10];
  char sunset[10];
  char maghrib[10];
  char isha[10];
  char imsak[10];
  char midnight[10];
  char firstthird[10];
  char lastthird[10];
  hijri_date_t hijri;
} prayer_times_t;

/**
* @brief Method calculation options
*/
typedef enum {
  PRAYER_METHOD_SHIA_ITHNA_ASHARI = 0,
  PRAYER_METHOD_KARACHI,
  PRAYER_METHOD_ISNA,
  PRAYER_METHOD_MWL,
  PRAYER_METHOD_MAKKAH,
  PRAYER_METHOD_EGYPT,
  PRAYER_METHOD_TEHRAN,
  PRAYER_METHOD_GULF,
  PRAYER_METHOD_KUWAIT,
  PRAYER_METHOD_QATAR,
  PRAYER_METHOD_SINGAPORE,
  PRAYER_METHOD_FRANCE,
  PRAYER_METHOD_TURKEY,
  PRAYER_METHOD_RUSSIA,
  PRAYER_METHOD_MOONSIGHTING,
} prayer_method_t;

/**
* @brief Prayer times configuration
*/
typedef struct {
  double latitude;
  double longitude;
  int timezone;
  prayer_method_t method;
  int adjustment_days;
  char school[20];
  char midnightMode[20];
  char latitudeAdjustmentMethod[20];
  int tune_imsak;
  int tune_fajr;
  int tune_sunrise;
  int tune_dhuhr;
  int tune_asr;
  int tune_sunset;
  int tune_maghrib;
  int tune_isha;
  int tune_midnight;
} prayer_times_config_t;

/**
* @brief Initialize prayer times component
* 
* @return esp_err_t ESP_OK on success
*/
esp_err_t prayer_times_init(void);

/**
* @brief Get prayer times for specific date
* 
* @param config Prayer times configuration
* @param year Year (e.g., 2024)
* @param month Month (1-12)
* @param day Day (1-31)
* @param times Pointer to prayer_times_t structure to store results
* @return esp_err_t ESP_OK on success
*/
esp_err_t prayer_times_get(const prayer_times_config_t *config, 
                        int year, int month, int day,
                        prayer_times_t *times);

#ifdef __cplusplus
}
#endif