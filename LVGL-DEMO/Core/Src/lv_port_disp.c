/*
 * lv_port_disp.c
 *
 *  Created on: Jan 29, 2026
 *      Author: lenin
 */

#include "lv_port_disp.h"
#include "ILI9341.h"

// Display resolution
#define MY_DISP_HOR_RES 240
#define MY_DISP_VER_RES 320

static lv_display_t *disp;

static void disp_init(void);
static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

void lv_port_disp_init(void)
{
    // Initialize your display hardware
    disp_init();

    // Create display object
    disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);

    // Set up draw buffers (using two buffers for better performance)
    // Buffer size: 10 rows
    static uint8_t buf_1[MY_DISP_HOR_RES * 10 * sizeof(lv_color_t)];
    static uint8_t buf_2[MY_DISP_HOR_RES * 10 * sizeof(lv_color_t)];
    lv_display_set_buffers(disp, buf_1, buf_2, sizeof(buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Set flush callback
    lv_display_set_flush_cb(disp, disp_flush);
}

static void disp_init(void)
{
    // Initialize  ILI9341 display
    ILI9341_Init();
}

// Without DMA version
static void disp_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *px_map)
{
    ILI9341_SetWindow(area->x1, area->y1, area->x2, area->y2);

    int32_t height = area->y2 - area->y1 + 1;
    int32_t width = area->x2 - area->x1 + 1;

    ILI9341_DrawBitmap(width, height, px_map);

    lv_display_flush_ready(disp_drv);
}

/* DMA version (if using DMA):
static void disp_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *px_map)
{
    ILI9341_SetWindow(area->x1, area->y1, area->x2, area->y2);

    int32_t height = area->y2 - area->y1 + 1;
    int32_t width = area->x2 - area->x1 + 1;

    ILI9341_DrawBitmapDMA(width, height, px_map);
    // Note: lv_display_flush_ready() should be called in the DMA complete callback
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    lv_display_flush_ready(disp);
}
*/
