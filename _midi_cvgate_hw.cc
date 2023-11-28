
#include "midi.h"
#include "midi_cvgate.h"
// #include "stm32f4xx_hal.h"
// #include "stm32f4xx.h"

midi_cvout_voiceblock_t cvgate_vb1;

uint32_t* const BB_GATE_1_OFF = (uint32_t*)(PERIPH_BB_BASE + (GPIOD_BASE + 0x14) * 32 + 3 * 4);
uint32_t* const BB_GATE_2_OFF = (uint32_t*)(PERIPH_BB_BASE + (GPIOD_BASE + 0x14) * 32 + 6 * 4);
uint32_t* const BB_GATE_3_OFF = (uint32_t*)(PERIPH_BB_BASE + (GPIOD_BASE + 0x14) * 32 + 7 * 4);
uint32_t* const BB_GATE_4_OFF = (uint32_t*)(PERIPH_BB_BASE + (GPIOD_BASE + 0x14) * 32 + 10 * 4);

void cvgate_start()
{
    GPIO_InitTypeDef GPIO_InitStruct;
    // PA0  - TIM5_CH1 dac 3 low
    // PA1  - TIM5_CH2 dac 3 high
    // PA2  - TIM5_CH3 dac 4 low
    // PA3  - TIM5_CH4 dac 4 high
    // PA4  - DAC1
    // PA5  - DAC2
    // PD3  - GPIO_O gate 1
    // PD6  - GPIO_O gate 2
    // PD7  - GPIO_O gate 3
    // PD10 - GPIO_O gate 4
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_TIM5_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    *BB_GATE_1_OFF = 1;
    *BB_GATE_2_OFF = 1;
    *BB_GATE_3_OFF = 1;
    *BB_GATE_4_OFF = 1;

    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    DAC->CR |= DAC_CR_MAMP2 | DAC_CR_TSEL2 | DAC_CR_EN2 | DAC_CR_MAMP1 | DAC_CR_TSEL1 | DAC_CR_EN1;
    DAC->DHR12L1 = 0x8000;
    DAC->DHR12L2 = 0x8000;
    DAC->SWTRIGR = 0x3;

    // APB1 = 42MHz
    TIM5->CR1 |= TIM_CR1_ARPE | TIM_CR1_CKD_1;
    TIM5->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2;
    TIM5->CCMR2 |= TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2;
    TIM5->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;
    TIM5->ARR = 0xFF; // 164062.5 Hz

    TIM5->CCR1 = 0x00; // CH3L
    TIM5->CCR2 = 0x80; // CH3H
    TIM5->CCR3 = 0x08; // CH4L
    TIM5->CCR4 = 0x80; // CH4H

    TIM5->CR1 |= TIM_CR1_CEN;

    // start sw
    midi_voiceblock_init(&cvgate_vb1);
}

void cvgate_send(MidiMessageT m)
{
    midi_cv_transmit(&cvgate_vb1, m);
}

static inline uint16_t scale_voltage(uint16_t in, uint16_t scl, int16_t off)
{
    int32_t v = (int32_t)in * scl / 16384 + off;
    return __USAT(v, 15);
}

// TODO: make better?
void cvgate_update()
{
    midi_cr_cvgprocess(&cvgate_vb1);
    uint16_t v;

    v = cvgate_vb1.out[MIDI_CVGVB_CH_PITCH];
    v = scale_voltage(v, 16384, 0);
    DAC->DHR12L1 = v;

    v = cvgate_vb1.out[MIDI_CVGVB_CH_MODWHEEL];
    v = scale_voltage(v, 16384, 0);
    DAC->DHR12L2 = v;
    DAC->SWTRIGR = 0x3;

    v = cvgate_vb1.out[MIDI_CVGVB_CH_VELO];
    v = scale_voltage(v, 16384, 0);
    TIM5->CCR1 = (uint8_t)v; // CH3L
    TIM5->CCR2 = (uint8_t)(v >> 8); // CH3H

    v = cvgate_vb1.out[MIDI_CVGVB_CH_AFTERTOUCH];
    v = scale_voltage(v, 16384, 0);
    TIM5->CCR3 = (uint8_t)v; // CH4L
    TIM5->CCR4 = (uint8_t)(v >> 8); // CH4H
    // TODO: compare treshold
    *BB_GATE_1_OFF = 1;
    *BB_GATE_2_OFF = 1;
    *BB_GATE_3_OFF = 1;
    *BB_GATE_4_OFF = 1;
}