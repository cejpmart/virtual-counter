#include "hw.h"

#include "stm32f0xx_hal.h"

// ugh! can we fix this?
extern DMA_HandleTypeDef    INPUT_CAPTURE_HDMA;
extern TIM_HandleTypeDef    INPUT_CAPTURE_HTIM;

static volatile uint32_t dmabuf[202];
static volatile size_t dma_num_samples;

/*static void XferCpltCallback(struct __DMA_HandleTypeDef* hdma) {
}*/

void HWClearPeriodMeasurement(void) {
    //meas_rec_valid = 0;
    //lastRiseTime = 0;
}

void HWClearPulseCounter(void) {
    INPUT_CAPTURE_TIMER->CNT = 0;
    INPUT_CAPTURE_TIMER->CR1 |= TIM_CR1_CEN_Msk;
}

int HWGetPeriodPulseWidth(uint64_t* period_out, uint64_t* pulse_width_out) {
    if (!(__HAL_DMA_GET_FLAG(&INPUT_CAPTURE_HDMA, INPUT_CAPTURE_DMA_TC_FLAG)))
        return 0;

    uint64_t sum_period = 0;
    uint64_t sum_pulse_width = 0;

    for (size_t i = 1; i < dma_num_samples + 1; i++) {
        // +2 is correct, but why and how?
        //static const uint32_t mask = 0xffffffff;

        uint32_t period = dmabuf[2 * i] + 2;//(dmabuf[2 * i] - dmabuf[2 * (i - 1)]) & mask;
        uint32_t pulse_width = dmabuf[2 * i + 1] + 2;//(dmabuf[2 * i + 1] - dmabuf[2 * (i - 1)]) & mask;

        sum_period += period;
        sum_pulse_width += pulse_width;
    }

    *period_out = (sum_period << 16) / dma_num_samples;
    *pulse_width_out = (sum_pulse_width << 16) / dma_num_samples;

    return 1;
}

void HWInitPeriodMeasurement(size_t num_samples) {
    TIM_ClockConfigTypeDef sClockSourceConfig;
    TIM_IC_InitTypeDef sConfigIC;

    // clock
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&INPUT_CAPTURE_HTIM, &sClockSourceConfig);

    // slave mode reset - this way, missed pulses don't matter
    TIM_SlaveConfigTypeDef slaveConfig;
    slaveConfig.SlaveMode = TIM_SLAVEMODE_RESET;
    slaveConfig.InputTrigger = TIM_TS_TI2FP2;
    slaveConfig.TriggerFilter = 0;
    slaveConfig.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;
    slaveConfig.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
    HAL_TIM_SlaveConfigSynchronization(&INPUT_CAPTURE_HTIM, &slaveConfig);

    // input capture
    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
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

    assert_param((1 + num_samples) * 8 <= sizeof(dmabuf));
    dma_num_samples = num_samples;

    HAL_DMA_Abort(&INPUT_CAPTURE_HDMA);
    __HAL_DMA_CLEAR_FLAG(&INPUT_CAPTURE_HDMA, INPUT_CAPTURE_DMA_TC_FLAG);

    HAL_DMA_Start(&INPUT_CAPTURE_HDMA, (uint32_t) &INPUT_CAPTURE_TIMER->DMAR, (uint32_t) dmabuf, (1 + num_samples) * 2);
}

void HWSetGeneratorPWM(uint16_t prescaler, uint16_t period, uint16_t pulse_time, int phase) {
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

void HWStartTimeframe(uint32_t duration) {
    // TODO: this should be more flexible and not need TIMEFRAME_PRESCALER,
    // TODO: deriving the necessary values from SystemCoreClock instead

    // configure parameters
    TIMEFRAME_TIM->ARR = 65535;
    TIMEFRAME_TIM->PSC = SystemCoreClock / 1000 - 1;
    TIMEFRAME_TIM->CCR1 = duration / TIMEFRAME_PRESCALER;//-1;
    TIMEFRAME_TIM->CCMR1 = (0b110 << TIM_CCMR1_OC1M_Pos);
    TIMEFRAME_TIM->CNT = 0;

    // generate Update event
    TIMEFRAME_TIM->EGR = TIM_EGR_UG_Msk;

    // start timer
    TIMEFRAME_TIM->CR1 |= TIM_CR1_CEN;
}

int HWTimeframeElapsed(void) {
    return TIMEFRAME_TIM->CNT > TIMEFRAME_TIM->CCR1;
}

void utilDelayMs(uint32_t milliseconds) {
    HAL_Delay(milliseconds);
}
