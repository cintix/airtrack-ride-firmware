#ifndef UBX_READER_H
#define UBX_READER_H

#include <stdint.h>
#include "models/UbxPacket.h"

class UbxReader
{
public:
    void begin();

    void update();

    bool hasPacket();

    UbxPacket getPacket();

private:
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
};

#endif