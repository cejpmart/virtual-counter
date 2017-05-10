#include "serialsession.h"

constexpr int timeout = 1000;

constexpr bool enableLogging = false;

void SerialSession::open(const char* filename) {
    serialPort.reset(new QSerialPort(filename));

    if (!serialPort->open(QIODevice::ReadWrite)) {
        auto error = (QString) filename + ": " + serialPort->errorString();
        throw std::runtime_error(qPrintable(error));
    }

    // Set mode to binary
    if (!this->write<uint8_t>(0xf0)) {
        throw std::runtime_error("Communication initialization failed");
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

/*QString SerialSession::readLine(int timeout) {
    if (!serialPort->canReadLine())
        serialPort->waitForReadyRead(timeout);

    if (!serialPort->canReadLine())
        return "ERROR";

    QByteArray line = serialPort->readLine();
    printf("%s", line.data());

    while (line.size() && isspace(line[line.size() - 1])) {
        line[line.size() - 1] = 0;      // this is needed, but why?!
        line.truncate(line.size() - 1);
    }

    return QString::fromLatin1(line);
}*/

bool SerialSession::readString(QString& string_out) {
    QElapsedTimer et;
    et.start();

    string_out = "";

    while (/*et.elapsed() < timeout*/ 1) {
        if (!serialPort->bytesAvailable())
            serialPort->waitForReadyRead(10);
        else {
            char c;

            if (!serialPort->read(&c, 1))
                continue;

            if (!c)
                return true;

            string_out.append(c);
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
        if (serialPort->read((char*) &receivedPacketData[0], length) == length) {
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
                qInfo(buffer);
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

/*void SerialSession::writeLine(QString text) {
    text += "\n";

    QByteArray bytes = text.toLatin1();
    serialPort->write(bytes);

    printf("%s", bytes.data());

    // Needed because of bugs in USB device implementation :(
    QThread::msleep(50);

    //serialPort->flush();
    //serialPort->waitForBytesWritten(-1);
}*/
