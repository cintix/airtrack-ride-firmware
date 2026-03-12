# AirTrack Ride Firmware

AirTrack Ride is an open-source bicycle computer firmware designed to record GPS rides and export them for route sharing and analysis.

The project focuses on:

* deterministic embedded architecture
* clear and maintainable code
* low resource usage
* full control of the GPS data pipeline

The firmware runs on an **ESP32-C3** and uses a **u-blox GPS module (e.g. NEO-6M)**.

---

# Architecture Overview

The system uses a simple and deterministic scheduler loop.

```
GPS → Parser → Application → Storage → Client
```

Main loop:

```
gps.update();
application.update();
storage.update();
client.update();
```

Each module has a single responsibility.

| Module      | Responsibility                      |
| ----------- | ----------------------------------- |
| gps         | read GPS data and produce GpsRecord |
| application | ride tracking and filtering         |
| storage     | write ride data to filesystem       |
| client      | export and sync                     |

---

# GPS Protocol

AirTrack Ride uses the **u-blox UBX binary protocol** instead of the traditional NMEA ASCII protocol.

The firmware listens for the **UBX-NAV-PVT message**, which provides all required navigation data in a single packet.

---

# Why UBX Instead of NMEA

NMEA is a legacy ASCII protocol designed in the 1980s for marine equipment and serial terminals.

Example NMEA sentence:

```
$GPRMC,092204.999,A,4250.5589,N,07106.8817,W,0.02,31.66,200520,,,A*77
```

While easy to read, NMEA has several disadvantages for embedded systems:

* ASCII parsing
* multiple sentences per navigation solution
* duplicated data
* larger bandwidth

UBX solves these problems by using a compact binary protocol.

Example UBX packet structure:

```
SYNC1 SYNC2 CLASS ID LENGTH_L LENGTH_H PAYLOAD CK_A CK_B
```

Advantages of UBX:

| Feature   | UBX                       | NMEA               |
| --------- | ------------------------- | ------------------ |
| Format    | binary                    | ASCII              |
| Parsing   | simple state machine      | string parsing     |
| Messages  | single navigation message | multiple sentences |
| Bandwidth | lower                     | higher             |
| CPU usage | lower                     | higher             |

---

# UBX Message Used

The firmware uses:

```
UBX-NAV-PVT
```

This message provides:

* latitude
* longitude
* altitude
* ground speed
* heading
* satellite count
* fix type
* accuracy

All navigation data is received in **one message per navigation cycle**.

---

# GPS Update Rate

The firmware is designed to support multiple GPS update rates.

Default:

```
1 Hz
```

Supported:

```
1–5 Hz
```

The rate can be configured in:

```
Config.h
```

Example:

```
#define GPS_UPDATE_RATE_HZ 1
```

---

# Ride Logging Strategy

GPS updates may occur more frequently than ride logging.

Typical configuration:

| Function       | Rate               |
| -------------- | ------------------ |
| GPS updates    | 1–5 Hz             |
| display update | 1 Hz               |
| ride logging   | every 5–10 seconds |

This reduces storage usage while maintaining accurate ride tracks.

---

# Hardware

Tested hardware:

* ESP32-C3 Mini
* u-blox NEO-6M GPS module

Connections:

```
GPS TX → ESP32 RX
GPS RX → ESP32 TX
VCC → 3.3V
GND → GND
```

---

# Goals of the Project

AirTrack Ride aims to be:

* understandable
* hackable
* modular
* efficient

The project is intentionally designed to avoid heavy frameworks or unnecessary abstractions.

---

# License

MIT License
