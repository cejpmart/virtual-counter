#ifndef SERIALSESSION_H
#define SERIALSESSION_H

#include <QObject>
#include <QString>

#include <QtSerialPort/QtSerialPort>

#include <memory>

class SerialSession : public QObject {
    Q_OBJECT

public:
    ~SerialSession() {}

    bool open(const char* filename);

    void writeLine(QString text);
    QString readLine(int timeout);

signals:
    void finished();
    void status(QString status);
    void error(QString err);

private:
    std::unique_ptr<QSerialPort> serialPort;
};

#endif // SERIALSESSION_H
