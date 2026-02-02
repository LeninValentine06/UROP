/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program with FreeRTOS and VISIBLE Performance Metrics
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
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
#include <stdio.h>  // For printf
#include <math.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
    float x_prev;
    float y_prev;
} TrapezoidIntegrator;

typedef struct {
    float volume;
    float flow;
} FVLPoint;

typedef struct {
    float flow_lps;
    float volume_l;
    TickType_t timestamp_ticks;
} SampleData;

/* Performance Metrics Structures */
typedef struct {
    uint32_t total_samples;
    uint32_t missed_deadlines;
    float miss_rate_percent;
} DeadlineMetrics;

typedef struct {
    float latency_sum_ms;
    float latency_max_ms;
    float latency_avg_ms;
    uint32_t sample_count;
} LatencyMetrics;

typedef struct {
    TickType_t jitter_sum;
    TickType_t jitter_max;
    float jitter_avg_ms;
    uint32_t measurement_count;
} JitterMetrics;

typedef struct {
    TickType_t start_tick;
    TickType_t end_tick;
    TickType_t response_time_ticks;
    float response_time_ms;
    float response_time_max_ms;
    uint32_t execution_count;
} TaskResponseMetrics;

typedef struct {
    uint32_t context_switches;
    uint32_t idle_ticks;
    uint32_t total_ticks;
    float cpu_utilization_percent;
    float idle_time_percent;
} CPUMetrics;

typedef struct {
    uint32_t heap_used_bytes;
    uint32_t heap_free_bytes;
    uint32_t heap_min_free_bytes;
} HeapMetrics;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SAMPLE_PERIOD_MS   10u
#define SAMPLING_TIME_S    0.01f
#define EXHALE_THRESHOLD_LPS   0.05f
#define FVL_MAX_POINTS 800

/* Task priorities */
#define SAMPLING_TASK_PRIORITY    (tskIDLE_PRIORITY + 2)
#define SPIROMETRY_TASK_PRIORITY  (tskIDLE_PRIORITY + 1)
#define METRICS_TASK_PRIORITY     (tskIDLE_PRIORITY + 1)

/* Stack sizes */
#define SAMPLING_TASK_STACK_SIZE   256
#define SPIROMETRY_TASK_STACK_SIZE 512
#define METRICS_TASK_STACK_SIZE    256

/* Metrics GPIO Pins (for WCET measurement) */
#define SAMPLING_TASK_PIN    GPIO_PIN_0
#define SPIROMETRY_TASK_PIN  GPIO_PIN_1

/* Choose your output method - uncomment ONE */
//#define USE_ITM_TRACE    // Use ITM/SWO trace (easiest for debugging)
#define USE_UART_PRINTF  // Use UART printf (requires UART setup)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* FreeRTOS Handles */
TaskHandle_t samplingTaskHandle = NULL;
TaskHandle_t spirometryTaskHandle = NULL;
TaskHandle_t metricsTaskHandle = NULL;
QueueHandle_t sampleDataQueue = NULL;
SemaphoreHandle_t dataAccessMutex = NULL;

/* Shared data */
uint32_t raw_value = 0;
float flow_slpm = 0.0f;
float flow_lps  = 0.0f;
float volume_l  = 0.0f;
float flow_offset_slpm = 0.0f;

/* Spirometry variables */
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

FVLPoint fvl_buffer[FVL_MAX_POINTS];
uint16_t fvl_index = 0;

TrapezoidIntegrator flow_int = {0};

/* ========== PERFORMANCE METRICS VARIABLES ========== */
DeadlineMetrics deadline_metrics = {0};
LatencyMetrics latency_metrics = {0};
JitterMetrics jitter_metrics = {0};

TaskResponseMetrics sampling_response = {0};
TaskResponseMetrics spirometry_response = {0};

CPUMetrics cpu_metrics = {0};
HeapMetrics heap_metrics = {0};

UBaseType_t sampling_stack_hwm = 0;
UBaseType_t spirometry_stack_hwm = 0;
UBaseType_t metrics_stack_hwm = 0;

volatile uint32_t idle_counter = 0;
volatile uint32_t total_counter = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
float adc_to_slpm(uint32_t adc_value);
float trapezoidal_update(TrapezoidIntegrator *i, float x_now);
float calibrate_flow_offset(void);
void update_fev_parameters(float flow_lps, float volume_l, uint32_t now_ms);

/* FreeRTOS Task Functions */
void SamplingTask(void *pvParameters);
void SpirometryTask(void *pvParameters);
void MetricsTask(void *pvParameters);

/* Metrics Helper Functions */
void UpdateLatencyMetrics(TickType_t start_tick, TickType_t end_tick);
void UpdateJitterMetrics(TickType_t expected_tick, TickType_t actual_tick);
void UpdateDeadlineMetrics(uint8_t deadline_missed);
void CollectCPUMetrics(void);
void CollectHeapMetrics(void);
void CollectStackMetrics(void);
void PrintMetricsReport(void);

/* Printf redirection for ITM */
#ifdef USE_ITM_TRACE
int _write(int file, char *ptr, int len)
{
    for(int i = 0; i < len; i++)
    {
        ITM_SendChar((*ptr++));
    }
    return len;
}
#endif

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

  /* Enable ITM trace if selected */
  #ifdef USE_ITM_TRACE
  /* Enable TRCENA in Debug Exception and Monitor Control Register */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

  /* Enable stimulus port 0 */
  ITM->LAR = 0xC5ACCE55;
  ITM->TER = 0x1;
  ITM->TCR = ITM_TCR_ITMENA_Msk;
  #endif

  /* Calibrate flow sensor offset */
  flow_offset_slpm = calibrate_flow_offset();

  /* Create FreeRTOS objects */
  sampleDataQueue = xQueueCreate(5, sizeof(SampleData));
  dataAccessMutex = xSemaphoreCreateMutex();

  /* Create Tasks */
  xTaskCreate(SamplingTask,
              "Sampling",
              SAMPLING_TASK_STACK_SIZE,
              NULL,
              SAMPLING_TASK_PRIORITY,
              &samplingTaskHandle);

  xTaskCreate(SpirometryTask,
              "Spirometry",
              SPIROMETRY_TASK_STACK_SIZE,
              NULL,
              SPIROMETRY_TASK_PRIORITY,
              &spirometryTaskHandle);

  xTaskCreate(MetricsTask,
              "Metrics",
              METRICS_TASK_STACK_SIZE,
              NULL,
              METRICS_TASK_PRIORITY,
              &metricsTaskHandle);

  printf("\r\n");
  printf("==========================================\r\n");
  printf("  RTOS SPIROMETRY SYSTEM INITIALIZED\r\n");
  printf("==========================================\r\n");
  printf("Starting scheduler...\r\n\r\n");

  /* Start the scheduler */
  vTaskStartScheduler();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

/**
  * @brief  Stack overflow hook
  */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("\r\n!!! STACK OVERFLOW IN TASK: %s !!!\r\n", pcTaskName);

    __disable_irq();
    while(1);
}

/**
  * @brief  Malloc failed hook
  */
void vApplicationMallocFailedHook(void)
{
    printf("\r\n!!! HEAP ALLOCATION FAILED !!!\r\n");

    __disable_irq();
    while(1);
}

/**
  * @brief  Idle Hook for CPU utilization measurement
  */
void vApplicationIdleHook(void)
{
    idle_counter++;
}

/**
  * @brief  Task 1: Sampling Task - Runs every 10ms
  */
void SamplingTask(void *pvParameters)
{
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = pdMS_TO_TICKS(SAMPLE_PERIOD_MS);

    xLastWakeTime = xTaskGetTickCount();

    SampleData sample;
    TickType_t expected_wake_time;

    for (;;)
    {
        /* ========== METRICS: Task Start ========== */
        sampling_response.start_tick = xTaskGetTickCount();

        /* GPIO HIGH - for WCET measurement with oscilloscope */
        HAL_GPIO_WritePin(GPIOA, SAMPLING_TASK_PIN, GPIO_PIN_SET);

        /* ========== METRICS: Jitter Measurement ========== */
        expected_wake_time = xLastWakeTime + xPeriod;
        TickType_t actual_wake_time = xTaskGetTickCount();
        UpdateJitterMetrics(expected_wake_time, actual_wake_time);

        /* ========== METRICS: Deadline Check ========== */
        uint8_t deadline_missed = (actual_wake_time > expected_wake_time) ? 1 : 0;
        UpdateDeadlineMetrics(deadline_missed);

        /* -------- ADC Read -------- */
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
        raw_value = HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);

        /* -------- Flow Calculation -------- */
        flow_slpm = adc_to_slpm(raw_value);
        flow_slpm -= flow_offset_slpm;

        /* -------- Deadband -------- */
        if (fabsf(flow_slpm) < 1.0f)
            flow_slpm = 0.0f;

        /* -------- Convert to L/s -------- */
        flow_lps = flow_slpm / 60.0f;

        /* -------- Integrate Volume -------- */
        if (xSemaphoreTake(dataAccessMutex, pdMS_TO_TICKS(5)) == pdTRUE)
        {
            volume_l = trapezoidal_update(&flow_int, flow_lps);
            xSemaphoreGive(dataAccessMutex);
        }

        /* -------- Prepare Sample Data -------- */
        sample.flow_lps = flow_lps;
        sample.volume_l = volume_l;
        sample.timestamp_ticks = xTaskGetTickCount();

        /* -------- Send to Spirometry Task -------- */
        xQueueSend(sampleDataQueue, &sample, 0);

        /* ========== METRICS: Task End ========== */
        sampling_response.end_tick = xTaskGetTickCount();
        sampling_response.response_time_ticks = sampling_response.end_tick - sampling_response.start_tick;
        sampling_response.response_time_ms = sampling_response.response_time_ticks * portTICK_PERIOD_MS;

        if (sampling_response.response_time_ms > sampling_response.response_time_max_ms)
            sampling_response.response_time_max_ms = sampling_response.response_time_ms;

        sampling_response.execution_count++;

        /* GPIO LOW */
        HAL_GPIO_WritePin(GPIOA, SAMPLING_TASK_PIN, GPIO_PIN_RESET);

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/**
  * @brief  Task 2: Spirometry Processing Task
  */
void SpirometryTask(void *pvParameters)
{
    SampleData sample;

    for (;;)
    {
        if (xQueueReceive(sampleDataQueue, &sample, portMAX_DELAY) == pdTRUE)
        {
            /* ========== METRICS: Task Start ========== */
            spirometry_response.start_tick = xTaskGetTickCount();

            /* GPIO HIGH */
            HAL_GPIO_WritePin(GPIOA, SPIROMETRY_TASK_PIN, GPIO_PIN_SET);

            /* ========== METRICS: End-to-End Latency ========== */
            TickType_t current_tick = xTaskGetTickCount();
            UpdateLatencyMetrics(sample.timestamp_ticks, current_tick);

            /* -------- Process FEV Parameters -------- */
            if (xSemaphoreTake(dataAccessMutex, pdMS_TO_TICKS(10)) == pdTRUE)
            {
                uint32_t timestamp_ms = sample.timestamp_ticks * portTICK_PERIOD_MS;
                update_fev_parameters(sample.flow_lps, sample.volume_l, timestamp_ms);
                xSemaphoreGive(dataAccessMutex);
            }

            /* ========== METRICS: Task End ========== */
            spirometry_response.end_tick = xTaskGetTickCount();
            spirometry_response.response_time_ticks = spirometry_response.end_tick - spirometry_response.start_tick;
            spirometry_response.response_time_ms = spirometry_response.response_time_ticks * portTICK_PERIOD_MS;

            if (spirometry_response.response_time_ms > spirometry_response.response_time_max_ms)
                spirometry_response.response_time_max_ms = spirometry_response.response_time_ms;

            spirometry_response.execution_count++;

            /* GPIO LOW */
            HAL_GPIO_WritePin(GPIOA, SPIROMETRY_TASK_PIN, GPIO_PIN_RESET);
        }
    }
}

/**
  * @brief  Task 3: Metrics Collection and Reporting Task
  */
void MetricsTask(void *pvParameters)
{
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = pdMS_TO_TICKS(1000);  // Every 1 second

    xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        /* Collect all metrics */
        CollectCPUMetrics();
        CollectHeapMetrics();
        CollectStackMetrics();

        /* Print comprehensive report */
        PrintMetricsReport();

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* ========== METRICS HELPER FUNCTIONS ========== */

void UpdateLatencyMetrics(TickType_t start_tick, TickType_t end_tick)
{
    float latency_ms = (end_tick - start_tick) * portTICK_PERIOD_MS;

    latency_metrics.latency_sum_ms += latency_ms;
    latency_metrics.sample_count++;

    if (latency_ms > latency_metrics.latency_max_ms)
        latency_metrics.latency_max_ms = latency_ms;

    latency_metrics.latency_avg_ms = latency_metrics.latency_sum_ms / latency_metrics.sample_count;
}

void UpdateJitterMetrics(TickType_t expected_tick, TickType_t actual_tick)
{
    TickType_t jitter;

    if (actual_tick >= expected_tick)
        jitter = actual_tick - expected_tick;
    else
        jitter = expected_tick - actual_tick;

    jitter_metrics.jitter_sum += jitter;
    jitter_metrics.measurement_count++;

    if (jitter > jitter_metrics.jitter_max)
        jitter_metrics.jitter_max = jitter;

    jitter_metrics.jitter_avg_ms = (jitter_metrics.jitter_sum * portTICK_PERIOD_MS) / jitter_metrics.measurement_count;
}

void UpdateDeadlineMetrics(uint8_t deadline_missed)
{
    deadline_metrics.total_samples++;

    if (deadline_missed)
        deadline_metrics.missed_deadlines++;

    deadline_metrics.miss_rate_percent =
        ((float)deadline_metrics.missed_deadlines / deadline_metrics.total_samples) * 100.0f;
}

void CollectCPUMetrics(void)
{
    total_counter++;

    if (total_counter > 0)
    {
        cpu_metrics.idle_time_percent = ((float)idle_counter / total_counter) * 100.0f;
        cpu_metrics.cpu_utilization_percent = 100.0f - cpu_metrics.idle_time_percent;
    }
}

void CollectHeapMetrics(void)
{
    heap_metrics.heap_free_bytes = xPortGetFreeHeapSize();
    heap_metrics.heap_min_free_bytes = xPortGetMinimumEverFreeHeapSize();
    heap_metrics.heap_used_bytes = configTOTAL_HEAP_SIZE - heap_metrics.heap_free_bytes;
}

void CollectStackMetrics(void)
{
    sampling_stack_hwm = uxTaskGetStackHighWaterMark(samplingTaskHandle);
    spirometry_stack_hwm = uxTaskGetStackHighWaterMark(spirometryTaskHandle);
    metrics_stack_hwm = uxTaskGetStackHighWaterMark(metricsTaskHandle);
}

void PrintMetricsReport(void)
{
    printf("\r\n");
    printf("==========================================\r\n");
    printf("  RTOS PERFORMANCE METRICS REPORT (M4)\r\n");
    printf("  Uptime: %lu seconds\r\n", xTaskGetTickCount() / 1000);
    printf("==========================================\r\n");

    printf("\r\n1. REAL-TIME PERFORMANCE\r\n");
    printf("------------------------\r\n");
    printf("End-to-End Latency:\r\n");
    printf("  Average: %.2f ms\r\n", latency_metrics.latency_avg_ms);
    printf("  Maximum: %.2f ms\r\n", latency_metrics.latency_max_ms);
    printf("  Samples: %lu\r\n", latency_metrics.sample_count);

    printf("\r\nDeadline Metrics:\r\n");
    printf("  Total Samples: %lu\r\n", deadline_metrics.total_samples);
    printf("  Missed Deadlines: %lu\r\n", deadline_metrics.missed_deadlines);
    printf("  Miss Rate: %.3f%%\r\n", deadline_metrics.miss_rate_percent);

    printf("\r\n2. SCHEDULING METRICS\r\n");
    printf("---------------------\r\n");
    printf("Jitter:\r\n");
    printf("  Average: %.3f ms\r\n", jitter_metrics.jitter_avg_ms);
    printf("  Maximum: %.3f ms\r\n", jitter_metrics.jitter_max * portTICK_PERIOD_MS);
    printf("  Measurements: %lu\r\n", jitter_metrics.measurement_count);

    printf("\r\nTask Response Times:\r\n");
    printf("  Sampling Task:\r\n");
    printf("    Current: %.2f ms\r\n", sampling_response.response_time_ms);
    printf("    Maximum: %.2f ms\r\n", sampling_response.response_time_max_ms);
    printf("    Executions: %lu\r\n", sampling_response.execution_count);

    printf("  Spirometry Task:\r\n");
    printf("    Current: %.2f ms\r\n", spirometry_response.response_time_ms);
    printf("    Maximum: %.2f ms\r\n", spirometry_response.response_time_max_ms);
    printf("    Executions: %lu\r\n", spirometry_response.execution_count);

    printf("\r\n3. CPU UTILIZATION\r\n");
    printf("------------------\r\n");
    printf("  CPU Usage: %.2f%%\r\n", cpu_metrics.cpu_utilization_percent);
    printf("  Idle Time: %.2f%%\r\n", cpu_metrics.idle_time_percent);
    printf("  Total Ticks: %lu\r\n", total_counter);
    printf("  Idle Ticks: %lu\r\n", idle_counter);

    printf("\r\n4. MEMORY METRICS\r\n");
    printf("-----------------\r\n");
    printf("Heap Usage:\r\n");
    printf("  Used: %lu bytes (%.2f KB)\r\n",
           heap_metrics.heap_used_bytes,
           heap_metrics.heap_used_bytes / 1024.0f);
    printf("  Free: %lu bytes (%.2f KB)\r\n",
           heap_metrics.heap_free_bytes,
           heap_metrics.heap_free_bytes / 1024.0f);
    printf("  Min Free (Safety): %lu bytes\r\n", heap_metrics.heap_min_free_bytes);

    printf("\r\nStack High Water Marks:\r\n");
    printf("  Sampling Task: %lu bytes ", sampling_stack_hwm * 4);
    if (sampling_stack_hwm * 4 < 50)
        printf("*** WARNING: LOW STACK! ***");
    printf("\r\n");

    printf("  Spirometry Task: %lu bytes ", spirometry_stack_hwm * 4);
    if (spirometry_stack_hwm * 4 < 50)
        printf("*** WARNING: LOW STACK! ***");
    printf("\r\n");

    printf("  Metrics Task: %lu bytes ", metrics_stack_hwm * 4);
    if (metrics_stack_hwm * 4 < 50)
        printf("*** WARNING: LOW STACK! ***");
    printf("\r\n");

    printf("\r\n5. SPIROMETRY STATUS\r\n");
    printf("--------------------\r\n");
    printf("  Current Flow: %.2f L/s (%.1f L/min)\r\n", flow_lps, flow_lps * 60.0f);
    printf("  Current Volume: %.3f L\r\n", volume_l);
    printf("  Exhale Active: %s\r\n", exhale_active ? "YES" : "NO");
    if (fvc_l > 0.001f) {
        printf("  FVC: %.3f L\r\n", fvc_l);
        printf("  FEV1: %.3f L\r\n", fev1_l);
        printf("  FEV1/FVC: %.2f%%\r\n", fev1_fvc_ratio * 100.0f);
        printf("  PEF: %.2f L/s\r\n", pef_lps);
    }

    printf("\r\n==========================================\r\n");
    printf("\r\n");
}

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

    printf("Calibrating flow sensor offset...\r\n");

    for(int i = 0; i < 200; i++)
    {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);

        uint32_t raw = HAL_ADC_GetValue(&hadc1);

        HAL_ADC_Stop(&hadc1);

        sum += adc_to_slpm(raw);

        HAL_Delay(10);
    }

    float offset = sum / 200.0f;
    printf("Calibration complete. Offset: %.2f SLPM\r\n", offset);

    return offset;
}

void update_fev_parameters(float flow_lps, float volume_l, uint32_t now_ms)
{
    static uint32_t low_flow_start = 0;

    /* Detect start of exhalation */
    if (!exhale_active && flow_lps > EXHALE_THRESHOLD_LPS)
    {
        fvl_index = 0;
        exhale_active = 1;

        exhale_start_time = now_ms;
        exhale_start_volume = volume_l;

        fev1_captured = 0;
        fev6_captured = 0;

        fvc_l = 0.0f;
        pef_lps = 0.0f;

        printf("\r\n>>> EXHALATION STARTED <<<\r\n");
    }

    /* During exhalation */
    if (exhale_active)
    {
        /* Store Flow-Volume loop */
        if (fvl_index < FVL_MAX_POINTS)
        {
            fvl_buffer[fvl_index].volume = volume_l - exhale_start_volume;
            fvl_buffer[fvl_index].flow   = flow_lps;
            fvl_index++;
        }

        uint32_t elapsed = now_ms - exhale_start_time;

        float exhaled_volume = volume_l - exhale_start_volume;

        /* FVC (max volume) */
        if (exhaled_volume > fvc_l)
            fvc_l = exhaled_volume;

        /* PEF (max flow) */
        if (flow_lps > pef_lps)
            pef_lps = flow_lps;

        /* FEV1 */
        if (!fev1_captured && elapsed >= 1000)
        {
            fev1_l = exhaled_volume;
            fev1_captured = 1;
            printf(">>> FEV1 captured: %.3f L at %lu ms\r\n", fev1_l, elapsed);
        }

        /* FEV6 */
        if (!fev6_captured && elapsed >= 6000)
        {
            fev6_l = exhaled_volume;
            fev6_captured = 1;
            printf(">>> FEV6 captured: %.3f L at %lu ms\r\n", fev6_l, elapsed);
        }

        /* End detection with hysteresis */
        if (flow_lps < EXHALE_THRESHOLD_LPS)
        {
            if (low_flow_start == 0)
                low_flow_start = now_ms;

            if ((now_ms - low_flow_start) > 300)
            {
                exhale_active = 0;
                low_flow_start = 0;

                /* Compute ratio */
                if (fvc_l > 0.001f)
                    fev1_fvc_ratio = fev1_l / fvc_l;

                printf("\r\n>>> EXHALATION COMPLETE <<<\r\n");
                printf(">>> Results: FVC=%.3f L, FEV1=%.3f L, FEV1/FVC=%.2f%%, PEF=%.2f L/s\r\n",
                       fvc_l, fev1_l, fev1_fvc_ratio * 100.0f, pef_lps);
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
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM2)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
