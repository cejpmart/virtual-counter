#ifndef VIRTUALINSTRUMENT_HW_STM32F042_H_
#define VIRTUALINSTRUMENT_HW_STM32F042_H_

#include <stm32f042x6.h>

#define PWM1_TIM        TIM14
#define PWM1_HTIM       htim14
#define PWM1_CCR        (TIM14->CCR1)
#define PWM1_CHANNEL    TIM_CHANNEL_1

#define PWM2_TIM        TIM17
#define PWM2_HTIM       htim17
#define PWM2_CCR        (TIM17->CCR1)
#define PWM2_CHANNEL    TIM_CHANNEL_1

#define TIMEFRAME_TIM               TIM3
#define TIMEFRAME_HTIM              htim3
#define TIMEFRAME_PRESCALER         1

#define COUNTER_TIM                 TIM2
#define COUNTER_HTIM                htim2
// ITR2 = TIM3
#define COUNTER_TIM_GATE_IT         TIM_TS_ITR2

#define INPUT_CAPTURE_TIMER         TIM2
#define INPUT_CAPTURE_HTIM          htim2

#define INPUT_CAPTURE_HDMA          hdma_tim2_ch2
#define INPUT_CAPTURE_DMA_TC_FLAG   DMA_FLAG_TC3

// For reciprocal mode
#define INPUT_CAPTURE_RISING_CHAN   TIM_CHANNEL_2
#define INPUT_CAPTURE_RISING_CCR    (TIM2->CCR2)
#define INPUT_CAPTURE_RISING_CCIF   TIM_SR_CC2IF

#define INPUT_CAPTURE_FALLING_CHAN  TIM_CHANNEL_3
#define INPUT_CAPTURE_FALLING_CCR   (TIM2->CCR3)

// For interval mode
#define INPUT_CAPTURE_CH1_CHAN      TIM_CHANNEL_3
#define INPUT_CAPTURE_CH2_CHAN      TIM_CHANNEL_2

#define FREQ_RATIO_GATE_TIM         TIM3
#define FREQ_RATIO_GATE_CHANNEL     TIM_CHANNEL_1

#endif /* VIRTUALINSTRUMENT_HW_STM32F042_H_ */
