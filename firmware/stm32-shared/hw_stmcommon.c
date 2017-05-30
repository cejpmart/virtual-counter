#include "virtualinstrument/hw.h"

#ifdef STM32F042x6
#include "stm32f0xx_hal.h"
#endif

enum { HW_PRESCALER_MAX = 3 };

enum { SAMPLES_OVERHEAD = 2 };

// ugh! can we fix this?
extern DMA_HandleTypeDef    INPUT_CAPTURE_HDMA;
extern TIM_HandleTypeDef    INPUT_CAPTURE_HTIM, TIMEFRAME_HTIM;

static volatile uint32_t dmabuf[(125 + SAMPLES_OVERHEAD) * 2];

// If hardware pre-scaler is used, dma_num_samples < measurement_num_samples
static size_t dma_num_samples, measurement_num_samples, measurement_prescaler;

#define AUTO_RESET_IC_CNT
//#define ENABLE_HW_PRESCALER

enum {
    GATE_MODE_TIME,
    GATE_MODE_ETR,
};

enum { CCR_value = 1 };

static int ConfigureAndStartGatedInternal(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig;
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    sClockSourceConfig.ClockPolarity = TIM_CLOCKPOLARITY_NONINVERTED;
    sClockSourceConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
    sClockSourceConfig.ClockFilter = 0;
    HAL_TIM_ConfigClockSource(&COUNTER_HTIM, &sClockSourceConfig);

    TIM_SlaveConfigTypeDef sSlaveConfig;
    sSlaveConfig.SlaveMode = TIM_SLAVEMODE_GATED;
    sSlaveConfig.InputTrigger = COUNTER_TIM_GATE_IT;
    HAL_TIM_SlaveConfigSynchronization(&COUNTER_HTIM, &sSlaveConfig);

    COUNTER_TIM->CNT = 0;
    COUNTER_TIM->CR1 |= TIM_CR1_CEN_Msk;
    return 1;
}

static void DmaStop(void) {
    HAL_DMA_Abort(&INPUT_CAPTURE_HDMA);
    __HAL_DMA_CLEAR_FLAG(&INPUT_CAPTURE_HDMA, INPUT_CAPTURE_DMA_TC_FLAG);
}

static void DmaStart(size_t num_samples) {
    // configure the DMA Burst Mode
    INPUT_CAPTURE_TIMER->DCR = (INPUT_CAPTURE_DMABASE | TIM_DMABURSTLENGTH_2TRANSFERS);
    __HAL_TIM_ENABLE_DMA(&INPUT_CAPTURE_HTIM, INPUT_CAPTURE_DMA_TRIGGER);

    HAL_DMA_Start(&INPUT_CAPTURE_HDMA, (uint32_t) &INPUT_CAPTURE_TIMER->DMAR, (uint32_t) dmabuf, (SAMPLES_OVERHEAD + num_samples) * 2);
}

static uint32_t GetHWPrescaler(size_t index) {
    static const uint32_t hw_prescalers[] = {TIM_ICPSC_DIV1, TIM_ICPSC_DIV2, TIM_ICPSC_DIV4, TIM_ICPSC_DIV8};
    return hw_prescalers[index];
}

static void ResetTimer(TIM_TypeDef* tim) {
    tim->CR1 &= ~(TIM_CR1_CEN | TIM_CR1_OPM);
    tim->CNT = 0;

    // Clear external enable modes
    tim->SMCR &= ~TIM_SMCR_SMS;
}

static void StartOnePulse(TIM_TypeDef* tim) {
    tim->CR1 |= (TIM_CR1_CEN | TIM_CR1_OPM);
}

static void StartTimer(TIM_TypeDef* tim) {
    tim->CR1 |= TIM_CR1_CEN;
}

// TODO: maybe refactor
static int ConfigureGate(int mode, unsigned int prescaler, unsigned int period) {
    TIM_ClockConfigTypeDef sClockSourceConfig;
    TIM_MasterConfigTypeDef sMasterConfig;
    TIM_OC_InitTypeDef sConfigOC;

    ResetTimer(TIMEFRAME_TIM);

    TIMEFRAME_HTIM.Instance = TIMEFRAME_TIM;
    TIMEFRAME_HTIM.Init.Prescaler = prescaler;
    TIMEFRAME_HTIM.Init.CounterMode = TIM_COUNTERMODE_UP;
    TIMEFRAME_HTIM.Init.Period = period;
    TIMEFRAME_HTIM.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_Base_Init(&TIMEFRAME_HTIM) != HAL_OK)
        return -1;

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&TIMEFRAME_HTIM, &sClockSourceConfig) != HAL_OK)
        return -1;

    if (HAL_TIM_OC_Init(&TIMEFRAME_HTIM) != HAL_OK)
        return -1;

    TIM_SlaveConfigTypeDef slaveConfig;
    if (mode == GATE_MODE_TIME) {
        slaveConfig.SlaveMode = TIM_SLAVEMODE_DISABLE;
        slaveConfig.InputTrigger = TIM_TS_ITR0;
    }
    else {
        slaveConfig.SlaveMode = TIM_SLAVEMODE_EXTERNAL1;
        slaveConfig.InputTrigger = TIM_TS_TI1FP1;           // TODO: must be configurable
        slaveConfig.TriggerFilter = 0;
        slaveConfig.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;
        slaveConfig.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
    }
    HAL_TIM_SlaveConfigSynchronization(&TIMEFRAME_HTIM, &slaveConfig);

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC2REF;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_ENABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&TIMEFRAME_HTIM, &sMasterConfig) != HAL_OK)
        return -1;

    sConfigOC.OCMode = TIM_OCMODE_INACTIVE;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_OC_ConfigChannel(&TIMEFRAME_HTIM, &sConfigOC, TIMEFRAME_CHAN) != HAL_OK)
        return -1;

    return 1;
}

// Configure a timer as gate, using EXT1 mode
// (OC2 is routed to TRGO)
// num_periods is the number of full periods counted
// input_trigger is TIM_TS_TI1FP1 etc.
static int ConfigureGateExt1(TIM_TypeDef* tim, uint32_t num_periods, uint32_t input_trigger) {
    static const uint32_t CCR_value = 1;                // number of events to skip (1 = first, incomplete period)
    static const uint32_t channel = TIM_CHANNEL_2;

    ResetTimer(tim);

    TIM_HandleTypeDef htim;
    htim.Instance = tim;
    htim.State = HAL_TIM_STATE_READY;
    htim.Init.Prescaler = 0;
    htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim.Init.Period = CCR_value + num_periods - 1;
    htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_Base_Init(&htim) != HAL_OK)
        return -1;

    TIM_ClockConfigTypeDef sClockSourceConfig;
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim, &sClockSourceConfig) != HAL_OK)
        return -1;

    if (HAL_TIM_OC_Init(&htim) != HAL_OK)
        return -1;

    TIM_SlaveConfigTypeDef slaveConfig;
    slaveConfig.SlaveMode = TIM_SLAVEMODE_EXTERNAL1;
    slaveConfig.InputTrigger = input_trigger;
    slaveConfig.TriggerFilter = 0;
    slaveConfig.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;
    slaveConfig.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
    HAL_TIM_SlaveConfigSynchronization(&htim, &slaveConfig);

    TIM_MasterConfigTypeDef sMasterConfig;
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC2REF;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_ENABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim, &sMasterConfig) != HAL_OK)
        return -1;

    TIM_OC_InitTypeDef sConfigOC;
    sConfigOC.OCMode = TIM_OCMODE_FORCED_INACTIVE;
    sConfigOC.Pulse = CCR_value;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_OC_ConfigChannel(&htim, &sConfigOC, channel) != HAL_OK)
        return -1;

    sConfigOC.OCMode = TIM_OCMODE_PWM2;
    if (HAL_TIM_OC_ConfigChannel(&htim, &sConfigOC, channel) != HAL_OK)
        return -1;

    return 1;
}

static int ConfigureMaster(TIM_TypeDef* tim, uint32_t master_output_trigger) {
    TIM_HandleTypeDef htim;
    htim.Instance = tim;
    htim.Lock = HAL_UNLOCKED;
    htim.State = HAL_TIM_STATE_READY;

    TIM_MasterConfigTypeDef sMasterConfig;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_ENABLE;
    sMasterConfig.MasterOutputTrigger = master_output_trigger;

    if (HAL_TIMEx_MasterConfigSynchronization(&htim, &sMasterConfig) != HAL_OK)
        return -1;

    return 1;
}

static int ConfigureInternalCounting(TIM_TypeDef* tim) {
    TIM_HandleTypeDef htim;
    htim.Instance = tim;
    htim.Lock = HAL_UNLOCKED;
    htim.State = HAL_TIM_STATE_READY;

    htim.Init.Prescaler = 0;
    htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim.Init.Period = (uint32_t)(-1);
    htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim.Init.RepetitionCounter = 0;
    if (HAL_TIM_Base_Init(&htim) != HAL_OK)
        return -1;

    TIM_ClockConfigTypeDef sClockSourceConfig;
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&TIMEFRAME_HTIM, &sClockSourceConfig) != HAL_OK)
        return -1;

    return 1;
}

static int ConfigureSlave(TIM_TypeDef* tim, uint32_t clock_source, uint32_t slave_mode, uint32_t input_trigger) {
    TIM_HandleTypeDef htim;
    htim.Instance = tim;
    htim.Lock = HAL_UNLOCKED;
    htim.State = HAL_TIM_STATE_READY;

    htim.Init.Prescaler = 0;
    htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim.Init.Period = (uint32_t)(-1);
    htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim.Init.RepetitionCounter = 0;
    if (HAL_TIM_Base_Init(&htim) != HAL_OK)
        return -1;

    TIM_ClockConfigTypeDef sClockSourceConfig;
    sClockSourceConfig.ClockSource = clock_source;
    sClockSourceConfig.ClockPolarity = TIM_CLOCKPOLARITY_NONINVERTED;
    sClockSourceConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
    sClockSourceConfig.ClockFilter = 0;
    HAL_TIM_ConfigClockSource(&htim, &sClockSourceConfig);

    TIM_SlaveConfigTypeDef sSlaveConfig;
    sSlaveConfig.SlaveMode = slave_mode;
    sSlaveConfig.InputTrigger = input_trigger;
    HAL_TIM_SlaveConfigSynchronization(&htim, &sSlaveConfig);

    return 1;
}

static int ConfigureGatedInternal2(TIM_TypeDef* tim, uint32_t input_trigger) {
    return ConfigureSlave(tim, TIM_CLOCKSOURCE_INTERNAL, TIM_SLAVEMODE_GATED, input_trigger);
}

static int ConfigureSlaveExt1(TIM_TypeDef* tim, uint32_t input_trigger) {
    return ConfigureSlave(tim, TIM_CLOCKSOURCE_INTERNAL, TIM_SLAVEMODE_EXTERNAL1, input_trigger);
}

static void StartGateTime(uint32_t duration) {
    // TODO: this should be more flexible and not need TIMEFRAME_PRESCALER,
    // TODO: deriving the necessary values from SystemCoreClock instead

    TIMEFRAME_TIM->CNT = 0;
    TIMEFRAME_CCR = duration / TIMEFRAME_PRESCALER;             // gate time
    TIMEFRAME_TIM->CCMR1 = (0b101 << TIM_CCMR1_OC2M_Pos);       // force to 1
    TIMEFRAME_TIM->CCMR1 = (0b010 << TIM_CCMR1_OC2M_Pos);       // clear when CNT == CCR

    // start timer
    TIMEFRAME_TIM->CR1 |= TIM_CR1_CEN;
}

static void StopTimer(TIM_TypeDef* tim) {
    tim->CR1 &= ~TIM_CR1_CEN;
}

int HWStartPeriodMeasurement(size_t num_periods) {
#ifdef PERIOD_SETUP_16_32
    StopTimer(COUNTER_TIM);
    StopTimer(TIMEFRAME_TIM);

    if (ConfigureGateExt1(TIMEFRAME_TIM, num_periods, PERIOD_GATE_TS) <= 0)
        return -1;

    ConfigureAndStartGatedInternal();

    StartOnePulse(TIMEFRAME_TIM);

    return 1;
#elif defined(PERIOD_SETUP_32_16_16)
    ResetTimer(PERIOD_GATE_TIM);
    ResetTimer(PERIOD_COUNTER_LO_TIM);
    ResetTimer(PERIOD_COUNTER_HI_TIM);

    if (ConfigureGateExt1(PERIOD_GATE_TIM, num_periods, PERIOD_GATE_TS) <= 0)
        return -1;

    if (ConfigureGatedInternal2(PERIOD_COUNTER_LO_TIM, PERIOD_COUNTER_LO_TS) <= 0
            || ConfigureMaster(PERIOD_COUNTER_LO_TIM, TIM_TRGO_UPDATE) <= 0)
        return -1;

    if (ConfigureSlaveExt1(PERIOD_COUNTER_HI_TIM, PERIOD_COUNTER_HI_TS) <= 0)
        return -1;

    StartTimer(PERIOD_COUNTER_HI_TIM);
    StartTimer(PERIOD_COUNTER_LO_TIM);
    StartOnePulse(PERIOD_GATE_TIM);

    return 1;
#else
#error
#endif
}

int HWPollPeriodMeasurement(uint64_t* period_out) {
#ifdef PERIOD_SETUP_16_32
    // When the measurement is done, the timer stops and clears CEN flag
    if ((TIMEFRAME_TIM->CR1 & TIM_CR1_CEN))
        return 0;

    __HAL_TIM_DISABLE(&COUNTER_HTIM);

    *period_out = ((uint64_t)(COUNTER_TIM->CNT) << 32) / (TIMEFRAME_TIM->ARR + 1 - CCR_value);
    return 1;
#elif defined(PERIOD_SETUP_32_16_16)
    // When the measurement is done, the timer stops and clears CEN flag
    if ((PERIOD_GATE_TIM->CR1 & TIM_CR1_CEN))
        return 0;

    const uint32_t cnt = PERIOD_COUNTER_LO_TIM->CNT | PERIOD_COUNTER_HI_TIM->CNT << 16;
    const uint32_t num_periods = PERIOD_GATE_TIM->ARR + 1 - CCR_value;
    *period_out = ((uint64_t)(cnt) << 32) / num_periods;
    return 1;
#else
#error
#endif
}

int HWStartPulseCountMeasurement(uint32_t gate_time_ms) {
    ResetTimer(COUNTER_TIM);
    ResetTimer(TIMEFRAME_TIM);

    int prescaler = SystemCoreClock / 1000 - 1;
    if (ConfigureGate(GATE_MODE_TIME, prescaler, 65535) <= 0)
        return -1;

    if (ConfigureSlave(COUNTER_TIM, TIM_CLOCKSOURCE_ETRMODE2, TIM_SLAVEMODE_GATED, COUNTER_TIM_GATE_IT) <= 0)
        return -1;

    StartTimer(COUNTER_TIM);
    StartGateTime(gate_time_ms);
    return 1;
}

int HWPollPulseCountMeasurement(uint32_t *value_out) {
    if (TIMEFRAME_TIM->CNT > TIMEFRAME_CCR) {
        *value_out = COUNTER_TIM->CNT;
        return 1;
    }
    else
        return 0;
}

int HWStartPwmMeasurement(size_t num_samples) {
    TIM_ClockConfigTypeDef sClockSourceConfig;
    TIM_IC_InitTypeDef sConfigIC;

    __HAL_TIM_DISABLE(&INPUT_CAPTURE_HTIM);
    DmaStop();

    dma_num_samples = num_samples;
    measurement_num_samples = num_samples;

    int hw_prescaler = 0;
    measurement_prescaler = 1;

#if !defined(AUTO_RESET_IC_CNT) && defined(ENABLE_HW_PRESCALER)
    while (dma_num_samples % 2 == 0 && hw_prescaler < HW_PRESCALER_MAX) {
        dma_num_samples /= 2;
        measurement_prescaler <<= 1;
        hw_prescaler++;
    }
#endif

    if ((SAMPLES_OVERHEAD + dma_num_samples) * 8 > sizeof(dmabuf))
        return -1;

    // clock
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&INPUT_CAPTURE_HTIM, &sClockSourceConfig);

#ifdef AUTO_RESET_IC_CNT
    // slave mode reset - this way, missed pulses don't matter, but HW prescaling can't be used
    TIM_SlaveConfigTypeDef slaveConfig;
    slaveConfig.SlaveMode = TIM_SLAVEMODE_RESET;
    slaveConfig.InputTrigger = INPUT_CAPTURE_AUTO_RESET_TRIGGER;
    slaveConfig.TriggerFilter = 0;
    slaveConfig.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;
    slaveConfig.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
    HAL_TIM_SlaveConfigSynchronization(&INPUT_CAPTURE_HTIM, &slaveConfig);
#else
    TIM_SlaveConfigTypeDef slaveConfig;
    slaveConfig.SlaveMode = TIM_SLAVEMODE_DISABLE;
    HAL_TIM_SlaveConfigSynchronization(&INPUT_CAPTURE_HTIM, &slaveConfig);
#endif

    // input capture
    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = GetHWPrescaler(hw_prescaler);
    sConfigIC.ICFilter = 0;
    HAL_TIM_IC_ConfigChannel(&INPUT_CAPTURE_HTIM, &sConfigIC, INPUT_CAPTURE_FALLING_CHAN);

    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    HAL_TIM_IC_ConfigChannel(&INPUT_CAPTURE_HTIM, &sConfigIC, INPUT_CAPTURE_RISING_CHAN);

    // start!
    HAL_TIM_IC_Start(&INPUT_CAPTURE_HTIM, INPUT_CAPTURE_RISING_CHAN);
    HAL_TIM_IC_Start(&INPUT_CAPTURE_HTIM, INPUT_CAPTURE_FALLING_CHAN);

    DmaStart(dma_num_samples);
    __HAL_TIM_ENABLE(&INPUT_CAPTURE_HTIM);
    return 1;
}

int HWPollPwmMeasurement(uint64_t* period_out, uint64_t* pulse_width_out) {
    if (!(__HAL_DMA_GET_FLAG(&INPUT_CAPTURE_HDMA, INPUT_CAPTURE_DMA_TC_FLAG)))
        return 0;

    uint64_t sum_period = 0;
    uint64_t sum_pulse_width = 0;

    for (size_t i = 2; i < dma_num_samples + 2; i++) {
#ifdef AUTO_RESET_IC_CNT
        // +2 is correct, but why and how?
        uint32_t period = dmabuf[2 * i] + 2;
        uint32_t pulse_width = dmabuf[2 * i + 1] + 2;
#else
        static const uint32_t mask = 0xffffffff;

        // load this manually because dmabuf is declared volatile
        uint32_t prev = dmabuf[2 * (i - 1)];

        uint32_t period = (dmabuf[2 * i] - prev) & mask;
        uint32_t pulse_width;

        if (dmabuf[2 * i + 1] < dmabuf[2 * i])
            pulse_width = (dmabuf[2 * i + 1] - prev) & mask;
        else
            pulse_width = (dmabuf[2 * i + 1] - dmabuf[2 * i]) & mask;
#endif

        sum_period += period;
        sum_pulse_width += pulse_width;
    }

    *period_out = (sum_period << 16) / measurement_num_samples;

    if (dma_num_samples == measurement_num_samples) {
        *pulse_width_out = (sum_pulse_width << 16) / measurement_num_samples;
    }
    else {
        uint64_t actual_period = sum_period / measurement_prescaler;

        while (sum_pulse_width > actual_period)
            sum_pulse_width -= actual_period;

        *pulse_width_out = ((sum_pulse_width * measurement_prescaler) << 16) / measurement_num_samples;
    }

    return 1;
}

int HWStartIntervalMeasurement(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig;
    TIM_IC_InitTypeDef sConfigIC;

    ResetTimer(INPUT_CAPTURE_TIMER);
    DmaStop();

    ConfigureInternalCounting(INPUT_CAPTURE_TIMER);

    // clock
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&INPUT_CAPTURE_HTIM, &sClockSourceConfig);

    // input capture
    // TODO: polarity
    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 0;
    HAL_TIM_IC_ConfigChannel(&INPUT_CAPTURE_HTIM, &sConfigIC, INPUT_CAPTURE_CH1_CHAN);
    HAL_TIM_IC_ConfigChannel(&INPUT_CAPTURE_HTIM, &sConfigIC, INPUT_CAPTURE_CH2_CHAN);

    // start!
    HAL_TIM_IC_Start(&INPUT_CAPTURE_HTIM, INPUT_CAPTURE_CH1_CHAN);
    HAL_TIM_IC_Start(&INPUT_CAPTURE_HTIM, INPUT_CAPTURE_CH2_CHAN);

    DmaStart(2);
    StartTimer(INPUT_CAPTURE_TIMER);

    return 1;
}

int HWPollIntervalMeasurement(uint32_t* period_out, uint32_t* pulse_width_out) {
    if (!(__HAL_DMA_GET_FLAG(&INPUT_CAPTURE_HDMA, INPUT_CAPTURE_DMA_TC_FLAG)))
        return 0;

    enum { start = 2 };

    *period_out = dmabuf[start + 2] - dmabuf[start];
    *pulse_width_out = dmabuf[start + 3] - dmabuf[start];
    return 1;
}

int HWStartFreqRatioMeasurement(size_t num_periods) {
    ResetTimer(COUNTER_TIM);
    ResetTimer(TIMEFRAME_TIM);

    if (ConfigureGateExt1(TIMEFRAME_TIM, num_periods, FREQ_RATIO_GATE_TS) <= 0)
        return -1;

    if (ConfigureSlave(COUNTER_TIM, TIM_CLOCKSOURCE_ETRMODE2, TIM_SLAVEMODE_GATED, COUNTER_TIM_GATE_IT) <= 0)
        return -1;

    StartTimer(COUNTER_TIM);
    StartOnePulse(TIMEFRAME_TIM);




    return 1;
}

int HWPollFreqRatioMeasurement(uint64_t* ratio_out) {
    // When the measurement is done, the timer stops and clears CEN flag
    if ((TIMEFRAME_TIM->CR1 & TIM_CR1_CEN))
        return 0;

    __HAL_TIM_DISABLE(&COUNTER_HTIM);

    *ratio_out = ((uint64_t)(COUNTER_TIM->CNT) << 16) / (TIMEFRAME_TIM->ARR + 1 - CCR_value);
    return 1;
}

int HWSetPwm(size_t index, uint16_t prescaler, uint16_t period, uint16_t pulse_time, int phase) {
    // TODO: we should do our own init of PWM -- or not?
    if (index == 0) {
        PWM1_TIM->PSC = prescaler;
        PWM1_TIM->ARR = period;
        PWM1_CCR = pulse_time;
        PWM1_TIM->EGR |= TIM_EGR_UG;
    }
    else if (index == 1) {
        PWM2_TIM->PSC = prescaler;
        PWM2_TIM->ARR = period;
        PWM2_CCR = pulse_time;
        PWM2_TIM->EGR |= TIM_EGR_UG;
    }
    else
        return -1;

    return 1;
}

int HWSetPwmPhase(uint32_t phase) {
    PWM1_TIM->CNT = 0;
    PWM2_TIM->CNT = phase;
    return 1;
}

void utilDelayMs(uint32_t milliseconds) {
    HAL_Delay(milliseconds);
}
