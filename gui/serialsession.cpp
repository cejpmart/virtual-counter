#include "serialsession.h"

constexpr int timeout = 1000;
constexpr int baudrate = 57600;

constexpr bool enableLogging = true;

void SerialSession::open(const char* filename) {
    numRxBytes = 0;
    totalRxBytes = 0;

    serialPort.reset(new QSerialPort(filename));
    serialPort->setBaudRate(baudrate, QSerialPort::AllDirections);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!serialPort->open(QIODevice::ReadWrite)) {
        auto error = (QString) filename + ": " + serialPort->errorString();
        throw std::runtime_error(qPrintable(error));
    }
}

bool SerialSession::awaitPacket(uint8_t* tag_out, uint8_t const** data_out, size_t* length_out) {
    QElapsedTimer et;
    et.start();

    while (et.elapsed() < timeout) {
        if (!serialPort->bytesAvailable())
            serialPort->waitForReadyRead(10);
        else {
            if (receivePacket(tag_out, data_out, length_out))
                return true;
            else
                QThread::msleep(10);        // this is stupid
        }
    }

    setTimeoutErrorFlag();
    return false;
}

bool SerialSession::receivePacket(uint8_t* tag_out, uint8_t const** data_out, size_t* length_out) {
    if (!serialPort)
        return false;

    uint8_t header[2];

    if (recvall(header, sizeof(header), false)) {
        auto length = header[1];

        receivedPacketData.resize(2 + length);
        auto rx = recvall(&receivedPacketData[0], 2 + length, true);

        if (enableLogging) {
            char buffer[1000];
            sprintf(buffer, "tag %02X, length %02X, rx +%d", header[0], header[1], (int)rx);
            qInfo("%s", buffer);
        }

        if (rx) {
            *tag_out = header[0];
            *length_out = header[1];
            *data_out = &receivedPacketData[2];

            if (enableLogging) {
                char dangerousSprintfBuffer[1000];
                sprintf(dangerousSprintfBuffer, "tag %02X, length %02X", *tag_out, (unsigned int) *length_out);

                if (length) {
                    strcat(dangerousSprintfBuffer, ", data: [");
                    for (size_t i = 0; i < length; i++)
                        sprintf(dangerousSprintfBuffer + strlen(dangerousSprintfBuffer), " %02X", receivedPacketData[2 + i]);
                    strcat(dangerousSprintfBuffer, "]");
                }
                qInfo("%s", dangerousSprintfBuffer);
            }

            return true;
        }
    }

    return false;
}

bool SerialSession::recvall(uint8_t* buffer, size_t count, bool removeFromBuffer) {
    if (numRxBytes < count) {
        rxBytes.resize(count);

        size_t need = count - numRxBytes;
        size_t got = serialPort->read((char*) &rxBytes[numRxBytes], need);

        if (enableLogging) {
            char dangerousSprintfBuffer[1000] = "in\t";

            for (size_t i = 0; i < got; i++)
                sprintf(dangerousSprintfBuffer + strlen(dangerousSprintfBuffer), " %02X", rxBytes[numRxBytes + i]);

            qInfo("%s", dangerousSprintfBuffer);
        }

        if (totalRxBytes == 0) {
            // Throw away spurious 0x00s caused by poor hardware design on Nucleo kits

            while (got && rxBytes[0] == 0) {
                rxBytes.erase(rxBytes.begin());
                got--;
            }
        }

        numRxBytes += got;
        totalRxBytes += got;
    }

    if (numRxBytes >= count) {
        memcpy(buffer, &rxBytes[0], count);

        if (removeFromBuffer) {
            rxBytes.erase(rxBytes.begin(), rxBytes.begin() + count);
            numRxBytes -= count;
        }

        return true;
    }

    return false;
}

bool SerialSession::sendPacket(uint8_t tag, const uint8_t* data, size_t length) {
    if (!serialPort || length > UINT8_MAX)
        return false;

    /*uint8_t buffer[258];
    buffer[0] = tag;
    buffer[1] = length;
    memcpy(buffer + 2, data, length);*/

    return this->write<uint8_t>(tag)
            && this->write<uint8_t>(length)
            && (length ? this->write(data, length) == length : true)
            //session->write(buffer, length + 2) == length + 2
            && serialPort->flush();
}

void SerialSession::setTimeoutErrorFlag() {
    qCritical("SerialSession::setTimeoutErrorFlag()");
}

size_t SerialSession::write(const uint8_t* bytes, size_t length) {
    if (enableLogging) {
        char dangerousSprintfBuffer[1000] = "out\t";

        for (size_t i = 0; i < length; i++)
            sprintf(dangerousSprintfBuffer + strlen(dangerousSprintfBuffer), " %02X", bytes[i]);

        qInfo("%s", dangerousSprintfBuffer);
    }

    const auto written = serialPort->write((const char*) bytes, length);
    return written;
}
