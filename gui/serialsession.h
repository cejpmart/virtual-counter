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

    void open(const char* filename);

    bool awaitPacket(uint8_t* tag_out, uint8_t const** data_out, size_t* length_out);
    bool receivePacket(uint8_t* tag_out, uint8_t const** data_out, size_t* length_out);
    bool sendPacket(uint8_t tag, const uint8_t* data, size_t length);

    bool readString(QString& string_out);

private:
    //size_t read(const uint8_t* bytes, size_t length);
    size_t write(const uint8_t* bytes, size_t length);

    template <typename T>
    bool write(T value) {
        return this->write(reinterpret_cast<const uint8_t*>(&value), sizeof(value)) == sizeof(value);
    }

/*signals:
    void finished();
    void status(QString status);
    void error(QString err);*/

private:
    void setTimeoutErrorFlag();

    std::unique_ptr<QSerialPort> serialPort;

    std::vector<uint8_t> receivedPacketData;
};

#endif // SERIALSESSION_H
