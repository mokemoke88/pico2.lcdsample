/**
 * @file app/src/touchdrv.c
 *
 * @date 2024.10.13 k.shibata newly created
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/touchdrv.h>

#include <user/types.h>

#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <pico/binary_info.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////
#define TOUCH_RST (10)
#define TOUCH_INT (11)
#define TOUCH_SDA (12)
#define TOUCH_SCL (13)
#define TOUCH_I2C (i2c0)
#define TOUCH_I2C_ADDR (0x1a)

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

typedef struct tagTouchDrvContext_t {
  i2c_inst_t* i2c;
  uint8_t addr;
  uint32_t sda;
  uint32_t scl;
  uint32_t rst;
  uint32_t intr;

  bool active;
  uint32_t xmax;
  uint32_t ymax;
  volatile uint32_t event;

  CST328Data_t data;
} TouchDrvContext_t;

static TouchDrvContext_t builtinContext[] = {
    {
        .i2c = TOUCH_I2C,
        .addr = TOUCH_I2C_ADDR,
        .sda = TOUCH_SDA,
        .scl = TOUCH_SCL,
        .rst = TOUCH_RST,
        .intr = TOUCH_INT,
        .active = false,
        .xmax = 0,
        .ymax = 0,
        .event = 0,
    },
};

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////
static const uint8_t TOUCHDRV_DEBUG_INFO_MODE[] = {0xd1, 0x01};
static const uint8_t TOUCHDRV_NORMAL_MODE[] = {0xd1, 0x09};
static const uint8_t TOUCHDRV_DEBUG_INFO_BOOT_TIME[] = {0xd1, 0xfc};
static const uint8_t TOUCHDRV_DEBUG_INFO_RES_X[] = {0xd1, 0xf8};
static const uint8_t TOUCHDRV_DEBUG_INFO_TP_NTX[] = {0xd1, 0xf4};

static const uint8_t TOUCHDRV_READ_Number_REG[] = {0xd0, 05};
static const uint8_t TOUCHDRV_READ_Number_REG_CLEAR[] = {0xd0, 0x05, 0x00};
static const uint8_t TOUCHDRV_READ_XY_REG[] = {0xd0, 0x00};

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

static void TouchDrv_gpio_callback(uint gpio, uint32_t events) {
  // printf("[DEBUG] intr entry\n");
  for (size_t i = 0; i < sizeof(builtinContext) / sizeof(builtinContext[0]); ++i) {
    if (builtinContext[i].intr == gpio && builtinContext[i].active) {
      // printf("[DEBUG] instance found\n");
      builtinContext[i].event = 1u;
      break;
    }
  }
}

static UError_t TouchDrv_platformInit(TouchDrvContext_t* ctx) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (ctx->active) {
      // already init
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    // I2C 設定
    // タッチ側でpullupされているため, nopull.
    uint hz = i2c_init(ctx->i2c, 400 * 1000);  // 400kHz
    // printf("[DEBUG] i2c hz [%d]\n", hz);
    gpio_set_pulls(ctx->scl, false, false);
    gpio_set_pulls(ctx->sda, false, false);
    gpio_set_function_masked((1 << ctx->sda) | (1 << ctx->scl), GPIO_FUNC_I2C);

    // RST/INTピン
    gpio_set_pulls(ctx->intr, true, false);
    gpio_init_mask((1 << ctx->rst) | (1 << ctx->intr));
    gpio_set_function_masked((1 << ctx->rst) | (1 << ctx->intr), GPIO_FUNC_SIO);

    // RSTピン
    gpio_put(ctx->rst, false);
    gpio_set_dir(ctx->rst, GPIO_OUT);

    // INTピン
    gpio_set_dir(ctx->intr, GPIO_IN);

    // 割り込みハンドラ登録
    // gpio_set_irq_callback(&TouchDrv_gpio_callback);
  }

  return err;
}

static UError_t TouchDrv_hwInit(TouchDrvContext_t* ctx) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (ctx->active) {
      // already init
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    gpio_put(ctx->rst, false);
    sleep_ms(1);
    gpio_put(ctx->rst, true);
    sleep_ms(150);
    // gpio_set_irq_enabled(, GPIO_IRQ_EDGE_FALL, true);
    // printf("[DEBUG] INTR register\n");
    gpio_set_irq_enabled_with_callback(ctx->intr, GPIO_IRQ_EDGE_FALL, true, &TouchDrv_gpio_callback);
  }

  return err;
}

static inline size_t TouchDrv_i2cWrite(TouchDrvContext_t* ctx, const void* data, size_t size) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == data || 0 == size) {
      err = uFailure;
    }
  }

  size_t ret = 0u;
  if (uSuccess == err) {
    int result = i2c_write_blocking(ctx->i2c, ctx->addr, data, size, false);
    if (PICO_ERROR_GENERIC == result) {
      err = uFailure;
    } else {
      ret = result;
    }
  }

  if (uFailure == err) {
    ret = 0u;
  }

  return ret;
}

static inline size_t TouchDrv_i2cRead(TouchDrvContext_t* ctx, void* data, size_t size) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == data || 0 == size) {
      err = uFailure;
    }
  }

  size_t ret = 0u;
  if (uSuccess == err) {
    int result = i2c_read_blocking(ctx->i2c, ctx->addr, data, size, false);
    if (PICO_ERROR_GENERIC == result) {
      err = uFailure;
    } else {
      ret = result;
    }
  }

  if (uFailure == err) {
    ret = 0u;
  }

  return ret;
}

static UError_t TouchDrv_deviceInit(TouchDrvContext_t* ctx) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (ctx->active) {
      // already init
      err = uFailure;
    }
  }

  {
    // I2C による HW初期化
    uint8_t data[64] = {0};

    // ハードウェア諸元の取得

    // DEBUG INFO MODE へ遷移
    if (uSuccess == err) {
      if (2u != TouchDrv_i2cWrite(ctx, TOUCHDRV_DEBUG_INFO_MODE, 2)) {
        printf("[ERROR] TOUCHDRV_DEBUG_INFO_MODE failure\n");
        err = uFailure;
      }
    }

    if (uSuccess == err) {
      // BOOT TIME 取得
      if (2u != TouchDrv_i2cWrite(ctx, TOUCHDRV_DEBUG_INFO_BOOT_TIME, 2)) {
        printf("[ERROR] TOUCHDRV_DEBUG_INFO_BOOT_TIME failure\n");
        err = uFailure;
      } else {
        if (4u == TouchDrv_i2cRead(ctx, data, 4)) {
          printf("[DEBUG] read [%02x %02x %02x %02x]\n", data[0], data[1], data[2], data[3]);
        } else {
          printf("[ERROR] TOUCHDRV_DEBUG_INFO_BOOT_TIME read failure\n");
          err = uFailure;
        }
      }
    }

    if (uSuccess == err) {
      // X/Y 解像度取得
      if (2u != TouchDrv_i2cWrite(ctx, TOUCHDRV_DEBUG_INFO_RES_X, 2)) {
        printf("[ERROR] TOUCHDRV_DEBUG_INFO_RES_X failure\n");
        err = uFailure;
      } else {
        if (4u == TouchDrv_i2cRead(ctx, data, 4)) {
          ctx->xmax = data[1] * 256 + data[0];
          ctx->ymax = data[3] * 256 + data[2];
          printf("[DEBUG] X_MAX: [%d] Y_MAX: [%d]\n", ctx->xmax, ctx->ymax);
        } else {
          printf("[ERROR] TOUCHDRV_DEBUG_INFO_RES_X read failure\n");
          err = uFailure;
        }
      }
    }

    if (uSuccess == err) {
      // 諸元取得
      if (2u != TouchDrv_i2cWrite(ctx, TOUCHDRV_DEBUG_INFO_TP_NTX, 2)) {
        printf("[ERROR] TOUCHDRV_DEBUG_INFO_TP_NTX failure\n");
        err = uFailure;
      } else {
        if (24u == TouchDrv_i2cRead(ctx, data, 24)) {
          if (0xca != data[10] || 0xca != data[11]) {
            printf("[ERROR] TOUCHDRV_DEBUG_INFO_TP_NTX read CACA failure\n");
            err = uFailure;
          }
        } else {
          printf("[ERROR] TOUCHDRV_DEBUG_INFO_TP_NTX read failure\n");
          err = uFailure;
        }
      }
    }

    if (uSuccess == err) {
      // 諸元取得
      if (2u != TouchDrv_i2cWrite(ctx, TOUCHDRV_NORMAL_MODE, 2)) {
        printf("[ERROR] TOUCHDRV_NORMAL_MODE failure\n");
        err = uFailure;
      }
    }
  }

  if (uSuccess == err) {
    ctx->active = true;
  }
  return err;
}

static UError_t TouchDrv_getPosition(TouchDrvContext_t* ctx) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (!ctx->active) {
      err = uFailure;
    }
  }

  // 有効データ数取得コマンド発行
  if (uSuccess == err) {
    if (2u != TouchDrv_i2cWrite(ctx, TOUCHDRV_READ_Number_REG, 2)) {
      printf("[ERROR] TOUCHDRV_READ_Number_REG fail\n");
      err = uFailure;
    }
  }

  // 有効データ数取得
  size_t num = 0u;
  if (uSuccess == err) {
    uint8_t data[1] = {0};
    if (1u != TouchDrv_i2cRead(ctx, data, 1)) {
      printf("[ERROR] read fail\n");
      err = uFailure;
    } else {
      num = 0x0f & data[0];
    }
  }

  // ポジション取得
  uint8_t posData[27] = {0};
  if (uSuccess == err && (0 != num && 5 >= num)) {
    if (2u != TouchDrv_i2cWrite(ctx, TOUCHDRV_READ_XY_REG, 2)) {
      printf("[ERROR] TOUCHDRV_READ_XY_REG fail\n");
      err = uFailure;
    } else {
      if (27u != TouchDrv_i2cRead(ctx, posData, 27)) {
        printf("[ERROR] read 27 fail\n");
        err = uFailure;
      }
    }
  }

  // ポジション情報クリア
  if (uSuccess == err) {
    if (3u != TouchDrv_i2cWrite(ctx, TOUCHDRV_READ_Number_REG_CLEAR, 3)) {
      printf("[ERROR] TOUCHDRV_READ_Number_REG_CLEAR fail\n");
      err = uFailure;
    }
  }

  // 内部の位置情報を更新
  if (uSuccess == err) {
    ctx->data.points = num;
    for (size_t i = 0; i < num; ++i) {
      size_t n = (i > 0) ? 2 : 0;
      ctx->data.coords[i].x = (posData[(i * 5) + 1 + n] << 4) + ((posData[(i * 5) + 3 + n] & 0xf0) >> 4);
      ctx->data.coords[i].y = (posData[(i * 5) + 2 + n] << 4) + (posData[(i * 5) + 3 + n] & 0x0f);
      ctx->data.coords[i].strength = (posData[(i * 5) + 4 + n]);
    }
  } else {
    ctx->data.points = 0;
  }

  return err;
}

/**
 * @brief
 * @param
 * @return
 * @note SDA(灰色) SCL(青) RST(紫:out) INT(黄:in)
 */
static UError_t TouchDrv_init(TouchDrvContext_t* ctx) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    err = TouchDrv_platformInit(ctx);
  }

  if (uSuccess == err) {
    err = TouchDrv_hwInit(ctx);
  }

  if (uSuccess == err) {
    err = TouchDrv_deviceInit(ctx);
  }
  return err;
}

UError_t TouchDrv_Open(TouchDrvHandle_t* pHandle) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == pHandle) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (builtinContext[0].active) {
      err = uFailure;
    } else {
      *pHandle = &builtinContext[0];
    }
  }
  return err;
}

UError_t TouchDrv_Init(TouchDrvHandle_t handle) {
  UError_t err = uSuccess;
  TouchDrvContext_t* ctx = (TouchDrvContext_t*)handle;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    err = TouchDrv_init(ctx);
  }

  return err;
}

UError_t TouchDrv_UpdateCoord(TouchDrvHandle_t handle) {
  UError_t err = uSuccess;
  TouchDrvContext_t* ctx = (TouchDrvContext_t*)handle;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (ctx->event) {
      ctx->event = 0;
      err = TouchDrv_getPosition(ctx);
    } else {
      ctx->data.points = 0;
    }
  }
  return err;
}

UError_t TouchDrv_GetCoord(TouchDrvHandle_t handle, CST328Data_t* pCoord) {
  UError_t err = uSuccess;
  TouchDrvContext_t* ctx = (TouchDrvContext_t*)handle;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == pCoord) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    memcpy(pCoord, &ctx->data, sizeof(CST328Data_t));
  }

  return err;
}
