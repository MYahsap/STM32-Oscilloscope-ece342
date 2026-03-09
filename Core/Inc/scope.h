#ifndef SCOPE_H
#define SCOPE_H

#include "main.h"
#include <stdint.h>

#define ADC_BUF_LEN 128

extern volatile uint16_t adc_buf[ADC_BUF_LEN];
extern volatile uint8_t adc_half_ready;
extern volatile uint8_t adc_full_ready;
extern uint32_t scope_current_fs;

/* these handles are created by CubeMX in main.c */
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;

void Scope_Init(uint32_t timer_clk_hz);
void Scope_Start(void);
void Scope_Stop(void);
void Scope_SetSamplingFreq(uint32_t fs_hz);

#endif
