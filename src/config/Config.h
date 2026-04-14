#ifndef CONFIG_H
#define CONFIG_H
/*
    GPS DEBUG CONFIGURATION
*/

#define DEBUG_GPS_PRINT_INTERVAL_MS 5000
#define DEBUG_GPS_LED_PIN 7

/*
    GPS UART configuration
*/

#define GPS_UART_RX_PIN 7
#define GPS_UART_TX_PIN 6

#define GPS_BAUD_RATE 9600

/*
    GPS navigation update rate
*/

#define GPS_UPDATE_RATE_HZ 1

/*
    UBX parser configuration
*/

#define UBX_MAX_PAYLOAD_LENGTH 100

/*
    GPS dynamic platform model
*/

#define GPS_ENABLE_DYNAMIC_PLATFORM_MODEL 1

/*
    Platform model values (u-blox NAV5)
*/

#define GPS_PLATFORM_PORTABLE 0
#define GPS_PLATFORM_STATIONARY 2
#define GPS_PLATFORM_PEDESTRIAN 3
#define GPS_PLATFORM_AUTOMOTIVE 4
#define GPS_PLATFORM_SEA 5
#define GPS_PLATFORM_AIRBORNE_1G 6
#define GPS_PLATFORM_AIRBORNE_2G 7
#define GPS_PLATFORM_AIRBORNE_4G 8
#define GPS_PLATFORM_WRIST 9
#define GPS_PLATFORM_BIKE 10

/*
    Selected platform model
*/

#define GPS_PLATFORM_MODEL GPS_PLATFORM_BIKE

/*
    Ride tracking configuration
*/

#define TRACK_MIN_DISTANCE_METERS 25
#define TRACK_MIN_INTERVAL_SECONDS 5
#define MOVING_STOP_SPEED_THRESHOLD_KMH 0.7f
#define MOVING_STOP_DELAY_SECONDS 3

/*
    Storage buffer configuration
*/

#define TRACK_BUFFER_SIZE 10

/*
    Input configuration
*/

#define INPUT_TOGGLE_PIN 10
#define INPUT_DEBOUNCE_MS 40

/*
    Main loop power optimization
*/

#define MAIN_LOOP_IDLE_DELAY_MS 20

/*
    Display driver selection
*/

#define DISPLAY_DRIVER_OLED 0
#define DISPLAY_DRIVER_EPAPER 1
#define DISPLAY_DRIVER_TYPE DISPLAY_DRIVER_OLED

/*
    Screen refresh policy (applies to all display drivers)
*/

#define SCREEN_MIN_REFRESH_INTERVAL_MS 500
#define SCREEN_MAX_REFRESH_INTERVAL_MS 2000
#define SCREEN_SPEED_DELTA_KMH 0.5f
#define SCREEN_DISTANCE_DELTA_KM 0.01f
#define SCREEN_TIME_DELTA_SECONDS 1.0f

/*
    Barometric sensor (BME280) configuration
*/

#define BAROMETRIC_ENABLED 1
#define BAROMETRIC_I2C_SDA_PIN 8
#define BAROMETRIC_I2C_SCL_PIN 9
#define BAROMETRIC_I2C_ADDRESS 0x76
#define BAROMETRIC_READ_INTERVAL_MS 1000
#define BAROMETRIC_SEA_LEVEL_PRESSURE_HPA 1013.25f

/*
    WiFi / Client configuration
*/

#define WIFI_AP_SSID_PREFIX "AirTrack"
#define WIFI_AP_PASSWORD "airtrack123"
#define WIFI_AP_OPEN_NETWORK 0
#define WIFI_AP_CHANNEL 6
#define WIFI_AP_MAX_CLIENTS 4
#define WIFI_AP_IP_OCTET_1 192
#define WIFI_AP_IP_OCTET_2 168
#define WIFI_AP_IP_OCTET_3 4
#define WIFI_AP_IP_OCTET_4 1
#define WIFI_AP_RESET_DELAY_MS 200UL
#define WIFI_AP_RETRY_DELAY_MS 500UL

// Keep empty to skip STA connect until credentials are configured via setup UI.
#define WIFI_STA_SSID ""
#define WIFI_STA_PASSWORD ""
#define WIFI_STA_CONNECT_TIMEOUT_MS 10000UL
#define WIFI_STA_RETRY_BASE_DELAY_MS 1000UL
#define WIFI_STA_RETRY_MAX_DELAY_MS 60000UL
#define WIFI_STA_RETRY_JITTER_MS 400UL
#define WIFI_STA_STATUS_LOG_INTERVAL_MS 10000UL

#endif
