#include "UbxReader.h"
#include <Arduino.h>
#include "../config/Config.h"

HardwareSerial gpsSerial(1);

void sendUBX(uint8_t cls, uint8_t id, const uint8_t *payload, uint16_t len)
{
    uint8_t ckA = 0, ckB = 0;

    gpsSerial.write(0xB5);
    gpsSerial.write(0x62);

    gpsSerial.write(cls);
    gpsSerial.write(id);

    gpsSerial.write(len & 0xFF);
    gpsSerial.write(len >> 8);

    ckA += cls;
    ckB += ckA;
    ckA += id;
    ckB += ckA;
    ckA += (len & 0xFF);
    ckB += ckA;
    ckA += (len >> 8);
    ckB += ckA;

    for (uint16_t i = 0; i < len; i++)
    {
        gpsSerial.write(payload[i]);
        ckA += payload[i];
        ckB += ckA;
    }

    gpsSerial.write(ckA);
    gpsSerial.write(ckB);
}

void disableRMC()
{
    uint8_t payload[] = {
        0xF0, 0x04, // RMC
        0x00, 0x00, 0x00, 0x00, 0x00};

    sendUBX(0x06, 0x01, payload, sizeof(payload));
}

void setRate1Hz()
{
    uint8_t payload[] = {
        0xE8, 0x03, // measRate = 1000 ms
        0x01, 0x00, // navRate = 1
        0x01, 0x00  // timeRef = GPS time
    };

    sendUBX(0x06, 0x08, payload, sizeof(payload));
}
void enablePOSLLH()
{
    uint8_t payload[] = {
        0x01, 0x02, // NAV-POSLLH
        0x01,       // UART1
        0x01,       // UART2
        0x01,       // USB
        0x01,       // SPI
        0x00};

    sendUBX(0x06, 0x01, payload, sizeof(payload));
}

void enableVELNED()
{
    const uint8_t payload[] = {
        0x01, 0x12, // NAV-VELNED
        0x01,       // UART1
        0x01,
        0x01,
        0x01,
        0x00};

    sendUBX(0x06, 0x01, payload, sizeof(payload));
}
void disableNMEAOutput()
{
    uint8_t payload[] = {
        0xF0,
        0x00,
        0,
        0,
        0,
        0,
        0, // GGA
    };
    sendUBX(0x06, 0x01, payload, sizeof(payload));

    uint8_t msgs[] = {0x01, 0x02, 0x03, 0x04, 0x05};

    for (int i = 0; i < 5; i++)
    {
        uint8_t p[] = {
            0xF0, msgs[i],
            0, 0, 0, 0, 0};
        sendUBX(0x06, 0x01, p, sizeof(p));
        delay(50);
    }
}

void resetGPS()
{
    uint8_t payload[] = {
        0xFF, 0xFF, // hardware reset
        0x00, 0x00};

    sendUBX(0x06, 0x04, payload, sizeof(payload));
}

void UbxReader::begin()
{
    gpsSerial.begin(9600, SERIAL_8N1, 7, 6);

    delay(1000);

    enablePOSLLH();
    delay(200);

    enableVELNED();
    delay(200);

    setRate1Hz();
    delay(200);

    disableNMEAOutput(); // 🔥 sidst!
}
void UbxReader::update()
{
    while (gpsSerial.available())
    {
        uint8_t incomingByte = gpsSerial.read();
        processIncomingByte(incomingByte);
    }
}

bool UbxReader::hasPacket()
{
    return packetAvailable;
}

UbxPacket UbxReader::getPacket()
{
    packetAvailable = false;
    return packet;
}

void UbxReader::updateChecksum(uint8_t incomingByte)
{
    checksumByteA = checksumByteA + incomingByte;
    checksumByteB = checksumByteB + checksumByteA;
}

void UbxReader::processIncomingByte(uint8_t incomingByte)
{
    switch (parserState)
    {
    case ParserState::WaitingForSyncByte1:

        if (incomingByte == 0xB5)
            parserState = ParserState::WaitingForSyncByte2;

        break;

    case ParserState::WaitingForSyncByte2:

        if (incomingByte == 0x62)
        {
            checksumByteA = 0;
            checksumByteB = 0;

            parserState = ParserState::ReadingMessageClass;
        }
        else
            parserState = ParserState::WaitingForSyncByte1;

        break;

    case ParserState::ReadingMessageClass:

        messageClass = incomingByte;
        updateChecksum(incomingByte);

        parserState = ParserState::ReadingMessageId;
        break;

    case ParserState::ReadingMessageId:

        messageId = incomingByte;
        updateChecksum(incomingByte);

        parserState = ParserState::ReadingPayloadLengthLow;
        break;

    case ParserState::ReadingPayloadLengthLow:

        payloadLength = incomingByte;
        updateChecksum(incomingByte);

        parserState = ParserState::ReadingPayloadLengthHigh;
        break;

    case ParserState::ReadingPayloadLengthHigh:

        payloadLength |= (incomingByte << 8);
        updateChecksum(incomingByte);

        payloadBytesRead = 0;

        parserState = ParserState::ReadingPayloadBytes;
        break;

    case ParserState::ReadingPayloadBytes:

        payloadBuffer[payloadBytesRead++] = incomingByte;
        updateChecksum(incomingByte);

        if (payloadBytesRead >= payloadLength)
            parserState = ParserState::ReadingChecksumByteA;

        break;

    case ParserState::ReadingChecksumByteA:

        if (incomingByte == checksumByteA)
            parserState = ParserState::ReadingChecksumByteB;
        else
            parserState = ParserState::WaitingForSyncByte1;

        break;

    case ParserState::ReadingChecksumByteB:

        if (incomingByte == checksumByteB)
            finalizePacket();

        parserState = ParserState::WaitingForSyncByte1;
        break;
    }
}

void UbxReader::finalizePacket()
{
    packet.messageClass = messageClass;
    packet.messageId = messageId;
    packet.payloadLength = payloadLength;

    for (uint16_t i = 0; i < payloadLength; i++)
        packet.payload[i] = payloadBuffer[i];

    packetAvailable = true;
}