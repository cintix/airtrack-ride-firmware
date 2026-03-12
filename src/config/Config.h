#ifndef CONFIG_H
#define CONFIG_H

/*
    GPS UART configuration
*/

#define GPS_UART_RX_PIN 20
#define GPS_UART_TX_PIN 21

#define GPS_BAUD_RATE 9600

/*
    GPS navigation update rate

    Supported values:
    1 Hz  (default and most stable)
    up to 5 Hz on NEO-6M
*/

#define GPS_UPDATE_RATE_HZ 1

/*
    UBX parser configuration
*/

#define UBX_MAX_PAYLOAD_LENGTH 100

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