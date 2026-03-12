#include "UbxReader.h"
#include <Arduino.h>
#include "../config/Config.h"

void UbxReader::begin()
{
    Serial1.begin(
        GPS_BAUD_RATE,
        SERIAL_8N1,
        GPS_UART_RX_PIN,
        GPS_UART_TX_PIN);
}

void UbxReader::update()
{
    while (Serial1.available())
    {
        uint8_t incomingByte = Serial1.read();

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