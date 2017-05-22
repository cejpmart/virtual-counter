#ifndef PROTOCOLDEFS_H
#define PROTOCOLDEFS_H

#include <stdint.h>

enum {
    CMD_SET_MODE_COUNTING =     'q',

    CMD_START_MEASUREMENT =     's',
    CMD_POLL_MEASUREMENT =      'p',
    CMD_ABORT_MEASUREMENT =     'a',

    CMD_SET_PWM =               0x80,
    CMD_RESET_INSTRUMENT =      0xA0,
    CMD_QUERY_INSTRUMENT =      0xA1,
};

enum {
    MEASUREMENT_PULSE_COUNT =   0x01,
    MEASUREMENT_PERIOD =        0x02,
    MEASUREMENT_PWM =           0x03,
    MEASUREMENT_INTERVAL =      0x04,
    MEASUREMENT_FREQ_RATIO =    0x05,

    MEASUREMENT_FLAG_CONTINUOUS =   0x01,
};

enum {
    INFO_RESULT_CODE =          0x10,   // uint8 rc
    INFO_MEASUREMENT_DATA =     0x20,   // uint8 rc + uint8[] data
    INFO_INSTRUMENT_INFO =      0xA1,   // instrument_info_t
};

enum {
    RESULT_CODE_BUSY =          0,
    RESULT_CODE_OK =            1,
    RESULT_CODE_PROTOCOL_ERROR = 2,
};

enum {
    BOARD_F042F6 =              0x0100,
    BOARD_F042K6_NUCLEO32 =     0x0200,
    BOARD_F303_NUCLEO64 =       0x0300,
    BOARD_F411_NUCLEO64 =       0x0400,
    BOARD_F373_EVAL =           0x0500,
};

typedef struct {
    uint16_t board_id;
    uint16_t fw_ver;
    uint32_t f_cpu;
} __attribute__((packed)) instrument_info_t;

typedef struct {
    uint32_t gate_time;
} __attribute__((packed)) measurement_pulse_count_request_t;

typedef struct {
    uint32_t count;
} __attribute__((packed)) measurement_pulse_count_result_t;

typedef struct {
    uint32_t num_periods;
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
    uint64_t ratio;
} __attribute__((packed)) measurement_freq_ratio_result_t;

typedef struct {
    uint16_t index;             // 0 or 1
    uint16_t prescaler;         // in CPU units
    uint16_t period;            // in CPU units
    uint16_t pulse_width;       // in CPU units
    uint16_t phase;             // in CPU units (0..period-1)
} __attribute__((packed)) set_pwm_request_t;

#endif // PROTOCOLDEFS_H
