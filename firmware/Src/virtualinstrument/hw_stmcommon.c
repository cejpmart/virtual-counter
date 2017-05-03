#include "hw.h"

#include "stm32f0xx_hal.h"

// ugh! can we fix this?
extern TIM_HandleTypeDef INPUT_CAPTURE_HTIM;

void HWClearPeriodMeasurement(void) {
    //meas_rec_valid = 0;
    //lastRiseTime = 0;
}

void HWClearPulseCounter(void) {
    INPUT_CAPTURE_TIMER->CNT = 0;
    INPUT_CAPTURE_TIMER->CR1 |= TIM_CR1_CEN_Msk;
}

int HWGetPeriodPulseWidth(unsigned int* period_out, unsigned int* pulse_width_out) {
    if (!(INPUT_CAPTURE_TIMER->SR & INPUT_CAPTURE_RISING_CCIF))
        return 0;

    // +2 is correct, but why and how?
    __disable_irq();
    *period_out = INPUT_CAPTURE_RISING_CCR + 2;
    *pulse_width_out = INPUT_CAPTURE_FALLING_CCR + 2;
    __enable_irq();

    return 1;
}

void HWInitReciprocalMeasurement(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig;
    TIM_IC_InitTypeDef sConfigIC;

    // clock
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&INPUT_CAPTURE_HTIM, &sClockSourceConfig);

    // slave mode reset
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
