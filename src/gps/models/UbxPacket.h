#include <stdint.h>

#ifndef UBX_PACKET_H
#define UBX_PACKET_H

struct UbxPacket
{
    uint8_t messageClass;
    uint8_t messageId;

    uint16_t payloadLength;

    uint8_t payload[100];
};

#endif