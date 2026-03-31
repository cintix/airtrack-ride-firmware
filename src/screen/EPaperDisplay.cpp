#include "EPaperDisplay.h"
#include <Arduino.h>

void EPaperDisplay::begin()
{
    // Dummy setup placeholder for upcoming e-paper hardware integration.
    Serial.println("EPaperDisplay: dummy init");
}

void EPaperDisplay::updateDisplay(const DisplayRecord &record)
{
    // Dummy update placeholder:
    // Replace this with your panel driver draw + refresh + sleep sequence.
    Serial.print("EPaperDisplay: speed=");
    Serial.print(record.SpeedKm, 1);
    Serial.print(" km/t, distance=");
    Serial.print(record.distanceKm, 2);
    Serial.println(" km");
}
