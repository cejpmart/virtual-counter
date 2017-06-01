#ifndef VIRTUALINSTRUMENT_PROTOCOL_ASCII_H_
#define VIRTUALINSTRUMENT_PROTOCOL_ASCII_H_

typedef struct {
    const char* port_in_pulse_count;
    const char* port_in_period;
    const char* port_in_pwm_1;
    const char* port_in_pwm_2;
    const char* port_in_interval_a;
    const char* port_in_interval_b;
    const char* port_in_freq_ratio_a;
    const char* port_in_freq_ratio_b;
    const char* port_out_pwm_a;
    const char* port_out_pwm_b;
}
protocol_ascii_options_t;

void protocolAsciiInit(const protocol_ascii_options_t* options);
void protocolAsciiHandle(const uint8_t* data, size_t length);

#endif /* VIRTUALINSTRUMENT_PROTOCOL_ASCII_H_ */
