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
#include "adc.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
    float x_prev;   // previous input (flow)
    float y_prev;   // previous output (volume)
} TrapezoidIntegrator;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SAMPLE_PERIOD_MS   10u
#define SAMPLING_TIME_S    0.01f   // 10 ms
#define EXHALE_THRESHOLD_LPS   0.05f   // detect blowing (~3 LPM)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint32_t last_sample_time = 0;
uint32_t raw_value = 0;

float flow_slpm = 0.0f;
float flow_lps  = 0.0f;
float volume_l  = 0.0f;
float flow_offset_slpm = 0.0f;   // auto-calculated zero

uint8_t exhale_active = 0;

uint32_t exhale_start_time = 0;
float exhale_start_volume = 0;

float fev1_l = 0.0f;
float fev6_l = 0.0f;

uint8_t fev1_captured = 0;
uint8_t fev6_captured = 0;

float fvc_l = 0.0f;
float fev1_fvc_ratio = 0.0f;
float pef_lps = 0.0f;


TrapezoidIntegrator flow_int = {0};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
float adc_to_slpm(uint32_t adc_value);
float trapezoidal_update(TrapezoidIntegrator *i, float x_now);
float calibrate_flow_offset(void);
void update_fev_parameters(float flow_lps, float volume_l, uint32_t now_ms);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  flow_offset_slpm = calibrate_flow_offset();
  last_sample_time =  HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      uint32_t now = HAL_GetTick();

      if ((now - last_sample_time) >= SAMPLE_PERIOD_MS)
      {
          /* -------- ADC read -------- */
          HAL_ADC_Start(&hadc1);
          HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
          raw_value = HAL_ADC_GetValue(&hadc1);
          HAL_ADC_Stop(&hadc1);

          /* -------- Flow calculation -------- */
          flow_slpm = adc_to_slpm(raw_value);

          /* ---------- offset removal ---------- */
          flow_slpm -= flow_offset_slpm;

          /* ---------- deadband ---------- */
          if (fabsf(flow_slpm) < 1.0f)   // 1 SLPM noise zone
              flow_slpm = 0.0f;

          /* ---------- convert ---------- */
          flow_lps = flow_slpm / 60.0f;

          /* ---------- integrate ---------- */
          volume_l = trapezoidal_update(&flow_int, flow_lps);

          /* compute FEV metrics */
          update_fev_parameters(flow_lps, volume_l, now);

          last_sample_time = now;
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* Convert ADC -> Standard Liters Per Minute */
float adc_to_slpm(uint32_t adc_value)
{
    float voltage = (adc_value * 3.3f) / 4095.0f;

    float flow = 51.19f * voltage - 18.94f;

    if (flow < 0.0f)   flow = 0.0f;
    if (flow > 150.0f) flow = 150.0f;

    return flow;
}


/* Trapezoidal integrator update */
float trapezoidal_update(TrapezoidIntegrator *i, float x_now)
{
    float y_now = i->y_prev + (SAMPLING_TIME_S * 0.5f) * (x_now + i->x_prev);

    i->x_prev = x_now;
    i->y_prev = y_now;

    return y_now;
}

float calibrate_flow_offset(void)
{
    float sum = 0.0f;

    for(int i = 0; i < 200; i++)   // 2 seconds @ 10ms
    {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);

        uint32_t raw = HAL_ADC_GetValue(&hadc1);

        HAL_ADC_Stop(&hadc1);

        sum += adc_to_slpm(raw);

        HAL_Delay(10);
    }

    return sum / 200.0f;
}

void update_fev_parameters(float flow_lps, float volume_l, uint32_t now_ms)
{
    static uint32_t low_flow_start = 0;

    /* -------- Detect start of exhalation -------- */
    if (!exhale_active && flow_lps > EXHALE_THRESHOLD_LPS)
    {
        exhale_active = 1;

        exhale_start_time = now_ms;
        exhale_start_volume = volume_l;

        fev1_captured = 0;
        fev6_captured = 0;

        /* reset metrics */
        fvc_l = 0.0f;
        pef_lps = 0.0f;
    }

    /* -------- During exhalation -------- */
    if (exhale_active)
    {
        uint32_t elapsed = now_ms - exhale_start_time;

        float exhaled_volume = volume_l - exhale_start_volume;

        /* -------- FVC (max volume) -------- */
        if (exhaled_volume > fvc_l)
            fvc_l = exhaled_volume;

        /* -------- PEF (max flow) -------- */
        if (flow_lps > pef_lps)
            pef_lps = flow_lps;

        /* -------- FEV1 -------- */
        if (!fev1_captured && elapsed >= 1000)
        {
            fev1_l = exhaled_volume;
            fev1_captured = 1;
        }

        /* -------- FEV6 -------- */
        if (!fev6_captured && elapsed >= 6000)
        {
            fev6_l = exhaled_volume;
            fev6_captured = 1;
        }

        /* -------- End detection with hysteresis -------- */
        if (flow_lps < EXHALE_THRESHOLD_LPS)
        {
            if (low_flow_start == 0)
                low_flow_start = now_ms;

            if ((now_ms - low_flow_start) > 300)
            {
                exhale_active = 0;
                low_flow_start = 0;

                /* compute ratio after exhale ends */
                if (fvc_l > 0.001f)
                    fev1_fvc_ratio = fev1_l / fvc_l;
            }
        }
        else
        {
            low_flow_start = 0;
        }
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
