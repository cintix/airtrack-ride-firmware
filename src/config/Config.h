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

#define GPS_UART_RX_PIN 20
#define GPS_UART_TX_PIN 21

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

/*
    Storage buffer configuration
*/

#define TRACK_BUFFER_SIZE 10

#endif