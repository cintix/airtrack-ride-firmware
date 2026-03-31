#include "UbxReader.h"
#include "../config/Config.h"
#include "models/GpsProtocolTypes.h"

void UbxReader::sendUbxMessage(uint8_t messageClass, uint8_t messageId, const uint8_t* payload, uint16_t length)
{
    uint8_t ckA = 0, ckB = 0;

    serial->write(0xB5);
    serial->write(0x62);

    serial->write(messageClass);
    serial->write(messageId);

    serial->write(length & 0xFF);
    serial->write(length >> 8);

    ckA += messageClass;
    ckB += ckA;
    ckA += messageId;
    ckB += ckA;
    ckA += (length & 0xFF);
    ckB += ckA;
    ckA += (length >> 8);
    ckB += ckA;

    for (uint16_t i = 0; i < length; i++)
    {
        serial->write(payload[i]);
        ckA += payload[i];
        ckB += ckA;
    }

    serial->write(ckA);
    serial->write(ckB);
}

void UbxReader::enableNavigationMessage(uint8_t messageId)
{
    uint8_t payload[] = {
        GpsProtocol::MessageClass::Navigation,
        messageId,
        1, 1, 1, 1, 0
    };

    sendUbxMessage(
        GpsProtocol::MessageClass::Configuration,
        GpsProtocol::ConfigurationMessageId::SetMessageRate,
        payload,
        sizeof(payload));
}

void UbxReader::setUpdateRate(uint16_t milliseconds)
{
    uint8_t payload[] = {
        (uint8_t)(milliseconds & 0xFF),
        (uint8_t)(milliseconds >> 8),
        0x01, 0x00,
        0x01, 0x00
    };

    sendUbxMessage(
        GpsProtocol::MessageClass::Configuration,
        GpsProtocol::ConfigurationMessageId::SetRate,
        payload,
        sizeof(payload));
}


void UbxReader::disableNmeaOutput()
{
    uint8_t payload[] = {
        0xF0, 0x00,
        0, 0, 0, 0, 0
    };

    sendUbxMessage(
        GpsProtocol::MessageClass::Configuration,
        GpsProtocol::ConfigurationMessageId::SetMessageRate,
        payload,
        sizeof(payload));

    uint8_t messages[] = {0x01, 0x02, 0x03, 0x04, 0x05};

    for (int i = 0; i < 5; i++)
    {
        uint8_t p[] = {
            0xF0, messages[i],
            0, 0, 0, 0, 0
        };

        sendUbxMessage(
            GpsProtocol::MessageClass::Configuration,
            GpsProtocol::ConfigurationMessageId::SetMessageRate,
            p,
            sizeof(p));

        delay(50);
    }
}

void UbxReader::begin()
{
    serial = &Serial1;

    serial->begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_UART_RX_PIN, GPS_UART_TX_PIN);

    delay(500);

    enableNavigationMessage(GpsProtocol::NavigationMessageId::PositionLatitudeLongitudeHeight);
    delay(200);

    enableNavigationMessage(GpsProtocol::NavigationMessageId::VelocityNorthEastDown);
    delay(200);

    enableNavigationMessage(GpsProtocol::NavigationMessageId::NavigationSolution);
    delay(200);

    enableNavigationMessage(GpsProtocol::NavigationMessageId::TimeUtc);
    delay(200);

    setUpdateRate(1000);
    delay(200);

    disableNmeaOutput();
}

void UbxReader::update()
{
    if (serial == nullptr)
        return;

    while (serial->available())
    {
        uint8_t incomingByte = serial->read();
        processIncomingByte(incomingByte);
    }
}

bool UbxReader::hasPacket() const
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
    checksumByteA += incomingByte;
    checksumByteB += checksumByteA;
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
        {
            parserState = ParserState::WaitingForSyncByte1;
        }

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

        if (payloadLength > sizeof(payloadBuffer))
        {
            parserState = ParserState::WaitingForSyncByte1;
            return;
        }

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
