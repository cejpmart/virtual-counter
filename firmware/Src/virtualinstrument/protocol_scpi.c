#ifdef ENABLE_SCPI

#include "protocol.h"
#include "instrument.h"

#include <scpi/scpi.h>

#define SCPI_INPUT_BUFFER_LENGTH 256
#define SCPI_ERROR_QUEUE_SIZE 17
#define SCPI_IDN1 "FELCVUT"
#define SCPI_IDN2 "VINSTRUMENT"
#define SCPI_IDN3 NULL
#define SCPI_IDN4 "1.0"

static char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];
static scpi_error_t scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

static uint8_t scpi_output_buffer[SCPI_INPUT_BUFFER_LENGTH];
static size_t scpi_output_used = 0;

static scpi_t scpi_context;

// Settings
static uint16_t s_pwmPrescaler = 1;
static int32_t s_pwmPrescaled = 48000;
static int32_t s_pulsePhase = 0;

static size_t SCPI_Write(scpi_t * context, const char * data, size_t len) {
	memcpy(scpi_output_buffer + scpi_output_used, data, len);
	scpi_output_used += len;
    return len;
}

static scpi_result_t SCPI_Flush(scpi_t * context) {
    cdcDataOut(scpi_output_buffer, scpi_output_used);
    scpi_output_used = 0;

    return SCPI_RES_OK;
}

static int SCPI_Error(scpi_t * context, int_fast16_t err) {
    (void) context;

    //fprintf(stderr, "**ERROR: %d, \"%s\"\r\n", (int16_t) err, SCPI_ErrorTranslate(err));
    return 0;
}

static scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {
    (void) context;

    /*if (SCPI_CTRL_SRQ == ctrl) {
        fprintf(stderr, "**SRQ: 0x%X (%d)\r\n", val, val);
    } else {
        fprintf(stderr, "**CTRL %02x: 0x%X (%d)\r\n", ctrl, val, val);
    }*/
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_Reset(scpi_t * context) {
    (void) context;

    //fprintf(stderr, "**Reset\r\n");
    return SCPI_RES_OK;
}

static scpi_result_t scpi_frequency_mode(scpi_t * context);
static scpi_result_t scpi_measure_frequency(scpi_t * context);
static scpi_result_t scpi_measure_period(scpi_t * context);
static scpi_result_t scpi_measure_phase(scpi_t * context);
static scpi_result_t scpi_pulse_period(scpi_t * context);
static scpi_result_t scpi_pulse_phase(scpi_t * context);
static scpi_result_t scpi_sense_frequency_gate_time(scpi_t * context);

static scpi_interface_t scpi_interface = {
    .error = SCPI_Error,
    .write = SCPI_Write,
    .control = SCPI_Control,
    .flush = SCPI_Flush,
    .reset = SCPI_Reset,
};

static const scpi_command_t scpi_commands[] = {
    /* IEEE Mandated Commands (SCPI std V1999.0 4.1.1) */
    { .pattern = "*CLS", .callback = SCPI_CoreCls,},
    { .pattern = "*ESE", .callback = SCPI_CoreEse,},
    { .pattern = "*ESE?", .callback = SCPI_CoreEseQ,},
    { .pattern = "*ESR?", .callback = SCPI_CoreEsrQ,},
    { .pattern = "*IDN?", .callback = SCPI_CoreIdnQ,},
    { .pattern = "*OPC", .callback = SCPI_CoreOpc,},
    { .pattern = "*OPC?", .callback = SCPI_CoreOpcQ,},
    { .pattern = "*RST", .callback = SCPI_CoreRst,},
    { .pattern = "*SRE", .callback = SCPI_CoreSre,},
    /*{ .pattern = "*SRE?", .callback = SCPI_CoreSreQ,},
    { .pattern = "*STB?", .callback = SCPI_CoreStbQ,},
    { .pattern = "*TST?", .callback = SCPI_CoreTstQ,},
    { .pattern = "*WAI", .callback = SCPI_CoreWai,},*/

    /* Required SCPI commands (SCPI std V1999.0 4.2.1) */
	// Commented out to save Flash
    /*{ .pattern = "SYSTem:ERRor[:NEXT]?", .callback = SCPI_SystemErrorNextQ,},
    { .pattern = "SYSTem:ERRor:COUNt?", .callback = SCPI_SystemErrorCountQ,},
    { .pattern = "SYSTem:VERSion?", .callback = SCPI_SystemVersionQ,},

    { .pattern = "STATus:QUEStionable[:EVENt]?", .callback = SCPI_StatusQuestionableEventQ,},
    { .pattern = "STATus:QUEStionable:ENABle", .callback = SCPI_StatusQuestionableEnable,},
    { .pattern = "STATus:QUEStionable:ENABle?", .callback = SCPI_StatusQuestionableEnableQ,},

    { .pattern = "STATus:PRESet", .callback = SCPI_StatusPreset,},*/

	// Measurement
	{ .pattern = "SENSe:FREQuency:GATE:TIME", .callback = &scpi_sense_frequency_gate_time, },
	{ .pattern = "SENSe:FREQuency:MODE", .callback = &scpi_frequency_mode, },
    { .pattern = "MEASure:FREQuency?", .callback = &scpi_measure_frequency, },
	{ .pattern = "MEASure:PERiod?", .callback = &scpi_measure_period, },
	{ .pattern = "MEASure:PHASe?", .callback = &scpi_measure_phase, },

	// Output
	{ .pattern = "SOURce:PULSe:PERiod", .callback = &scpi_pulse_period, },
	{ .pattern = "SOURce:PULSe:PHASe", .callback = &scpi_pulse_phase, },

    //{ .pattern = "TEST:TREEA?", .callback = test_treeA,},
    //{ .pattern = "TEST:TREEB?", .callback = test_treeB,},

    { .pattern = "STUB", .callback = SCPI_Stub,},
    { .pattern = "STUB?", .callback = SCPI_StubQ,},

    //{ .pattern = "SAMple", .callback = SCPI_Sample,},
    SCPI_CMD_LIST_END
};

static const scpi_choice_def_t freq_modes[] = {
		{ "COUNter", MODE_COUNTER },
		{ "RECiprocal", MODE_RECIPROCAL },
		{ NULL, -1 },
};

static scpi_result_t scpi_frequency_mode(scpi_t* context) {
	int32_t freq_mode;

	if (!SCPI_ParamChoice(context, freq_modes, &freq_mode, TRUE))
		return SCPI_RES_ERR;

	instrumentSetFreqMode(freq_mode);
	return SCPI_RES_OK;
}

static scpi_result_t scpi_sense_frequency_gate_time(scpi_t* context) {
	int32_t gate_time;

	// TODO: Change to double when we have enough flash to do so
	if (!SCPI_ParamInt32(context, &gate_time, TRUE))
		return SCPI_RES_ERR;

	instrumentSetAperture(gate_time);
	return SCPI_RES_OK;
}

static scpi_result_t scpi_measure_frequency(scpi_t* context) {
	float freq;
	int duty;
	instrumentMeasureFrequency(&freq, &duty);

	SCPI_ResultUInt32(context, (unsigned int)(freq * 100));
	SCPI_ResultUInt32(context, (int)duty);

	return SCPI_RES_OK;
}

static scpi_result_t scpi_measure_period(scpi_t* context) {
	unsigned int period;
	int duty;
	instrumentMeasurePeriod(&period, &duty);

	SCPI_ResultUInt32(context, (unsigned int)period);
	SCPI_ResultUInt32(context, (int)duty);

	return SCPI_RES_OK;
}

static scpi_result_t scpi_measure_phase(scpi_t* context) {
	int period, phase;
	instrumentMeasurePhaseAtoB(&period, &phase);

	SCPI_ResultUInt32(context, (unsigned int)(period));
	SCPI_ResultInt32(context, (int)(phase));

	return SCPI_RES_OK;
}

static scpi_result_t scpi_pulse_period(scpi_t * context) {
	int32_t period;

	if (!SCPI_ParamInt32(context, &period, TRUE))
		return SCPI_RES_ERR;

	s_pwmPrescaler = 1;
	s_pwmPrescaled = period;

	while (s_pwmPrescaled >= 65535) {
		s_pwmPrescaler++;
		s_pwmPrescaled = period / s_pwmPrescaler;
	}

	HWSetGeneratorPWM(s_pwmPrescaler, s_pwmPrescaled, s_pwmPrescaled / 2, s_pulsePhase);
	return SCPI_RES_OK;
}

static scpi_result_t scpi_pulse_phase(scpi_t * context) {
	if (!SCPI_ParamInt32(context, &s_pulsePhase, TRUE))
		return SCPI_RES_ERR;

	while (s_pulsePhase < 0)
		s_pulsePhase += 360;

	HWSetGeneratorPWM(s_pwmPrescaler, s_pwmPrescaled, s_pwmPrescaled / 2, s_pulsePhase);
	return SCPI_RES_OK;
}

void protocolScpiInit() {
	SCPI_Init(&scpi_context, scpi_commands, &scpi_interface,
				scpi_units_def,
				SCPI_IDN1, SCPI_IDN2, SCPI_IDN3, SCPI_IDN4,
				scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
				scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE);
}

void protocolScpiHandle(const uint8_t* data, size_t length) {
	if (length)
		SCPI_Input(&scpi_context, (const char*) data, length);
}

#endif
