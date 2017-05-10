#ifndef GUICOMMON_H
#define GUICOMMON_H

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

struct PwmParameters {
    bool enabled = true;
    float freq = 1000, duty = 0.5, phase = 0;
};

#endif // GUICOMMON_H
