#include "scope.h"
#include "main.h"
#include "config.h"

//buffers
volatile uint16_t adc_buf[ADC_BUF_LEN];
volatile uint16_t display_buf[DISPLAY_BUF_LEN];

//flags
volatile uint8_t adc_half_ready = 0;
volatile uint8_t adc_full_ready = 0;
volatile uint8_t display_buf_ready = 0;

static uint32_t scope_timer_clk_hz = 0;
uint32_t scope_current_fs = 0;

static uint16_t trigger_level = 2048;
static uint8_t triggered = 0;
static uint32_t trigger_index = 0;
static uint32_t samples_after_trigger = 0;
static uint16_t prev_sample = 0;

static void Scope_CopyTriggeredWindow(uint32_t trig_idx);
static void Scope_ProcessSamples(uint32_t start, uint32_t end);

void Scope_Init(uint32_t timer_clk_hz)
{
    scope_timer_clk_hz = timer_clk_hz;
}

void Scope_SetTriggerLevel(uint16_t level)
{
    trigger_level = level;
}

void Scope_Start(void)
{
    adc_half_ready = 0;
    adc_full_ready = 0;
    display_buf_ready = 0;

    triggered = 0;
	trigger_index = 0;
	samples_after_trigger = 0;
	prev_sample = adc_buf[ADC_BUF_LEN - 1];

    HAL_TIM_Base_Stop(&htim2);
    HAL_ADC_Stop_DMA(&hadc1);
    __HAL_TIM_SET_COUNTER(&htim2, 0);


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

static void Scope_CopyTriggeredWindow(uint32_t trig_idx)
{
    int32_t start = (int32_t)trig_idx - PRE_TRIGGER_SAMPLES;

    for (uint32_t i = 0; i < DISPLAY_BUF_LEN; i++)
    {
        int32_t src_idx = start + (int32_t)i;

        while (src_idx < 0)
        {
            src_idx += ADC_BUF_LEN;
        }

        src_idx %= ADC_BUF_LEN;
        display_buf[i] = adc_buf[src_idx];
    }

    display_buf_ready = 1;
    Scope_Stop();
}

static void Scope_ProcessSamples(uint32_t start, uint32_t end)
{
    for (uint32_t i = start; i < end; i++)
    {
        uint16_t sample = adc_buf[i];

        if (!triggered)
        {
            if ((prev_sample < trigger_level) && (sample >= trigger_level))
            {
                triggered = 1;
                trigger_index = i;
                samples_after_trigger = 0;
            }
        }
        else
        {
            samples_after_trigger++;

            if (samples_after_trigger >= POST_TRIGGER_SAMPLES)
            {
                Scope_CopyTriggeredWindow(trigger_index);
                return;
            }
        }

        prev_sample = sample;
    }
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {

        adc_half_ready = 1;
        Scope_ProcessSamples(0, ADC_BUF_LEN / 2);
//        print_msg("DMA half done\r\n");
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        adc_full_ready = 1;
        Scope_ProcessSamples(ADC_BUF_LEN / 2, ADC_BUF_LEN);
//        Scope_Stop();
//        print_msg("DMA done\r\n");
    }
}
