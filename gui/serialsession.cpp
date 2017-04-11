#include "serialsession.h"

#define VERSION "1.0"

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

    // Check firmware version
    writeLine("*IDN?");
    QString reply = readLine(1000);
    QStringList tokens = reply.split(",");

    if (tokens.size() < 4 || tokens[3] != VERSION) {
        printf("%d `%s` `%s`\n", tokens.size(), tokens[3].toLatin1().data(), VERSION);
        emit status((QString) "Firmware version mismatch!");
        emit finished();
        return false;
    }

    emit status((QString) "Connected to " + filename);
    //serialPort->setReadBufferSize(1000000);

    return true;
}

QString SerialSession::readLine(int timeout) {
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
