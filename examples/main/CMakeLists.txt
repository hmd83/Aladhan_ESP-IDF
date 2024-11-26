idf_component_register(
  SRCS "main.c"
  INCLUDE_DIRS "."
  REQUIRES prayer_times esp_wifi nvs_flash esp_http_client
)
