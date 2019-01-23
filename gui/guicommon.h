#ifndef GUICOMMON_H
#define GUICOMMON_H

// TODO: rename; it's not much related to the GUI, but rather Qt and the Instrument

#include <QHash>
#include <QString>

#include <qmetatype.h>

#include "../common/protocoldefs.h"

template <typename P>
struct Parameter {
    P setpoint;
    bool setting = false;
    bool pending = false;

    // Returns true, if a new request should be started now
    bool canStartSetting() {
        if (!setting) {
            // it's ok to send the request NOW
            setting = true;
            return true;
        }
        else {
            // a request is in progress, hold off
            pending = true;
            return false;
        }
    }

    bool canContinuePending() {
        if (pending) {
            // another request is pending, keep setting == true and clear the request
            pending = false;
            return true;
        }
        else {
            // no request is pending, we're done
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

struct DgenOptions {
    bool enabled = false;
    float freq = 0, duty = 0, phase = 0;
};

typedef std::array<DgenOptions, NUM_DGEN> AllDgenOptions;

Q_DECLARE_METATYPE(AllDgenOptions);
Q_DECLARE_METATYPE(InstrumentInfo);
Q_DECLARE_METATYPE(DgenOptions);

#endif // GUICOMMON_H
