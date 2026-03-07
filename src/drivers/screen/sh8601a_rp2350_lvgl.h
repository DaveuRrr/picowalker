#ifndef PW_DRIVER_SCREEN_RP2XXX_SH8601A_LVGL_H
#define PW_DRIVER_SCREEN_RP2XXX_SH8601A_LVGL_H

// Pico
#include "stdio.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"

// Waveshare AMOLED 1.8" Library
#include "QSPI_PIO.h"
#include "SH8601A.h"
#include "FT3168.h"

// LVGL
#include "lvgl.h"

// Background Image
#include "picowalker_background_240x240_lvgl.h"
#include "picowalker_background_ultra_240x240_lvgl.h"

// Picowalker
#include "picowalker_structures.h"
#include "picowalker_core.h"

#define SH8601A_MIN_BRIGHTNESS 2
#define SH8601A_MAX_BRIGHTNESS 90

// DMA Configuration for QSPI
extern int SH8601A_DMA_TX;
extern dma_channel_config SH8601A_DMA_CONFIG;

#ifndef TOUCH
#define TOUCH true
#endif

// Use dynamic resolution from SH8601A attributes (set by orientation)
extern SH8601A_ATTRIBUTES SH8601A;
#define DISP_HOR_RES SH8601A.WIDTH   // Dynamic width based on orientation
#define DISP_VER_RES SH8601A.HEIGHT  // Dynamic height based on orientation

// 1x = 96x64, 1.5x = 144x96, 2x = 192x128, 3x = 288x192
#ifndef CANVAS_SCALE
#define CANVAS_SCALE 2
#endif

#if CANVAS_SCALE < 2
#define LR_BUTTON_Y_OFFSET  70
#define MD_BUTTON_Y_OFFSET  80
#define CANVAS_Y_OFFSET    -10
#elif CANVAS_SCALE == 2
#define LR_BUTTON_Y_OFFSET  85
#define MD_BUTTON_Y_OFFSET 100
#define CANVAS_Y_OFFSET      0
#else
#define LR_BUTTON_Y_OFFSET  85
#define MD_BUTTON_Y_OFFSET 100
#define CANVAS_Y_OFFSET      0
#endif

#define CANVAS_WIDTH  (int)(PW_SCREEN_WIDTH * CANVAS_SCALE)
#define CANVAS_HEIGHT (int)(PW_SCREEN_HEIGHT * CANVAS_SCALE)

// Global variables
extern bool is_sleeping;

// Button callback is implemented in main_ws.c
//extern void pw_button_callback(uint8_t button);

static void display_flush_callback(lv_disp_drv_t *display, const lv_area_t *area, lv_color_t *color);
static void direct_memory_access_handler(void);
static bool repeating_lvgl_timer_callback(struct repeating_timer *timer);

// Battery functions
void pw_screen_update_battery();

// Image color functions
lv_color_t get_color(uint16_t color, bool is_color);

#endif /* PW_DRIVER_SCREEN_RP2XXX_SH8601A_LVGL_H */