idf_component_register(
  SRCS "prayer_times.c"
  INCLUDE_DIRS "include"
  REQUIRES esp_http_client json esp_wifi nvs_flash  
)
