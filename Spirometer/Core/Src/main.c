/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "max30102_for_stm32_hal.h"
#include "string.h"
#include "stdio.h"
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SPO2_BUFFER_SIZE 100
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
float flow;
float rawValue;
max30102_t max30102;
uint32_t ir_buffer[SPO2_BUFFER_SIZE] = {0};
uint32_t red_buffer[SPO2_BUFFER_SIZE] = {0};
int spo2_index = 0;
bool buffer_filled = false;  // Track if buffer is full
char name[] = "Lenin";
uint8_t age = 19;
char dob[] = "26-05-2006";
float flow_value = 0.00;
uint32_t ir_value = 0;
uint32_t red_value = 0;
volatile uint32_t debug_ir_value = 0;
volatile uint32_t debug_red_value = 0;
volatile uint32_t debug_sample_count = 0;
volatile float debug_SpO2 = 0;
uint32_t ir_sample=0;
uint32_t red_sample=0;
void max30102_plot(uint32_t ir_sample, uint32_t red_sample)
{
    // Store raw values
    debug_ir_value = ir_sample;
    debug_red_value = red_sample;
    ir_value = ir_sample;
    red_value = red_sample;

    // Add to circular buffers
    ir_buffer[spo2_index] = ir_sample;
    red_buffer[spo2_index] = red_sample;
    spo2_index = (spo2_index + 1) % SPO2_BUFFER_SIZE;

    // Increment debug counter
    debug_sample_count++;
}

float flowRate(uint16_t rawValue){
    float rawVoltage = ((float)rawValue / 4095.0f) * 3.3f;
    flow = rawVoltage * 34.0f - 26.4f;
    return flow;
}
void send_to_nextion(char *cmd){
    HAL_UART_Transmit(&huart2, (uint8_t*)cmd, strlen(cmd), HAL_MAX_DELAY);
    uint8_t end_bytes[3] = {0xFF, 0xFF, 0xFF};
    HAL_UART_Transmit(&huart2, end_bytes, 3, HAL_MAX_DELAY);
}

void send_measure_data(void) {
    char cmd[50];

    sprintf(cmd, "Page1_Measure.flow_txt.txt=\"%d.%d\"", (int)flow_value, (int)((flow_value - (int)flow_value)*10));
    send_to_nextion(cmd);

    sprintf(cmd, "Page1_Measure.ir_txt.txt=\"%lu\"", ir_value);
    send_to_nextion(cmd);

    sprintf(cmd, "Page1_Measure.red_txt.txt=\"%lu\"", red_value);
    send_to_nextion(cmd);

    sprintf(cmd, "Page1_Measure.spo2_txt.txt=\"%d.%d\"", (int)debug_SpO2, (int)((debug_SpO2 - (int)debug_SpO2)*10));
    send_to_nextion(cmd);
}

void send_profile_data(void) {
    char cmd[50];

    sprintf(cmd, "Page2_Profile.name_txt.txt=\"%s\"", name);
    send_to_nextion(cmd);

    sprintf(cmd, "Page2_Profile.age_txt.txt=\"%d\"", age);
    send_to_nextion(cmd);

    sprintf(cmd, "Page2_Profile.dob_txt.txt=\"%s\"", dob);
    send_to_nextion(cmd);
}

float calculate_ac_dc(uint32_t *buffer, int length, float *dc) {
    uint32_t max = 0, min = 0xFFFFFFFF;
    uint64_t sum = 0;  // Use 64-bit to prevent overflow

    // Find min, max, and sum
    for(int i = 0; i < length; i++){
        if(buffer[i] > max) max = buffer[i];
        if(buffer[i] < min) min = buffer[i];
        sum += buffer[i];
    }

    *dc = (float)sum / length;
    return (float)(max - min);  // AC component
}

float calculate_spo2(uint32_t *red_buf, uint32_t *ir_buf, int len){
    // Don't calculate until we have enough valid samples
    if (!buffer_filled && spo2_index < 50) {  // Need at least 50 samples
        return 0;
    }

    int samples_to_use = buffer_filled ? len : spo2_index;

    float red_dc, ir_dc;
    float red_ac = calculate_ac_dc(red_buf, samples_to_use, &red_dc);
    float ir_ac = calculate_ac_dc(ir_buf, samples_to_use, &ir_dc);

    // Check for division by zero
    if (red_dc < 1.0f || ir_dc < 1.0f || red_ac < 1.0f || ir_ac < 1.0f) {
        return 0;  // Invalid reading
    }

    // Calculate R ratio
    float R = (red_ac / red_dc) / (ir_ac / ir_dc);

    // Apply calibrated SpO2 formula (this may need adjustment for your sensor)
    float spo2 = 110 - 25 * R;

    // Clamp to reasonable values
    if (spo2 > 100) spo2 = 100;
    if (spo2 < 70) spo2 = 0;  // Below 70% is likely invalid

    return spo2;
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
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  max30102_init(&max30102, &hi2c1);
  max30102_reset(&max30102);
  max30102_clear_fifo(&max30102);
  // FIFO configurations
  max30102_set_fifo_config(&max30102, max30102_smp_ave_8, 1, 7);
  // LED configurations
  max30102_set_led_pulse_width(&max30102, max30102_pw_18_bit);
  max30102_set_adc_resolution(&max30102, max30102_adc_4096);
  max30102_set_sampling_rate(&max30102, max30102_sr_200);
  max30102_set_led_current_1(&max30102, 12.5);
  max30102_set_led_current_2(&max30102, 12.5);

  // Enter SpO2 mode
  max30102_set_mode(&max30102, max30102_spo2);

  // Enable FIFO_A_FULL interrupt
  max30102_set_a_full(&max30102, 1);
  // Enable die temperature measurement
  max30102_set_die_temp_en(&max30102, 1);
  // Enable DIE_TEMP_RDY interrupt
  max30102_set_die_temp_rdy(&max30102, 1);

  uint8_t en_reg[2] = {0};
  max30102_read(&max30102, 0x00, en_reg, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	    // Read flow ADC
	    HAL_ADC_Start(&hadc1);
	    HAL_ADC_PollForConversion(&hadc1, 100);
	    rawValue = HAL_ADC_GetValue(&hadc1);
	    flow_value = flowRate(rawValue);   // Update flow_value for Nextion

	    // MAX30102 interrupt handler
	    if (max30102_has_interrupt(&max30102)) {
	        max30102_interrupt_handler(&max30102);
	    }
	    debug_SpO2 = calculate_spo2(red_buffer, ir_buffer, SPO2_BUFFER_SIZE);
	    // You can add a small delay if needed
	    send_measure_data();
	    send_profile_data();
	    HAL_Delay(50);
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;
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
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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

#ifdef  USE_FULL_ASSERT
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
