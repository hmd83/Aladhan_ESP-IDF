# ESP-IDF Prayer Times Component

A component for ESP32 that provides Islamic prayer times calculations using the Aladhan API. This component allows you to fetch prayer times for any location with various calculation methods and configurations.

## Features

- Get accurate prayer times for any location worldwide
- Support for multiple calculation methods (Makkah, ISNA, Egypt, etc.)
- Hijri date conversion included
- Configurable prayer time adjustments
- Built-in retry mechanism for failed requests
- Customizable timeout settings
- Support for various schools of thought
- Midnight mode configuration
- Latitude adjustment methods

## Installation

### As a Component

1. Create a `components` directory in your project if it doesn't exist:
```bash
mkdir -p components
cd components
```

2. Clone this repository:
```bash
git clone https://github.com/yourusername/prayer-times-esp-idf.git
```

### Configuration

The component can be configured through menuconfig:

```bash
idf.py menuconfig
```

Navigate to `Component config → Prayer Times Configuration` to configure:
- Maximum retry attempts for HTTP requests
- HTTP request timeout in milliseconds

## Usage

```c
#include "prayer_times.h"

// Initialize the component
esp_err_t ret = prayer_times_init();

// Configure prayer times
prayer_times_config_t config = {
    .latitude = 21.422510,  // Mecca coordinates
    .longitude = 39.826168,
    .method = PRAYER_METHOD_MAKKAH,
    .timezone = 3,
    .adjustment_days = 0,
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
strcpy(config.school, "Shafi");
strcpy(config.midnightMode, "Standard");
strcpy(config.latitudeAdjustmentMethod, "AngleBasedMethod");

// Get prayer times
prayer_times_t times;
ret = prayer_times_get(&config, 2024, 1, 1, &times);
if (ret == ESP_OK) {
    printf("Fajr: %s\n", times.fajr);
    printf("Sunrise: %s\n", times.sunrise);
    printf("Dhuhr: %s\n", times.dhuhr);
    printf("Asr: %s\n", times.asr);
    printf("Maghrib: %s\n", times.maghrib);
    printf("Isha: %s\n", times.isha);
    
    // Hijri date
    printf("Hijri: %d/%d/%d\n", 
           times.hijri.day,
           times.hijri.month,
           times.hijri.year);
}
```

## Available Prayer Methods

- `PRAYER_METHOD_SHIA_ITHNA_ASHARI`
- `PRAYER_METHOD_KARACHI`
- `PRAYER_METHOD_ISNA`
- `PRAYER_METHOD_MWL`
- `PRAYER_METHOD_MAKKAH`
- `PRAYER_METHOD_EGYPT`
- `PRAYER_METHOD_TEHRAN`
- `PRAYER_METHOD_GULF`
- `PRAYER_METHOD_KUWAIT`
- `PRAYER_METHOD_QATAR`
- `PRAYER_METHOD_SINGAPORE`
- `PRAYER_METHOD_FRANCE`
- `PRAYER_METHOD_TURKEY`
- `PRAYER_METHOD_RUSSIA`
- `PRAYER_METHOD_MOONSIGHTING`

## Dependencies

- esp_http_client
- json
- esp_wifi
- nvs_flash

## Error Handling

The component uses ESP-IDF's error handling system. All functions return `esp_err_t` with the following possible values:

- `ESP_OK`: Operation completed successfully
- `ESP_FAIL`: Generic failure
- `ESP_ERR_INVALID_ARG`: Invalid arguments
- `ESP_ERR_NO_MEM`: Memory allocation failed

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- [Aladhan API](https://aladhan.com/prayer-times-api) for providing the prayer times data
- ESP-IDF framework and community