#ifndef PROTOCOLDEFS_H
#define PROTOCOLDEFS_H

enum {
    CMD_QUERY_VERSION =         '?',
    CMD_SET_MODE_COUNTING =     'q',

    CMD_START_MEASUREMENT =     's',
    CMD_POLL_MEASUREMENT =      'p',
    CMD_ABORT_MEASUREMENT =     'a',

    CMD_SET_PWM_FREQUENCY =     0x80,
    CMD_SET_PWM_PHASE =         0x81,
    CMD_RESET_INSTRUMENT =      0xA0
};

enum {
    MEASUREMENT_PULSE_COUNT =   0x01,
    MEASUREMENT_PERIOD =        0x02,
    MEASUREMENT_PHASE =         0x03,
    MEASUREMENT_FREQ_RATIO =    0x04,

    MEASUREMENT_FLAG_CONTINUOUS =   0x01,
};

enum {
    INFO_RESULT_CODE =          0x10,   // uint8 rc
    INFO_MEASUREMENT_DATA =     0x20,   // uint8 rc + uint8[] data
};

enum {
    RESULT_CODE_BUSY =          0,
    RESULT_CODE_OK =            1,
};

typedef struct {
    uint32_t gate_time;
} __attribute__((packed)) measurement_pulse_count_request_t;

typedef struct {
    uint32_t count;
} __attribute__((packed)) measurement_pulse_count_result_t;

typedef struct {
    uint32_t iterations;
} __attribute__((packed)) measurement_period_request_t;

typedef struct {
    uint64_t period;
    uint64_t pulse_width;
} __attribute__((packed)) measurement_period_result_t;

typedef struct {
} __attribute__((packed)) measurement_phase_request_t;

typedef struct {
    uint32_t period;
    int32_t interval;
} __attribute__((packed)) measurement_phase_result_t;

typedef struct {
    uint32_t iterations;
} __attribute__((packed)) measurement_freq_ratio_request_t;

typedef struct {
    uint32_t ratio;
} __attribute__((packed)) measurement_freq_ratio_result_t;

typedef struct {
    uint32_t period;
} __attribute__((packed)) set_pwm_frequency_request_t;

typedef struct {
    uint32_t phase;
} __attribute__((packed)) set_pwm_phase_request_t;

#endif // PROTOCOLDEFS_H
