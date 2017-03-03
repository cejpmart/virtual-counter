#include "serialsession.h"

bool SerialSession::open(const char* filename) {
    serialPort.reset(new QSerialPort(filename));

    if (!serialPort->open(QIODevice::ReadWrite)) {
        emit error((QString) filename + ": " + serialPort->errorString());
        emit finished();
        return false;
    }

    // Set mode to SCPI
    char modeSet = 0xf2;
    serialPort->write(&modeSet, 1);

    emit status((QString) "Connected to " + filename);
    //serialPort->setReadBufferSize(1000000);

    return true;
}

QString SerialSession::readLine() {
    if (!serialPort->canReadLine())
        serialPort->waitForReadyRead(11000);

    if (!serialPort->canReadLine())
        return "ERROR";

    QByteArray line = serialPort->readLine();
    printf("%s", line.data());
    return QString::fromLatin1(line);
}

void SerialSession::writeLine(QString text) {
    text += "\n";

    QByteArray bytes = text.toLatin1();
    serialPort->write(bytes);

    printf("%s", bytes.data());

    // Needed because of bugs in USB device implementation :(
    QThread::msleep(50);

    //serialPort->flush();
    //serialPort->waitForBytesWritten(-1);
}
