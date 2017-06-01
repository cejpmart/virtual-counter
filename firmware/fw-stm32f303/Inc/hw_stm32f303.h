#ifndef VIRTUALINSTRUMENT_HW_STM32F303_H_
#define VIRTUALINSTRUMENT_HW_STM32F303_H_

#include <stm32f303xe.h>

#define PORT_PULSE_COUNT            "A0"
#define PORT_PERIOD_1               "A0"
#define PORT_PERIOD_2               "A1"
#define PORT_INTERVAL_A             "A0"
#define PORT_INTERVAL_B             "A1"
#define PORT_FREQ_RATIO_A           "A0"
#define PORT_FREQ_RATIO_B           "D5"
#define PORT_PWM_A                  "D12"
#define PORT_PWM_B                  "D11"

#define PWM1_TIM        TIM16
#define PWM1_HTIM       htim16
#define PWM1_CCR        (TIM16->CCR1)
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

#if 0
#define PERIOD_SETUP_16_32
#else
#define PERIOD_SETUP_32_16_16
#define PERIOD_GATE_TIM             TIM2
#define PERIOD_GATE_TS              TIM_TS_TI1FP1

#define PERIOD_COUNTER_LO_TIM       TIM1
#define PERIOD_COUNTER_LO_TS        TIM_TS_ITR1

#define PERIOD_COUNTER_HI_TIM       TIM3
#define PERIOD_COUNTER_HI_TS        TIM_TS_ITR0
#endif

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
#define FREQ_RATIO_GATE_TS          TIM_TS_TI1FP1

#endif /* VIRTUALINSTRUMENT_HW_STM32F042_H_ */
