#include "serialsession.h"

constexpr int timeout = 1000;
constexpr int baudrate = 57600;

constexpr bool enableLogging = false;

void SerialSession::open(const char* filename) {
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

    // Set mode to binary
    if (!this->write<uint8_t>(0xf0)) {
        throw std::runtime_error("Communication initialization failed");
    }

    serialPort->flush();
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

    serialPort->startTransaction();

    uint8_t header[2];

    if (serialPort->read((char*) header, sizeof(header)) == sizeof(header)) {
        auto length = header[1];

        receivedPacketData.resize(length);
        auto rx = serialPort->read((char*) &receivedPacketData[0], length);

        if (enableLogging) {
            char buffer[1000];
            sprintf(buffer, "tag %02X, length %02X, rx +%d", header[0], header[1], (int)rx);
            qInfo("%s", buffer);
        }

        if (rx == length) {
            *tag_out = header[0];
            *length_out = header[1];
            *data_out = &receivedPacketData[0];

            if (enableLogging) {
                char buffer[1000];
                sprintf(buffer, "tag %02X, length %02X", *tag_out, (unsigned int) *length_out);
                if (length) {
                    strcat(buffer, ", data: [");
                    for (size_t i = 0; i < length; i++) {
                        char buffer2[100];
                        sprintf(buffer2, " %02X", receivedPacketData[i]);
                        strcat(buffer, buffer2);
                    }
                    strcat(buffer, "]");
                }
                qInfo("%s", buffer);
            }

            serialPort->commitTransaction();
            return true;
        }
    }

    serialPort->rollbackTransaction();
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
    auto written = serialPort->write((const char*) bytes, length);
    return written;
}
