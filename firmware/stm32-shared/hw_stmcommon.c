#include "virtualinstrument/hw.h"

#ifdef STM32F042x6
#include "hw_stm32f042.h"
#include "stm32f0xx_hal.h"
#endif

// ugh! can we fix this?
extern DMA_HandleTypeDef    INPUT_CAPTURE_HDMA;
extern TIM_HandleTypeDef    INPUT_CAPTURE_HTIM;

static volatile uint32_t dmabuf[252];

// If hardware pre-scaler is used, dma_num_samples < measurement_num_samples
static volatile size_t dma_num_samples, measurement_num_samples;

//#define AUTO_RESET_IC_CNT

enum { HW_PRESCALER_MAX = 3 };

static uint32_t GetHWPrescaler(size_t index) {
    static const uint32_t hw_prescalers[] = {TIM_ICPSC_DIV1, TIM_ICPSC_DIV2, TIM_ICPSC_DIV4, TIM_ICPSC_DIV8};
    return hw_prescalers[index];
}

static void StartGate(uint32_t duration) {
    // TODO: we should do our own initialization of TIMEFRAME_TIM
    // TODO: this should be more flexible and not need TIMEFRAME_PRESCALER,
    // TODO: deriving the necessary values from SystemCoreClock instead

    // configure parameters
    TIMEFRAME_TIM->ARR = 65535;
    TIMEFRAME_TIM->PSC = SystemCoreClock / 1000 - 1;
    TIMEFRAME_TIM->CCR1 = duration / TIMEFRAME_PRESCALER;
    TIMEFRAME_TIM->CCMR1 = (0b110 << TIM_CCMR1_OC1M_Pos);

    // let the counter wrap-around immediately to force PWM reset
    TIMEFRAME_TIM->CNT = TIMEFRAME_TIM->ARR - 1;

    // generate Update event
    TIMEFRAME_TIM->EGR = TIM_EGR_UG_Msk;

    // start timer
    TIMEFRAME_TIM->CR1 |= TIM_CR1_CEN;
}

static void StopGate() {
    TIMEFRAME_TIM->CR1 &= ~TIM_CR1_CEN;
}

int HWStartPulseCountMeasurement(uint32_t gate_time_ms) {
    HAL_TIM_Base_Stop(&COUNTER_HTIM);
    StopGate();

    COUNTER_TIM->CNT = 0;

    TIM_ClockConfigTypeDef sClockSourceConfig;
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_ETRMODE2;
    sClockSourceConfig.ClockPolarity = TIM_CLOCKPOLARITY_NONINVERTED;
    sClockSourceConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
    sClockSourceConfig.ClockFilter = 0;
    HAL_TIM_ConfigClockSource(&COUNTER_HTIM, &sClockSourceConfig);

    TIM_SlaveConfigTypeDef sSlaveConfig;
    sSlaveConfig.SlaveMode = TIM_SLAVEMODE_GATED;
    sSlaveConfig.InputTrigger = COUNTER_TIM_GATE_IT;
    HAL_TIM_SlaveConfigSynchronization(&COUNTER_HTIM, &sSlaveConfig);

    COUNTER_TIM->CR1 |= TIM_CR1_CEN_Msk;

    StartGate(gate_time_ms);
    return 1;
}

int HWPollPulseCountMeasurement(uint32_t *value_out) {
    if (TIMEFRAME_TIM->CNT > TIMEFRAME_TIM->CCR1) {
        *value_out = COUNTER_TIM->CNT;
        return 1;
    }
    else
        return 0;
}

int HWStartPeriodMeasurement(size_t num_samples) {
    TIM_ClockConfigTypeDef sClockSourceConfig;
    TIM_IC_InitTypeDef sConfigIC;

    HAL_DMA_Abort(&INPUT_CAPTURE_HDMA);
    __HAL_DMA_CLEAR_FLAG(&INPUT_CAPTURE_HDMA, INPUT_CAPTURE_DMA_TC_FLAG);

    dma_num_samples = num_samples;
    measurement_num_samples = num_samples;

    int hw_prescaler = 0;

#ifndef AUTO_RESET_IC_CNT
    while (dma_num_samples % 2 == 0 && hw_prescaler < HW_PRESCALER_MAX) {
        dma_num_samples /= 2;
        hw_prescaler++;
    }
#endif

    if ((1 + dma_num_samples) * 8 > sizeof(dmabuf))
        return -1;

    // clock
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&INPUT_CAPTURE_HTIM, &sClockSourceConfig);

#ifdef AUTO_RESET_IC_CNT
    // slave mode reset - this way, missed pulses don't matter, but HW prescaling can't be used
    TIM_SlaveConfigTypeDef slaveConfig;
    slaveConfig.SlaveMode = TIM_SLAVEMODE_RESET;
    slaveConfig.InputTrigger = TIM_TS_TI2FP2;
    slaveConfig.TriggerFilter = 0;
    slaveConfig.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;
    slaveConfig.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
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

    // configure the DMA Burst Mode
    INPUT_CAPTURE_TIMER->DCR = (TIM_DMABASE_CCR2 | TIM_DMABURSTLENGTH_2TRANSFERS);
    __HAL_TIM_ENABLE_DMA(&INPUT_CAPTURE_HTIM, TIM_DMA_CC2);

    //INPUT_CAPTURE_HDMA.XferCpltCallback = XferCpltCallback;
    //__HAL_DMA_ENABLE_IT(&INPUT_CAPTURE_HDMA, DMA_IT_TC);

    HAL_DMA_Start(&INPUT_CAPTURE_HDMA, (uint32_t) &INPUT_CAPTURE_TIMER->DMAR, (uint32_t) dmabuf, (2 + dma_num_samples) * 2);
    return 1;
}

int HWPollPeriodMeasurement(uint64_t* period_out, uint64_t* pulse_width_out) {
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
        uint32_t period = (dmabuf[2 * i] - dmabuf[2 * (i - 1)]) & mask;
        uint32_t pulse_width = (dmabuf[2 * i + 1] - dmabuf[2 * (i - 1)]) & mask;
#endif

        sum_period += period;
        sum_pulse_width += pulse_width;
    }

    *period_out = (sum_period << 16) / measurement_num_samples;
    *pulse_width_out = (sum_pulse_width << 16) / measurement_num_samples;

    return 1;
}

int HWStartIntervalMeasurement(void) {
    return -1;

    HAL_TIM_Base_Stop(&COUNTER_HTIM);
    HAL_TIM_Base_Stop(&INPUT_CAPTURE_HTIM);
    HAL_TIM_IC_Stop_IT(&INPUT_CAPTURE_HTIM, INPUT_CAPTURE_FALLING_CHAN);

    TIM_ClockConfigTypeDef sClockSourceConfig;
    TIM_IC_InitTypeDef sConfigIC;

    // clock
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&INPUT_CAPTURE_HTIM, &sClockSourceConfig);

    // input capture
    // TODO: polarity
    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 0;
    HAL_TIM_IC_ConfigChannel(&INPUT_CAPTURE_HTIM, &sConfigIC, TIM_CHANNEL_2);
    HAL_TIM_IC_ConfigChannel(&INPUT_CAPTURE_HTIM, &sConfigIC, TIM_CHANNEL_4);

    // start!
    HAL_TIM_IC_Start(&INPUT_CAPTURE_HTIM, INPUT_CAPTURE_CH1_CHAN);
    //HAL_TIM_IC_Start(&INPUT_CAPTURE_HTIM, INPUT_CAPTURE_CH2_CHAN);
    HAL_TIM_IC_Start_IT(&INPUT_CAPTURE_HTIM, INPUT_CAPTURE_CH2_CHAN);
}

void HWSetGeneratorPWM(uint16_t prescaler, uint16_t period, uint16_t pulse_time, int phase) {
    // TODO: we should do our own init of PWM -- or not?
    PWM1_TIM->PSC = prescaler - 1;
    PWM1_TIM->ARR = period - 1;
    PWM1_CCR = pulse_time;
    PWM1_TIM->EGR |= TIM_EGR_UG;

    PWM2_TIM->PSC = prescaler - 1;
    PWM2_TIM->ARR = period - 1;
    PWM2_CCR = pulse_time;
    PWM2_TIM->EGR |= TIM_EGR_UG;

    // Ugh... basically make sure the update event has finished and we can safely mess with CNT
    while (PWM2_TIM->CNT == 0) {}

    PWM1_TIM->CNT = 0;
    PWM2_TIM->CNT = period * phase / 360 - 1;
}

void utilDelayMs(uint32_t milliseconds) {
    HAL_Delay(milliseconds);
}
