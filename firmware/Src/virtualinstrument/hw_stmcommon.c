#include "hw.h"

#include "stm32f0xx_hal.h"

#define INPUT_CAPTURE_TIMER_MASK 0xffffffff

static uint32_t lastRiseTime;

static volatile uint32_t meas_rec_period, meas_rec_pulseWidth;
static volatile uint32_t meas_rec_valid;

// TODO
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == INPUT_CAPTURE_TIMER) {
        uint32_t riseTime = INPUT_CAPTURE_RISING_CCR;
        uint32_t fallTime = INPUT_CAPTURE_FALLING_CCR;

        //meas_riseTime = riseTime;
        //meas_fallTime = fallTime;
        meas_rec_pulseWidth = ((fallTime - riseTime) & INPUT_CAPTURE_TIMER_MASK);
        meas_rec_period = ((riseTime - lastRiseTime) & INPUT_CAPTURE_TIMER_MASK);
        meas_rec_valid = (lastRiseTime != 0);
        lastRiseTime = riseTime;

        //meas_tdelta_edge1 = INPUT_CAPTURE_CH1_CCR;
        //meas_tdelta_edge2 = INPUT_CAPTURE_CH2_CCR;
        //meas_tdelta_period1 = (meas_tdelta_edge1 - lastEdge1);
        //meas_tdelta_period2 = (meas_tdelta_edge2 - lastEdge2);
        //meas_tdelta_valid = 1;
        //lastEdge1 = meas_tdelta_edge1;
        //lastEdge2 = meas_tdelta_edge2;
    }
}

void HWClearPeriodMeasurement(void) {
    meas_rec_valid = 0;
    lastRiseTime = 0;
}

void HWClearPulseCounter(void) {
    INPUT_CAPTURE_TIMER->CNT = 0;
    INPUT_CAPTURE_TIMER->CR1 |= TIM_CR1_CEN_Msk;
}

int HWGetPeriodPulseWidth(unsigned int* period_out, unsigned int* pulse_width_out) {
    if (!meas_rec_valid)
        return 0;

    __disable_irq();
    *period_out = meas_rec_period;
    *pulse_width_out = meas_rec_pulseWidth;
    __enable_irq();

    return 1;
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
