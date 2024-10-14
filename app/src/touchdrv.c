/**
 * @file app/src/touchdrv.c
 * Raspberry PI PICO C-SDK 向け
 * WAVEWARE-27579 2.0インチ静電容量タッチLCD(240x320) 向け I2C 接続タッチパッド
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

// ピン設定 ///////////////////////////////////////////////////////////////////
// 使用するGPIO端子, I2Cインスタンスを変更する場合はここを変更
//////////////////////////////////////////////////////////////////////////////

#define TOUCH_RST (10)   //< RSTピン  [OUT]
#define TOUCH_INTR (11)  //< INTRピン [IN]
#define TOUCH_SDA (12)   //< I2C SDA
#define TOUCH_SCL (13)   //< I2C SCL

#define TOUCH_I2C (i2c0)  //< 使用するI2Cインスタンス

// デバイス設定 ////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#define TOUCH_I2C_ADDR (0x1a)  //< I2Cアドレス

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief タッチパッド諸元. 初期化時に取得する情報
 */
typedef struct tagCST328Spec_t {
  uint16_t xmax;
  uint16_t ymax;
  uint32_t boottime;
  uint8_t info[24];
} CST328Spec_t;

typedef enum tagCST328Error_t {
  ERR_DEBUGMODE_CMD = 0,
  ERR_INFO_BOOTTIME_CMD,
  ERR_INFO_BOOTTIME,
  ERR_INFO_RES_CMD,
  ERR_INFO_RES,
  ERR_INFO_TP_NTX_CMD,
  ERR_INFO_TP_NTX,
  ERR_NORMALMODE_CMD,
  ERR_READ_NUMBER_CMD,
  ERR_READ_NUMBER,
  ERR_READ_XY_CMD,
  ERR_READ_XY,
  ERR_CLEAR_NUMBER_CMD,
  ERR_MAX,
} CST328Error_t;

/**
 * @brief タッチパッド状態
 */
typedef struct tagTouchDrvContext_t {
  i2c_inst_t* i2c;  //< I2Cインスタンス
  uint8_t addr;     //< タッチパッドのI2Cアドレス
  uint32_t sda;     //< SDA GPIO番号
  uint32_t scl;     //< SCL GPIO番号
  uint32_t rst;     //< RST GPIO番号
  uint32_t intr;    //< INTR GPIO番号

  bool active;              //< 初期化状態フラグ
  volatile uint32_t event;  //< 割り込みイベントフラグ

  CST328Spec_t spec;      //< タッチパッド諸元
  CST328Data_t data;      //< データ
  uint32_t err[ERR_MAX];  //< エラー情報
} TouchDrvContext_t;

static TouchDrvContext_t builtinContext[] = {
    {
        .i2c = TOUCH_I2C,
        .addr = TOUCH_I2C_ADDR,
        .sda = TOUCH_SDA,
        .scl = TOUCH_SCL,
        .rst = TOUCH_RST,
        .intr = TOUCH_INTR,
        .active = false,
        .event = 0,
        .spec = {0},
        .data = {0},
        .err = {0},
    },
};

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief GPIO割り込みハンドラ
 * 該当インスタンスの割り込みイベントフラグをセットする
 * @param [in] gpio : GPIO番号
 * @param [in] events : 割り込みイベント
 */
static void TouchDrv_gpio_callback(uint gpio, uint32_t events);

/**
 * I2C データ出力 処理
 * @param [in] ctx : 操作対象
 * @param [in] data : 出力データ
 * @param [in] size : 出力サイズ
 * @return 出力したバイト数
 * @retval 1以上 : 出力したバイト数
 * @retval 0 : エラー発生
 */
static inline size_t TouchDrv_i2cWrite(TouchDrvContext_t* ctx, const void* data, size_t size);

/**
 * I2C データ入力 処理
 * @param [in] ctx : 操作対象
 * @param [out] data : データ出力先
 * @param [in] size : 入力サイズ
 * @return 入力したバイト数
 * @retval 1以上 : 入力したバイト数
 * @retval 0 : エラー発生
 */
static inline size_t TouchDrv_i2cRead(TouchDrvContext_t* ctx, void* data, size_t size);

/**
 * @brief ラズパイ側の利用ブロック初期化
 * I2C, GPIO の初期化, 設定を行う
 * @param [in] ctx : 対象のタッチパッドインスタンス
 * @return 処理結果
 */
static UError_t TouchDrv_platformInit(TouchDrvContext_t* ctx);

/**
 * @brief デバイス側のハードウェア初期化
 * リセット信号を操作
 * @param [in] ctx : 操作対象のタッチパッド
 * @return 処理結果
 */
static UError_t TouchDrv_hwInit(TouchDrvContext_t* ctx);

/**
 * @brief タッチパッドデバイス側初期化処理
 * @param ctx : 操作対象
 * @return 処理結果
 */
static UError_t TouchDrv_deviceInit(TouchDrvContext_t* ctx);

/**
 * @brief デバイスからタッチ情報を取得し, 内部状態を更新する.
 * @param [in] ctx : 操作対象
 * @return 処理結果
 */
static UError_t TouchDrv_getPosition(TouchDrvContext_t* ctx);

/**
 * タッチパッドを初期化する
 * @param [in] ctx : 操作対象
 * @return 処理結果
 * @note SDA(灰色) SCL(青) RST(紫:out) INT(黄:in)
 */
static UError_t TouchDrv_init(TouchDrvContext_t* ctx);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

// タッチパッド コマンドバイト列
static const uint8_t TOUCHDRV_DEBUG_INFO_MODE[] = {0xd1, 0x01};
static const uint8_t TOUCHDRV_NORMAL_MODE[] = {0xd1, 0x09};
static const uint8_t TOUCHDRV_DEBUG_INFO_BOOTTIME[] = {0xd1, 0xfc};
static const uint8_t TOUCHDRV_DEBUG_INFO_RES[] = {0xd1, 0xf8};
static const uint8_t TOUCHDRV_DEBUG_INFO_TP_NTX[] = {0xd1, 0xf4};

static const uint8_t TOUCHDRV_READ_NUMBER[] = {0xd0, 05};
static const uint8_t TOUCHDRV_CLEAR_NUMBER[] = {0xd0, 0x05, 0x00};
static const uint8_t TOUCHDRV_READ_XY[] = {0xd0, 0x00};

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

static void TouchDrv_gpio_callback(uint gpio, uint32_t events) {
  for (size_t i = 0; i < sizeof(builtinContext) / sizeof(builtinContext[0]); ++i) {
    if (builtinContext[i].intr == gpio && builtinContext[i].active) {
      builtinContext[i].event = 1u;
      break;
    }
  }
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
    uint hz = i2c_init(ctx->i2c, 1000 * 1000);  // 1MHz
    gpio_set_pulls(ctx->scl, false, false);     // デバイス側でPULLUP
    gpio_set_pulls(ctx->sda, false, false);     // デバイス側でPULLUP
    gpio_set_function_masked((1 << ctx->sda) | (1 << ctx->scl), GPIO_FUNC_I2C);

    // RST/INTピン
    gpio_set_pulls(ctx->intr, true, false);  // ラズパイ側でPULLUP
    gpio_init_mask((1 << ctx->rst) | (1 << ctx->intr));
    gpio_set_function_masked((1 << ctx->rst) | (1 << ctx->intr), GPIO_FUNC_SIO);

    // RSTピン
    gpio_put(ctx->rst, false);  // LOWにしてからOUTに切り替え
    gpio_set_dir(ctx->rst, GPIO_OUT);

    // INTピン
    gpio_set_dir(ctx->intr, GPIO_IN);
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
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    gpio_put(ctx->rst, false);
    sleep_ms(1);
    gpio_put(ctx->rst, true);
    sleep_ms(150);

    // INTRの立下り割り込みを有効化
    // gpio_set_irq_enabled(, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled_with_callback(ctx->intr, GPIO_IRQ_EDGE_FALL, true, &TouchDrv_gpio_callback);
  }

  return err;
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
      err = uFailure;
    }
  }

  // I2C による HW初期化
  uint8_t data[64] = {0};

  // ハードウェア諸元の取得

  // DEBUG INFO MODE へ遷移
  if (uSuccess == err) {
    if (2u != TouchDrv_i2cWrite(ctx, TOUCHDRV_DEBUG_INFO_MODE, 2)) {
      ctx->err[ERR_DEBUGMODE_CMD]++;
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    // BOOT TIME 取得
    if (2u != TouchDrv_i2cWrite(ctx, TOUCHDRV_DEBUG_INFO_BOOTTIME, 2)) {
      ctx->err[ERR_INFO_BOOTTIME_CMD]++;
      err = uFailure;
    } else {
      if (4u == TouchDrv_i2cRead(ctx, data, 4)) {
        ctx->spec.boottime = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3] << 0);
      } else {
        ctx->err[ERR_INFO_BOOTTIME]++;
        err = uFailure;
      }
    }
  }

  if (uSuccess == err) {
    // X/Y 解像度取得
    if (2u != TouchDrv_i2cWrite(ctx, TOUCHDRV_DEBUG_INFO_RES, 2)) {
      ctx->err[ERR_INFO_RES_CMD]++;
      err = uFailure;
    } else {
      if (4u == TouchDrv_i2cRead(ctx, data, 4)) {
        ctx->spec.xmax = data[1] * 256u + data[0];
        ctx->spec.ymax = data[3] * 256u + data[2];
      } else {
        ctx->err[ERR_INFO_RES]++;
        err = uFailure;
      }
    }
  }

  if (uSuccess == err) {
    // 諸元取得
    if (2u != TouchDrv_i2cWrite(ctx, TOUCHDRV_DEBUG_INFO_TP_NTX, 2)) {
      ctx->err[ERR_INFO_TP_NTX_CMD]++;
      err = uFailure;
    } else {
      if (24u == TouchDrv_i2cRead(ctx, &ctx->spec.info, 24)) {
        if (0xca != ctx->spec.info[10] || 0xca != ctx->spec.info[11]) {
          err = uFailure;
        }
      } else {
        ctx->err[ERR_INFO_TP_NTX]++;
        err = uFailure;
      }
    }
  }

  if (uSuccess == err) {
    // NORMALモードへ遷移
    if (2u != TouchDrv_i2cWrite(ctx, TOUCHDRV_NORMAL_MODE, 2)) {
      ctx->err[ERR_NORMALMODE_CMD]++;
      err = uFailure;
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
    if (2u != TouchDrv_i2cWrite(ctx, TOUCHDRV_READ_NUMBER, 2)) {
      ctx->err[ERR_READ_NUMBER_CMD]++;
      err = uFailure;
    }
  }

  // 有効データ数取得
  size_t num = 0u;
  if (uSuccess == err) {
    uint8_t data[1] = {0};
    if (1u != TouchDrv_i2cRead(ctx, data, 1)) {
      ctx->err[ERR_READ_NUMBER]++;
      err = uFailure;
    } else {
      num = 0x0f & data[0];
    }
  }

  // ポジション取得
  uint8_t posData[27] = {0};
  if (uSuccess == err && (0 != num && 5 >= num)) {
    if (2u != TouchDrv_i2cWrite(ctx, TOUCHDRV_READ_XY, 2)) {
      ctx->err[ERR_READ_XY_CMD]++;
      err = uFailure;
    } else {
      if (27u != TouchDrv_i2cRead(ctx, posData, 27)) {
        ctx->err[ERR_READ_XY]++;
        err = uFailure;
      }
    }
  }

  // ポジション情報クリア
  if (uSuccess == err) {
    if (3u != TouchDrv_i2cWrite(ctx, TOUCHDRV_CLEAR_NUMBER, 3)) {
      ctx->err[ERR_CLEAR_NUMBER_CMD]++;
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    // 内部の位置情報を更新
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
