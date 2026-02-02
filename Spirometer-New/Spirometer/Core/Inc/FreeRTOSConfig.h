/**
  ******************************************************************************
  * @file    FreeRTOSConfig.h (Required Settings for Metrics Collection)
  * @brief   FreeRTOS Configuration for STM32F4 with Performance Metrics
  ******************************************************************************
  */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ========== CRITICAL SETTINGS FOR METRICS COLLECTION ========== */

/* Enable these for comprehensive metrics */
#define configUSE_PREEMPTION                     1
#define configUSE_IDLE_HOOK                      1      /* REQUIRED for CPU utilization */
#define configUSE_TICK_HOOK                      0
#define configUSE_TRACE_FACILITY                 1      /* REQUIRED for runtime stats */
#define configGENERATE_RUN_TIME_STATS            0      /* Optional - advanced profiling */

/* Clock Configuration */
#define configCPU_CLOCK_HZ                       84000000   /* STM32F4 @ 84 MHz */
#define configTICK_RATE_HZ                       1000       /* 1ms tick for precise timing */
#define configSYSTICK_CLOCK_HZ                   configCPU_CLOCK_HZ
#define configUSE_16_BIT_TICKS                   0          /* Use 32-bit tick counter */

/* Task Configuration */
#define configMAX_PRIORITIES                     5
#define configMINIMAL_STACK_SIZE                 128        /* Words, not bytes */
#define configMAX_TASK_NAME_LEN                  16

/* Memory Configuration */
#define configTOTAL_HEAP_SIZE                    10240      /* 10KB - adjust based on needs */
#define configSTACK_DEPTH_TYPE                   uint16_t

/* Synchronization */
#define configUSE_MUTEXES                        1          /* REQUIRED for mutex */
#define configUSE_COUNTING_SEMAPHORES            1
#define configUSE_RECURSIVE_MUTEXES              1

/* Queue Configuration */
#define configQUEUE_REGISTRY_SIZE                8

/* Memory allocation */
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configSUPPORT_STATIC_ALLOCATION          0

/* Hook function related definitions */
#define configCHECK_FOR_STACK_OVERFLOW           2          /* Enable stack overflow detection */
#define configUSE_MALLOC_FAILED_HOOK             1

/* Co-routine definitions */
#define configUSE_CO_ROUTINES                    0

/* Software timer definitions */
#define configUSE_TIMERS                         0          /* Not needed for this app */
#define configTIMER_TASK_PRIORITY                2
#define configTIMER_QUEUE_LENGTH                 10
#define configTIMER_TASK_STACK_DEPTH             configMINIMAL_STACK_SIZE

/* Optional functions */
#define INCLUDE_vTaskPrioritySet                 1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                      1
#define INCLUDE_vTaskCleanUpResources            0
#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_vTaskDelayUntil                  1          /* REQUIRED for periodic tasks */
#define INCLUDE_vTaskDelay                       1
#define INCLUDE_uxTaskGetStackHighWaterMark      1          /* REQUIRED for stack metrics */
#define INCLUDE_xTaskGetCurrentTaskHandle        1
#define INCLUDE_xTaskGetSchedulerState           1

/* Cortex-M specific definitions */
#ifdef __NVIC_PRIO_BITS
 #define configPRIO_BITS                         __NVIC_PRIO_BITS
#else
 #define configPRIO_BITS                         4
#endif

/* The lowest interrupt priority that can be used in a call to a "set priority" function */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   15

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

/* Priority 5, or 80 as only the top 4 bits are implemented */
#define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS names */
#define vPortSVCHandler                          SVC_Handler
#define xPortPendSVHandler                       PendSV_Handler
#define xPortSysTickHandler                      SysTick_Handler

/* Normal assert() semantics without relying on the provision of an assert.h header file */
#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

#endif /* FREERTOS_CONFIG_H */
