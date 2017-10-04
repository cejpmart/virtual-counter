#ifndef GUICOMMON_H
#define GUICOMMON_H

#include <QHash>
#include <QString>

#include <qmetatype.h>

constexpr uint16_t VERSION = 1103;

template <typename P>
struct Parameter {
    P setpoint;
    bool setting = false;
    bool pending = false;

    // Returns true, if a new request should be started now
    bool startSetting() {
        if (!setting) {
            setting = true;
            return true;
        }
        else {
            pending = true;
            return false;
        }
    }

    bool continuePending() {
        if (pending) {
            pending = false;
            return true;
        }
        else {
            setting = false;
            return false;
        }
    }
};

enum class TimebaseSource { external, internal, usb20 };

struct InstrumentInfo {
    QString port;
    QString board;
    int firmware;
    unsigned int f_cpu;
    TimebaseSource timebaseSource;
};

typedef QHash<QString, QString> InstrumentParameterMap;

struct PwmParameters {
    bool enabled = false;
    float freq = 0, duty = 0, phase = 0;
};

Q_DECLARE_METATYPE(InstrumentInfo);
Q_DECLARE_METATYPE(PwmParameters);

#endif // GUICOMMON_H
