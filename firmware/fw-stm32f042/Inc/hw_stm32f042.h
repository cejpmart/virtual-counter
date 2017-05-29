#ifndef VIRTUALINSTRUMENT_HW_STM32F042_H_
#define VIRTUALINSTRUMENT_HW_STM32F042_H_

#include <stm32f042x6.h>

#ifdef STM32F042F6
#define PORT_PULSE_COUNT            "6"
#define PORT_PERIOD_1               "6"
#define PORT_PERIOD_2               "7"
#define PORT_INTERVAL_A             "6"
#define PORT_INTERVAL_B             "7"
#define PORT_FREQ_RATIO_A           "6"
#define PORT_FREQ_RATIO_B           "12"
#define PORT_PWM_A                  "10"
#define PORT_PWM_B                  "13"
#endif

#ifdef STM32F042K6
#define PORT_PULSE_COUNT            "A0"
#define PORT_PERIOD_1               "A0"
#define PORT_PERIOD_2               "A1"
#define PORT_INTERVAL_A             "A0"
#define PORT_INTERVAL_B             "A1"
#define PORT_FREQ_RATIO_A           "A0"
#define PORT_FREQ_RATIO_B           "A5"
#define PORT_PWM_A                  "A3"
#define PORT_PWM_B                  "A6"
#endif

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

#define TIMEFRAME_CHAN              TIM_CHANNEL_2
#define TIMEFRAME_CCR               TIMEFRAME_TIM->CCR2
//#define TIMEFRAME_CCMR              TIMEFRAME_TIM->CCMR2

#define COUNTER_TIM                 TIM2
#define COUNTER_HTIM                htim2
// ITR2 = TIM3
#define COUNTER_TIM_GATE_IT         TIM_TS_ITR2

#define INPUT_CAPTURE_TIMER         TIM2
#define INPUT_CAPTURE_HTIM          htim2

#define INPUT_CAPTURE_HDMA          hdma_tim2_ch1
#define INPUT_CAPTURE_DMABASE       TIM_DMABASE_CCR1
#define INPUT_CAPTURE_DMA_TRIGGER   TIM_DMA_CC1
#define INPUT_CAPTURE_DMA_TC_FLAG   DMA_FLAG_TC5

// For reciprocal mode
#define INPUT_CAPTURE_RISING_CHAN   TIM_CHANNEL_1
#define INPUT_CAPTURE_RISING_CCR    (TIM2->CCR1)
#define INPUT_CAPTURE_RISING_CCIF   TIM_SR_CC1IF

#define INPUT_CAPTURE_FALLING_CHAN  TIM_CHANNEL_2
#define INPUT_CAPTURE_FALLING_CCR   (TIM2->CCR2)

#define INPUT_CAPTURE_AUTO_RESET_TRIGGER    TIM_TS_TI1FP1

// For interval mode
#define INPUT_CAPTURE_CH1_CHAN      TIM_CHANNEL_2
#define INPUT_CAPTURE_CH1_CCR       (TIM2->CCR2)
#define INPUT_CAPTURE_CH1_FLAG      TIM_SR_CC2IF

#define INPUT_CAPTURE_CH2_CHAN      TIM_CHANNEL_1
#define INPUT_CAPTURE_CH2_CCR       (TIM2->CCR1)

#define FREQ_RATIO_GATE_TIM         TIM3
#define FREQ_RATIO_GATE_CHANNEL     TIM_CHANNEL_1

#endif /* VIRTUALINSTRUMENT_HW_STM32F042_H_ */
