/**
 * @file prog01/app/src/lcddrv.c
 * Raspberry PI PICO C-SDK 向け
 * WAVESHARE-27579 2.8インチ静電容量タッチLCD(240x320) 向け SPI 接続LCDドライバ
 * 標準のSPI 25MHz 駆動で 全画面更新 16fps 程度
 */

#if 0
// リセット = リセットピン操作 1 -> 100ms -> 0 -> 100ms -> 1 -> 100ms

// コマンド送信
// DCピン 0
// SPI Write

// データ送信
// DCピン 1
// SPI Write

// 初期化コマンド
// リセット
// 0x36 - 0x00 : Memory Data Access Control
// 0x3A - 0x05 : Interface Pixel Format
// 0xB2 - 0x0B 0x0B 0x00 0x33 0x35 : Porch Setting
// 0xB7 - 0x11 : Gate Control
// 0xBB - 0x35 : VCOMS Setting
// 0xC0 - 0x2C : LCM Control
// 0xC2 - 0x01 : VDV and VRH Command Enable
// 0xC3 - 0x0D : VRH Set
// 0xC4 - 0x20 : VDV Set
// 0xC6 - 0x13 : Frame Rate Control in Normal Mode
// 0xD0 - 0xA4 0xA1 : Power Control 1
// 0xD6 - 0xA1 : // unknown...
// 0xE0 - 0xF0 0x06 0x0B 0x0A 0x09 0x26 0x29 0x33 0x41 0x18 0x16 0x15 0x29 0x2D
// : Positive Voltage Gammma Control 0xE1 - 0xF0 0x04 0x08 0x08 0x07 0x03 0x28
// 0x32 0x40 0x3B 0x19 0x18 0x2A 0x2E : Neg 0x21 : Display Inversion On 0x11 :
// Sleep Out delay 120ms 0x29 : Display On

// 320 * 240 * 2 = 153600 byte. = 150KB フレームバッファ

// 10,000, 000

#endif

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>

#include <pico/stdlib.h>

#include <hardware/gpio.h>
#include <hardware/pwm.h>

#include <user/lcddrv.h>
#include <user/macros.h>
#include <user/spidrv.h>
#include <user/types.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

#define LCD0_DC_PIN (6)
#define LCD0_RST_PIN (7)
#define LCD0_BL_PIN (8)

#define HANDLE_TO_CONTEXTP(p) (LCDDrvContext_t*)(p)

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

typedef struct tagLCDDrvContext_t {
  SPIDrvHandle_t spi;
  uint32_t dc;
  uint32_t rst;
  uint32_t bl;
  uint32_t pwm;

  bool bBusy;
} LCDDrvContext_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////
/**
 * @brief TFT の D/C信号を0にする
 * @param
 */
static inline void LCDDrv_DC0(LCDDrvContext_t* lcd);
/**
 * @brief TFT の D/C信号を1にする
 * @param
 */
static inline void LCDDrv_DC1(LCDDrvContext_t* lcd);

static UError_t LCDDrv_SendCommand(LCDDrvContext_t* lcd, const uint8_t cmd);
static UError_t LCDDrv_SendDataByte(LCDDrvContext_t* lcd, const uint8_t data);

static UError_t LCDDrv_HWReset(LCDDrvContext_t* lcd);
static UError_t LCDDrv_InitRegister(LCDDrvContext_t* lcd);
static UError_t LCDDrv_SetAttributes(LCDDrvContext_t* lcd);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

static LCDDrvContext_t builtinContext[] = {
    {
        .spi = NULL,
        .dc = LCD0_DC_PIN,
        .rst = LCD0_RST_PIN,
        .bl = LCD0_BL_PIN,
        .pwm = 0u,
        .bBusy = false,
    },
};

static LCDDrvContext_t* builtinContextInUse[] = {
    NULL,
};

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

static inline void LCDDrv_DC0(LCDDrvContext_t* lcd) {
  NOP3();
  gpio_put(lcd->dc, 0);
  NOP3();
}

static inline void LCDDrv_DC1(LCDDrvContext_t* lcd) {
  NOP3();
  gpio_put(lcd->dc, 1);
  NOP3();
}

static UError_t LCDDrv_SendCommand(LCDDrvContext_t* lcd, const uint8_t cmd) {
  UError_t err = uSuccess;
  if (uSuccess == err) {
    if (NULL == lcd) {
      err = uFailure;
    }
  }
  if (uSuccess == err) {
    LCDDrv_DC0(lcd);
    err = SPIDrv_SendByte(lcd->spi, cmd);
  }
  return err;
}

static UError_t LCDDrv_SendDataByte(LCDDrvContext_t* lcd, const uint8_t data) {
  UError_t err = uSuccess;
  if (uSuccess == err) {
    if (NULL == lcd) {
      err = uFailure;
    }
  }
  if (uSuccess == err) {
    LCDDrv_DC1(lcd);
    err = SPIDrv_SendByte(lcd->spi, data);
  }
  return err;
}

static UError_t LCDDrv_HWReset(LCDDrvContext_t* lcd) {
  UError_t err = uSuccess;
  if (uSuccess == err) {
    if (NULL == lcd) {
      err = uFailure;
    }
  }
  if (uSuccess == err) {
    gpio_put(lcd->rst, 1);
    sleep_ms(100);
    gpio_put(lcd->rst, 0);
    sleep_ms(100);
    gpio_put(lcd->rst, 1);
    sleep_ms(100);
  }
  return err;
}

static UError_t LCDDrv_InitRegister(LCDDrvContext_t* lcd) {
  int32_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == lcd) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    LCDDrv_SendCommand(lcd, 0x36);  // Memory data Access Control
    LCDDrv_SendDataByte(lcd, 0x00);

    LCDDrv_SendCommand(lcd, 0x3A);   // Interface Pixel format
    LCDDrv_SendDataByte(lcd, 0x05);  // 16bit/pixel
    // LCDDrv_SendDataByte(0x06);  // 18bit/pixel

    // ディスプレイタイミングパラメータ
    LCDDrv_SendCommand(lcd, 0xB2);   // Porch Setting require params = 5
    LCDDrv_SendDataByte(lcd, 0x0B);  // back porch setting b1011 = 11
    LCDDrv_SendDataByte(lcd, 0x0B);  // front porch setting b1011 = 11
    LCDDrv_SendDataByte(lcd,
                        0x00);       // Enable separate porch control = 0 = disable
    LCDDrv_SendDataByte(lcd, 0x33);  // Back porch setting in idle time = 3,
                                     // Front portch setting in idle time = 3
    LCDDrv_SendDataByte(lcd, 0x35);  // Back porch setting in idle time = 3,
                                     // Front portch setting in idle time = 5

    LCDDrv_SendCommand(lcd, 0xB7);   // Gate Control
    LCDDrv_SendDataByte(lcd, 0x11);  // VGL = -7.67, VGH = 12.54

    LCDDrv_SendCommand(lcd, 0xBB);   // VCOMS Setting
    LCDDrv_SendDataByte(lcd, 0x35);  // 1.425

    LCDDrv_SendCommand(lcd, 0xC0);  // LCM Control
    LCDDrv_SendDataByte(lcd,
                        0x2C);  // b0010_1100 XMY:0 XBGR:1 XINV:0 XMX:1 XMH:1 XMV:0 XGS:0

    LCDDrv_SendCommand(lcd, 0xC2);   // VDV and VRH Command Enable
    LCDDrv_SendDataByte(lcd, 0x01);  // Enable

    // note. Cx は謎
    LCDDrv_SendCommand(lcd, 0xC3);
    LCDDrv_SendDataByte(lcd, 0x0D);

    LCDDrv_SendCommand(lcd, 0xC4);
    LCDDrv_SendDataByte(lcd, 0x20);

    LCDDrv_SendCommand(lcd, 0xC6);
    LCDDrv_SendDataByte(lcd, 0x13);

    LCDDrv_SendCommand(lcd, 0xD0);
    LCDDrv_SendDataByte(lcd, 0xA4);
    LCDDrv_SendDataByte(lcd, 0xA1);

    LCDDrv_SendCommand(lcd, 0xD6);
    LCDDrv_SendDataByte(lcd, 0xA1);

    LCDDrv_SendCommand(lcd, 0xE0);
    LCDDrv_SendDataByte(lcd, 0xF0);
    LCDDrv_SendDataByte(lcd, 0x06);
    LCDDrv_SendDataByte(lcd, 0x0B);
    LCDDrv_SendDataByte(lcd, 0x0A);
    LCDDrv_SendDataByte(lcd, 0x09);
    LCDDrv_SendDataByte(lcd, 0x26);
    LCDDrv_SendDataByte(lcd, 0x29);
    LCDDrv_SendDataByte(lcd, 0x33);
    LCDDrv_SendDataByte(lcd, 0x41);
    LCDDrv_SendDataByte(lcd, 0x18);
    LCDDrv_SendDataByte(lcd, 0x16);
    LCDDrv_SendDataByte(lcd, 0x15);
    LCDDrv_SendDataByte(lcd, 0x29);
    LCDDrv_SendDataByte(lcd, 0x2D);

    LCDDrv_SendCommand(lcd, 0xE1);
    LCDDrv_SendDataByte(lcd, 0xF0);
    LCDDrv_SendDataByte(lcd, 0x04);
    LCDDrv_SendDataByte(lcd, 0x08);
    LCDDrv_SendDataByte(lcd, 0x08);
    LCDDrv_SendDataByte(lcd, 0x07);
    LCDDrv_SendDataByte(lcd, 0x03);
    LCDDrv_SendDataByte(lcd, 0x28);
    LCDDrv_SendDataByte(lcd, 0x32);
    LCDDrv_SendDataByte(lcd, 0x40);
    LCDDrv_SendDataByte(lcd, 0x3B);
    LCDDrv_SendDataByte(lcd, 0x19);
    LCDDrv_SendDataByte(lcd, 0x18);
    LCDDrv_SendDataByte(lcd, 0x2A);
    LCDDrv_SendDataByte(lcd, 0x2E);

    LCDDrv_SendCommand(lcd, 0x21);  // Display inversion on

    LCDDrv_SendCommand(lcd, 0x11);  // sleep out
    sleep_ms(120);
    LCDDrv_SendCommand(lcd, 0x29);  // Display on
  }

  return err;
}

static UError_t LCDDrv_SetAttributes(LCDDrvContext_t* lcd) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == lcd) {
      err = uFailure;
    }
  }

  // 一旦 320 x 240 だけ考える
  if (uSuccess == err) {
    err = LCDDrv_SendCommand(lcd, 0x36);
  }

  if (uSuccess == err) {
    err = LCDDrv_SendDataByte(lcd, 0x78);
  }

  return err;
}

UError_t LCDDrv_Open(LCDDrvHandle_t* handle, SPIDrvHandle_t spi) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle || NULL == spi) {
      err = uFailure;
    }
  }

  LCDDrvContext_t* ret = NULL;
  if (uSuccess == err) {
    for (size_t i = 0; i < (sizeof(builtinContextInUse) / sizeof(builtinContextInUse[0])); ++i) {
      if (NULL == builtinContextInUse[i]) {
        ret = &builtinContext[i];
        builtinContextInUse[i] = ret;
        break;
      }
    }
    if (NULL == ret) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    ret->spi = spi;
    *handle = (LCDDrvHandle_t)ret;
  }

  return err;
}

void LCDDrv_Close(LCDDrvHandle_t handle) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    for (size_t i = 0; i < (sizeof(builtinContextInUse) / sizeof(builtinContextInUse[0])); ++i) {
      if (builtinContextInUse[i] == handle) {
        builtinContextInUse[i] = NULL;
        builtinContext[i].bBusy = false;
        builtinContext[i].spi = NULL;
        // TODO: pwmの開放
        break;
      }
    }
  }
}

UError_t LCDDrv_Init(LCDDrvHandle_t handle) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle) {
      err = uFailure;
    }
  }

  LCDDrvContext_t* const lcd = HANDLE_TO_CONTEXTP(handle);

  // TFTコントローラ制御ピン

  // D/C 制御
  if (uSuccess == err) {
    gpio_init(lcd->dc);
    gpio_put(lcd->dc, 1);
    gpio_set_dir(lcd->dc, GPIO_OUT);
  }

  // リセット
  if (uSuccess == err) {
    gpio_init(lcd->rst);
    gpio_put(lcd->rst, 0);
    gpio_set_dir(lcd->rst, GPIO_OUT);
  }

  if (uSuccess == err) {
    // バックライト(PWM制御)
    gpio_set_function(lcd->bl, GPIO_FUNC_PWM);
    lcd->pwm = pwm_gpio_to_slice_num(lcd->bl);
    pwm_config pwm_config = pwm_get_default_config();
    pwm_init(lcd->pwm, &pwm_config, false);
    pwm_set_gpio_level(lcd->bl, 0u);  // 0xffff で初期化
    pwm_set_enabled(lcd->pwm, true);
  }

  if (uSuccess == err) {
    err = LCDDrv_InitalizeHW(lcd);
  }

  return err;
}

UError_t LCDDrv_InitalizeHW(LCDDrvHandle_t handle) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle) {
      err = uFailure;
    }
  }

  LCDDrvContext_t* const lcd = HANDLE_TO_CONTEXTP(handle);

  if (uSuccess == err) {
    LCDDrv_HWReset(lcd);
  }

  if (uSuccess == err) {
    LCDDrv_InitRegister(lcd);
  }

  return err;
}

UError_t LCDDrv_SetWindow(LCDDrvHandle_t handle, const uint16_t x, const uint16_t y, const uint16_t width, const uint16_t height) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle) {
      err = uFailure;
    }
  }

  LCDDrvContext_t* const lcd = HANDLE_TO_CONTEXTP(handle);

  if (uSuccess == err) {
    if ((320 <= y) || (240 <= x) || (320 < (y + height)) || (240 < (x + width))) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    const uint16_t xe = x + width - 1;
    const uint16_t ye = y + height - 1;

    LCDDrv_SendCommand(lcd, 0x2A);  // Column address set
    LCDDrv_SendDataByte(lcd, x >> 8);
    LCDDrv_SendDataByte(lcd, x);
    LCDDrv_SendDataByte(lcd, xe >> 8);
    LCDDrv_SendDataByte(lcd, xe);

    LCDDrv_SendCommand(lcd, 0x2B);  // Raw address set
    LCDDrv_SendDataByte(lcd, y >> 8);
    LCDDrv_SendDataByte(lcd, y);
    LCDDrv_SendDataByte(lcd, ye >> 8);
    LCDDrv_SendDataByte(lcd, ye);
  }

  if (uSuccess == err) {
    LCDDrv_SendCommand(lcd, 0x2C);  // Memory write. prepare send framedata
  }

  return err;
}

UError_t LCDDrv_Clear(LCDDrvHandle_t handle, const uint8_t r, const uint8_t g, const uint8_t b) {
  UError_t err = uSuccess;
  uint16_t buf[240];
  uint16_t color = RGB888toRGB565(r, g, b);

  if (uSuccess == err) {
    if (NULL == handle) {
      err = uFailure;
    }
  }

  LCDDrvContext_t* const lcd = HANDLE_TO_CONTEXTP(handle);

  if (uSuccess == err) {
    for (size_t j = 0; j < 240; ++j) {
      buf[j] = color;
    }
  }

  if (uSuccess == err) {
    err = LCDDrv_SetWindow(lcd, 0, 0, 240, 320);
  }

  if (uSuccess == err) {
    // SPIDrv を直接使用するため D/C を データ送信モードに設定
    LCDDrv_DC1(lcd);
  }

  if (uSuccess == err) {
    for (size_t j = 0; j < 320; ++j) {
      err = SPIDrv_SendNBytes(lcd->spi, buf, 240 * 2);
      if (uSuccess != err) {
        break;
      }
    }
  }

  return err;
}

UError_t LCDDrv_SetBrightness(LCDDrvHandle_t handle, uint16_t b) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle) {
      err = uFailure;
    }
  }

  LCDDrvContext_t* const lcd = HANDLE_TO_CONTEXTP(handle);

  if (uSuccess == err) {
    pwm_set_gpio_level(lcd->bl, b);
  }

  return err;
}

UError_t LCDDrv_SwapBuff(LCDDrvHandle_t handle, const void* frame, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle || NULL == frame) {
      err = uFailure;
    }
  }

  LCDDrvContext_t* const lcd = HANDLE_TO_CONTEXTP(handle);

  if (uSuccess == err) {
    if (lcd->bBusy) {
      SPIDrv_WaitForAsync(lcd->spi);
      // printf("[DEBUG] %s()\n", "SPIDrv_WaitForAsync");
    }
    err = LCDDrv_SetWindow(lcd, x, y, w, h);
    // printf("[DEBUG] %s(%d, %d, %d, %d)\n", "LCDDrv_SetWindow", x, y, w, h);
  }

  if (uSuccess == err) {
    LCDDrv_DC1(lcd);
    err = SPIDrv_AsyncSend(lcd->spi, frame, w * h * 2);
    // printf("[DEBUG] %s(0x%08lx, %d)\n", "SPIDrv_AsyncSend",frame, w * h * 2);
  }
  if (uSuccess == err) {
    lcd->bBusy = true;
  }

  return err;
}
