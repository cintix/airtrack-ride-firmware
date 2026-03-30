#ifndef UBX_READER_H
#define UBX_READER_H

#include <stdint.h>
#include <Arduino.h>
#include "models/UbxPacket.h"

class UbxReader
{
public:
    void begin();
    void update();

    bool hasPacket() const;
    UbxPacket getPacket();

    void enableNavigationMessage(uint8_t messageId);
    void setUpdateRate(uint16_t milliseconds);

private:
    HardwareSerial* serial = nullptr;

    enum class ParserState
    {
        WaitingForSyncByte1,
        WaitingForSyncByte2,
        ReadingMessageClass,
        ReadingMessageId,
        ReadingPayloadLengthLow,
        ReadingPayloadLengthHigh,
        ReadingPayloadBytes,
        ReadingChecksumByteA,
        ReadingChecksumByteB
    };

    ParserState parserState = ParserState::WaitingForSyncByte1;

    uint8_t messageClass;
    uint8_t messageId;

    uint16_t payloadLength;
    uint16_t payloadBytesRead;

    uint8_t payloadBuffer[100];

    uint8_t checksumByteA;
    uint8_t checksumByteB;

    bool packetAvailable = false;

    UbxPacket packet;

    void updateChecksum(uint8_t incomingByte);
    void processIncomingByte(uint8_t incomingByte);
    void finalizePacket();

    void sendUbxMessage(uint8_t messageClass, uint8_t messageId, const uint8_t* payload, uint16_t length);
    void disableNmeaOutput();
};

#endif