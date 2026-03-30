/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <math.h>
#include "config.h"
#include "scope.h"
#include "ssd1306.h"
#include "fonts.h"
#include "background.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart3;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

/* USER CODE BEGIN PV */
//OLED Screen
SSD1306_t oled1;
SSD1306_t oled2;

//Scope Settings
uint32_t hdiv = 200; //in us
int16_t hoffset = 0; //in us
uint32_t vdiv = 2000; //in mV
int16_t voffset = 0; //in mV
uint32_t triglvl = 2500; //in adc

//scope
extern volatile uint16_t adc_buf[ADC_BUF_LEN];
extern volatile uint8_t adc_half_ready;
extern volatile uint8_t adc_full_ready;

//test wave
uint16_t testSin[128];
char msg[64];

//OLED 2 stuff--------------------------------------------

//Buttons & Timing Flags
volatile uint32_t last_press_time = 0;
volatile uint8_t btn1_pressed = 0;
volatile uint8_t btn2_pressed = 0;
volatile uint8_t btn3_pressed = 0;
volatile uint8_t btn4_pressed = 0;
uint8_t update_display_flag = 1;

//Measurement Variables
float freq = 0.0, v_max = 0.0, v_min = 0.0, v_pp = 0.0;

//trigger mode
volatile uint8_t trigger_mode = 0;//0 = Auto, 1 = Normal (Triggered)
uint32_t trig_timeout = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
void Calculate_Measurements(void);
void Update_OLED_Measurements(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Calculate_Measurements(void) {
    uint16_t max_adc = 0;
    uint16_t min_adc = 4095;
    int zero_crossings = 0;

    for (int i = 0; i < DISPLAY_BUF_LEN; i++) {
        if (display_buf[i] > max_adc) max_adc = display_buf[i];
        if (display_buf[i] < min_adc) min_adc = display_buf[i];

        // Zero-crossing (centered at 2048)
        if (i > 0) {
            //Only count rising edge crossing to get full cycles
            if (display_buf[i-1] < 2048 && display_buf[i] >= 2048) {
                zero_crossings++;
            }
        }
    }

    // Vpp Calculation
    v_max = ((float)max_adc / 4095.0f) * 3.3f;
    v_min = ((float)min_adc / 4095.0f) * 3.3f;
    v_pp = v_max - v_min;

    // Frequency Fix:
    if (zero_crossings > 0 && hdiv > 0) {
        //use 128 samples. If each sample is (hdiv/32) microseconds:
        float total_time_us = 128.0f * ((float)hdiv / 32.0f); 
        freq = (float)zero_crossings * (1000000.0f / total_time_us);
    } else {
        freq = 0;
    }
}

void Update_OLED_Measurements(void){
    SSD1306_Fill(&oled2, SSD1306_COLOR_BLACK);
    SSD1306_SetCursor(&oled2, 0, 0);
    //had to correct hte way the output sprintf is done
    char buf[32];
    //Vertical Scale
    sprintf(buf, "V: %lumV/div", vdiv);
    SSD1306_Puts(&oled2, buf, &Font_7x10);

    //Horizontal Scale
    SSD1306_SetCursor(&oled2, 0, 12);
    sprintf(buf, "H: %luus/div", hdiv);
    SSD1306_Puts(&oled2, buf, &Font_7x10);

    //Vpp (Integer + 2 decimals)
    SSD1306_SetCursor(&oled2, 0, 30);
    int vpp_i = (int)v_pp;
    int vpp_d = (int)((v_pp - vpp_i) * 100);
    sprintf(buf, "Vpp: %d.%02dV", vpp_i, vpp_d);
    SSD1306_Puts(&oled2, buf, &Font_7x10);

    //Frequency
    SSD1306_SetCursor(&oled2, 0, 42);
    sprintf(buf, "Freq: %dHz", (int)freq);
    SSD1306_Puts(&oled2, buf, &Font_7x10);

    //Vertical Header on right side
    const char* side_text = "SCOPE";
    for (int i = 0; side_text[i] != '\0'; i++) {
        SSD1306_SetCursor(&oled2, 115, 5 + (i * 12));
        char temp[2] = {side_text[i], '\0'};
        SSD1306_Puts(&oled2, temp, &Font_7x10);
    }

    //trigger
    SSD1306_SetCursor(&oled2, 0, 54);
    if (trigger_mode) {
        SSD1306_Puts(&oled2, "MODE: NORM-TRIG", &Font_7x10);
    } else {
        SSD1306_Puts(&oled2, "MODE: AUTO", &Font_7x10);
    }

    SSD1306_UpdateScreen(&oled2);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  //SSD2306 Init
  if (SSD1306_Init(&oled1, &hi2c1, SSD1306_I2C_ADDR) != 0) Error_Handler();
  if (SSD1306_Init(&oled2, &hi2c2, SSD1306_I2C_ADDR) != 0) Error_Handler();
  SSD1306_Clear(&oled1);
  SSD1306_Clear(&oled2);
  SSD1306_UpdateScreen(&oled1);
  SSD1306_UpdateScreen(&oled2);

  //Scope Init
  Scope_Init(84000000);    // example timer clock, use your real TIM2 input clock
  Scope_SetSamplingPeriodUs(hdiv/32);   // 100 us = 10 kHz
  Scope_SetTriggerLevel(triglvl);
  Scope_Start();

  //Test oled
  SSD1306_SetCursor(&oled2, 0, 0);
  SSD1306_Puts(&oled2, "OLED on I2C2 \r\n settings", &Font_7x10);
  SSD1306_UpdateScreen(&oled2);
  //oled 2 home screen!!!
  SSD1306_Clear(&oled2);
  SSD1306_SetCursor(&oled2, 0, 0);
  SSD1306_Puts(&oled2,"ECE342\nPRJECT DEMO", &Font_11x18);
  SSD1306_SetCursor(&oled2, 0, 50);
  //SSD1306_GotoXY(0, 50);
  SSD1306_Puts(&oled2,"By Thariq n Yusuf", &Font_7x10);
  SSD1306_UpdateScreen(&oled2);

  SSD1306_Scroll(&oled2, SSD1306_SCROLL_RIGHT, 0, 7);
  HAL_Delay(3500);
  SSD1306_Scroll(&oled2, SSD1306_SCROLL_LEFT, 0, 7);
  HAL_Delay(3500);
  SSD1306_Stopscroll(&oled2);
  //oend of oled  2 home screen!!!

  buffer_Set(&oled1, Scopebackground);
  for (int i = 0; i < 128; i++)testSin[i] = (900 * (sin(i / 10.0)))+2048; // ~ 2.2V
  draw_Wave(&oled1, testSin, vdiv, voffset);
  SSD1306_UpdateScreen(&oled1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
//	  if (HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin) == GPIO_PIN_SET){
//		  print_msg("trig!\r\n");
//		  sprintf(msg, "PSC=%lu ARR=%lu fs=%lu\r\n",
//		          htim2.Init.Prescaler,
//		          __HAL_TIM_GET_AUTORELOAD(&htim2),
//		          scope_current_fs);
//		  print_msg(msg);
//
//
//		  uint32_t t0, t1;
//
//		  adc_full_ready = 0;
//		  adc_half_ready = 0;
//		  Scope_Start();
//
//		  t0 = HAL_GetTick();
//
//		  while (adc_half_ready == 0){}
//		  t1 = HAL_GetTick();
//		  sprintf(msg, "half time = %lu ms\r\n", t1 - t0);
//		  print_msg(msg);
//
//		  while (adc_full_ready == 0){}
//		  t1 = HAL_GetTick();
//		  sprintf(msg, "full time = %lu ms\r\n", t1 - t0);
//		  print_msg(msg);
//
//		  for (int i = 0; i < ADC_BUF_LEN; i++) {
//			  sprintf(msg, "%u ", adc_buf[i]);
//			  print_msg(msg);
//		  }
//		  print_msg("\r\n");
//
//		  buffer_Set(&oled1, Scopebackground);
//		  draw_Wave(&oled1, adc_buf, vdiv, voffset);
//		  SSD1306_UpdateScreen(&oled1);
//	  }

    //oled 2 settigs
    //1) Check for Hardware Button Inputs (Port E, Pins 10-13)
    if (btn1_pressed){ // Reset 
        vdiv = 2000; hdiv = 200;
        Scope_SetSamplingPeriodUs(hdiv/32);
        btn1_pressed = 0; 
        update_display_flag = 1;
    }
    if (btn2_pressed){ // Reserved for future use (e.g. Pause/Run)
        trigger_mode = !trigger_mode;//toggle
        btn2_pressed = 0;
        update_display_flag = 1;

    }
    if (btn3_pressed){//Vertical Scale
        vdiv += 500;
        if(vdiv > 5000) vdiv = 500;
        btn3_pressed = 0; 
        update_display_flag = 1;
    }
    if (btn4_pressed){//Horizontal Scale
        hdiv += 100;
        if(hdiv > 1000) hdiv = 100;
        Scope_SetSamplingPeriodUs(hdiv/32);//Tell hardware to slow down sampling
        btn4_pressed = 0; 
        update_display_flag = 1;
    }


	  if (display_buf_ready)
	      {
	          display_buf_ready = 0;

            Calculate_Measurements();// calcualtions 

            //trigg part 
            if(!trigger_mode || v_pp > 0.1f){
              buffer_Set(&oled1, Scopebackground);
              draw_Wave(&oled1, (uint16_t*)display_buf, vdiv, voffset);
              SSD1306_UpdateScreen(&oled1);
              Update_OLED_Measurements();
            }
/*
	          buffer_Set(&oled1, Scopebackground);
//	          SSD1306_Clear(&oled1);

			  draw_Wave(&oled1, display_buf, vdiv, voffset);
			  SSD1306_UpdateScreen(&oled1);

        Update_OLED_Measurements();//update OLED 2...
        */

			  for (int i = 0; i < DISPLAY_BUF_LEN; i++)
				  {
					  sprintf(msg, "%u ", display_buf[i]);
					  print_msg(msg);
				  }
			  print_msg("\r\n");
	          HAL_Delay(100);
	          Scope_Start();
	      }

    //Forced UI refresh if settings changed but no signal present
    if (update_display_flag) {
        Update_OLED_Measurements();
        update_display_flag = 0;
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : btn1_Pin btn2_Pin btn3_Pin btn4_Pin */
  GPIO_InitStruct.Pin = btn1_Pin|btn2_Pin|btn3_Pin|btn4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t now = HAL_GetTick();
    //Debouncing; 200ms
    if (now - last_press_time > 200) {
        if (GPIO_Pin == GPIO_PIN_10) btn1_pressed = 1;
        else if (GPIO_Pin == GPIO_PIN_11) btn2_pressed = 1;
        else if (GPIO_Pin == GPIO_PIN_12) btn3_pressed = 1;
        else if (GPIO_Pin == GPIO_PIN_13) btn4_pressed = 1;
        
        last_press_time = now;
        HAL_GPIO_TogglePin(GPIOB, LD1_Pin); //Toggle LED to see if press is recognised
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */