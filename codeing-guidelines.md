# AirTrack Ride Coding Guidelines

This project prioritizes readability, clarity, and maintainability.

The goal is that a developer can understand the codebase quickly without guessing intent.

---

# Naming

Variable and function names must be descriptive.

Avoid abbreviations.

Preferred:

```
checksumByteA
payloadLength
payloadBytesRead
```

Avoid:

```
ckA
len
idx
```

Code should be readable without comments whenever possible.

---

# Modules

Each module should have a single responsibility.

Example modules:

```
gps
application
storage
client
```

Modules should not depend on each other unnecessarily.

---

# Data Structures

Prefer simple structs for data transport.

Example:

```
struct GpsRecord
{
    bool valid;

    double latitude;
    double longitude;

    float altitude;

    float speed;
    float heading;

    uint8_t satellites;
};
```

Avoid unnecessary classes or dynamic allocation.

---

# Memory

Avoid:

* dynamic allocation
* hidden buffers
* unnecessary copies

Prefer:

* stack memory
* static buffers
* deterministic execution

---

# Parser Design

All parsers must operate on streaming data.

Do not rely on full message buffering when unnecessary.

Preferred approach:

```
UART byte
→ parser state machine
→ decoded message
```

This ensures the system remains robust even with noisy serial data.

---

# Comments

Comments should explain **why**, not **what**.

Bad:

```
increment counter
```

Good:

```
Increment payload index to track how many bytes of the UBX payload have been received.
```

---

# Formatting

Indentation:

```
4 spaces
```

Braces always on new line.

Example:

```
if (condition)
{
    doSomething();
}
```

---

# Configuration

All tunable values must live in:

```
Config.h
```

Example:

```
#define GPS_UPDATE_RATE_HZ 1
#define TRACK_DISTANCE_THRESHOLD_METERS 25
```

Avoid magic numbers in the code.
