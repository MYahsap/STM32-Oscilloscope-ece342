#include "scope.h"
#include "main.h"

volatile uint16_t adc_buf[ADC_BUF_LEN];
volatile uint8_t adc_half_ready = 0;
volatile uint8_t adc_full_ready = 0;

static uint32_t scope_timer_clk_hz = 0;
uint32_t scope_current_fs = 0;

void Scope_Init(uint32_t timer_clk_hz)
{
    scope_timer_clk_hz = timer_clk_hz;
}

void Scope_Start(void)
{
    adc_half_ready = 0;
    adc_full_ready = 0;

    HAL_TIM_Base_Stop(&htim2);
    HAL_ADC_Stop_DMA(&hadc1);

    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, ADC_BUF_LEN) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_Base_Start(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
}

void Scope_Stop(void)
{
    HAL_TIM_Base_Stop(&htim2);
    HAL_ADC_Stop_DMA(&hadc1);
}

void Scope_SetSamplingPeriodUs(uint32_t period_us)
{
    if (period_us == 0)
    {
        period_us = 1;
    }

    uint32_t psc = htim2.Init.Prescaler;
    uint32_t tim_cnt_clk = scope_timer_clk_hz / (psc + 1);

//  ARR = (period_us * tim_cnt_clk)/1e6 - 1
    uint32_t arr = ((uint64_t)period_us * tim_cnt_clk) / 1000000ULL;

    if (arr == 0)
    {
        arr = 1;
    }

    arr = arr - 1;

    __HAL_TIM_DISABLE(&htim2);
    __HAL_TIM_SET_AUTORELOAD(&htim2, arr);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    __HAL_TIM_ENABLE(&htim2);

    scope_current_fs = tim_cnt_clk / (arr + 1);
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        adc_half_ready = 1;
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        adc_full_ready = 1;
    }
}
