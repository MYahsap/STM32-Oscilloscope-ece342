#ifndef SCOPE_H
#define SCOPE_H

#include "main.h"
#include <stdint.h>

#define ADC_BUF_LEN            512
#define DISPLAY_BUF_LEN        128

#define PRE_TRIGGER_SAMPLES    32
#define POST_TRIGGER_SAMPLES   (DISPLAY_BUF_LEN - PRE_TRIGGER_SAMPLES - 1)

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;

extern volatile uint16_t adc_buf[ADC_BUF_LEN];
extern volatile uint16_t display_buf[DISPLAY_BUF_LEN];

extern volatile uint8_t adc_half_ready;
extern volatile uint8_t adc_full_ready;
extern volatile uint8_t display_buf_ready;

extern uint32_t scope_current_fs;

void Scope_Init(uint32_t timer_clk_hz);
void Scope_Start(void);
void Scope_Stop(void);
void Scope_SetSamplingPeriodUs(uint32_t period_us);
void Scope_SetTriggerLevel(uint16_t level);


#endif
