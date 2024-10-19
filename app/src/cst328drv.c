/**
 * @file app/src/cst328drv.c
 * Raspberry PI PICO C-SDK 向け
 * WAVEWARE-27579 2.0インチ静電容量タッチLCD(240x320) 向け I2C 接続タッチパッド
 * CST328 タッチコントローラ
 * https://www.buydisplay.com/download/ic/CST328_Datasheet.pdf
 * @date 2024.10.13 k.shibata newly created
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/cst328drv.h>

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

#define CST328_RST (10)   //< RSTピン  [OUT]
#define CST328_INTR (11)  //< INTRピン [IN]

#define CST328_SDA (12)    //< I2C SDA
#define CST328_SCL (13)    //< I2C SCL
#define CST328_I2C (i2c0)  //< 使用するI2Cインスタンス

// デバイス設定 ////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#define CST328_I2C_ADDR (0x1a)  //< I2Cアドレス

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

/**
 * CST328 データバス処理インタフェース
 */
typedef struct tagCST328Bus_t {
  UError_t (*init)(void* self);
  int32_t (*write)(void* self, const void* data, size_t size, bool nostop);
  int32_t (*read)(void* self, void* data, size_t size, bool nostop);
} CST328Bus_t;

/**
 * @brief PICO I2C を使ったCST328 データバス処理
 */
typedef struct tagCST328_I2CBus_t {
  CST328Bus_t bus;  // データバス処理インタフェース
  i2c_inst_t* i2c;
  uint8_t addr;  //< タッチパッドのI2Cアドレス
  uint32_t sda;  //< SDA GPIO番号
  uint32_t scl;  //< SCL GPIO番号
} CST328_I2CBus_t;

/**
 * @brief タッチパッド諸元. 初期化時に取得する情報
 */
typedef struct tagCST328Spec_t {
  uint16_t xmax;
  uint16_t ymax;
  uint32_t boottime;
  uint8_t info[24];
} CST328Spec_t;

/**
 * @brief CST328 エラー状態
 */
typedef enum tagCST328Error_t {
  CST328_ERR_DEBUGMODE_CMD = 0,
  CST328_ERR_INFO_BOOTTIME_CMD,
  CST328_ERR_INFO_BOOTTIME,
  CST328_ERR_INFO_RES_CMD,
  CST328_ERR_INFO_RES,
  CST328_ERR_INFO_TP_NTX_CMD,
  CST328_ERR_INFO_TP_NTX,
  CST328_ERR_NORMALMODE_CMD,
  CST328_ERR_READ_NUMBER_CMD,
  CST328_ERR_READ_NUMBER,
  CST328_ERR_READ_XY_CMD,
  CST328_ERR_READ_XY,
  CST328_ERR_CLEAR_NUMBER_CMD,
  CST328_ERR_MAX,
} CST328Error_t;

/**
 * @brief タッチパッド状態
 */
typedef struct tagCST328DrvContext_t {
  uint32_t rst;   //< RST GPIO番号
  uint32_t intr;  //< INTR GPIO番号

  bool active;              //< 初期化状態フラグ
  volatile uint32_t event;  //< 割り込みイベントフラグ

  CST328Spec_t spec;             //< タッチパッド諸元
  CST328Bus_t* bus;              //< IO write/read
  CST328Data_t data;             //< データ
  uint32_t err[CST328_ERR_MAX];  //< エラー情報
} CST328DrvContext_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief I2C バスを初期化する
 * @param [in] self : 処理対象
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
static UError_t CST328_i2c_init(void* self);

/**
 * @brief I2C バスにデータを出力する
 * @param [in] self : 処理対象
 * @param [in] data : 出力するデータ
 * @param [in] size : 出力するデータサイズ
 * @param nostop : restart condition を発行するか否か
 *   @arg true : restart condition を発行し, バスの占有を継続する
 *   @arg false : stop condition を発行しバスを解放する
 * @return 出力データバイト数
 * @retval 1 以上 : 出力データバイト数
 * @retval 0 以下 : 処理失敗
 */
static int32_t CST328_i2c_write(void* self, const void* data, size_t size, bool nostop);

/**
 * @brief I2C バスからデータを読み取る
 * @param [in] self : 処理対象
 * @param [out] data : データ出力先
 * @param [in] size : 読み取りデータサイズ
 * @param nostop : restart condition を発行するか否か
 *   @arg true : restart condition を発行し, バスの占有を継続する
 *   @arg false : stop condition を発行しバスを解放する
 * @return 読み取りデータバイト数
 * @retval 1 以上 : 読み取りデータバイト数
 * @retval 0 以下 : 処理失敗
 */
static int32_t CST328_i2c_read(void* self, void* data, size_t size, bool nostop);

/**
 * @brief CST328 タッチ通知ハンドラ
 * 該当インスタンスの割り込みイベントフラグをセットする
 * @param [in] gpio : GPIO番号
 * @param [in] events : 割り込みイベント
 */
static void CST328Drv_INTR_callback(uint gpio, uint32_t events);

/**
 * @brief バスにデータを出力する
 * @param [in] ctx : 操作対象
 * @param [in] data : 出力データ
 * @param [in] size : 出力サイズ
 * @return 出力したバイト数
 */
static inline size_t CST328Drv_Write(CST328DrvContext_t* ctx, const void* data, size_t size);

/**
 * @brief バスからデータを読み取る
 * @param [in] self : 処理対象
 * @param [out] data : データ出力先
 * @param [in] size : 読み取りデータサイズ
 * @return 読み取りデータバイト数
 */
static inline size_t CST328Drv_Read(CST328DrvContext_t* ctx, void* data, size_t size);

/**
 * @brief Debug Info Mode への遷移要求を発行する
 * @param [in] ctx : 処理対象
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
inline static UError_t CST328Drv_setupDebugInfoMode(CST328DrvContext_t* ctx);

/**
 * @brief Normal Mode への遷移要求を発行する
 * @param [in] ctx : 処理対象
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
inline static UError_t CST328Drv_setupNormalMode(CST328DrvContext_t* ctx);

/**
 * @brief BOOT TIME 情報を取得し, 状態を更新する
 * @param [in] ctx : 処理対象
 * @return 処理結果
 * @retval uSuccess : 処理成功. ctx内部情報を更新する
 * @retval uSuccess 以外 : 処理失敗
 */
inline static UError_t CST328Drv_getDebugBootTime(CST328DrvContext_t* ctx);

/**
 * @brief 解像度情報を取得し, 状態を更新する
 * @param [in] ctx : 処理対象
 * @return 処理結果
 * @retval uSuccess : 処理成功. ctx内部情報を更新する
 * @retval uSuccess 以外 : 処理失敗
 */
inline static UError_t CST328Drv_getResolution(CST328DrvContext_t* ctx);

/**
 * @brief 諸元情報を取得し, 状態を更新する
 * @param [in] ctx : 処理対象
 * @return 処理結果
 * @retval uSuccess : 処理成功. ctx内部情報を更新する
 * @retval uSuccess 以外 : 処理失敗
 */
inline static UError_t CST328Drv_getSpec(CST328DrvContext_t* ctx);

/**
 * @brief タッチ情報を取得し, 内部状態を更新する.
 * @param [in] ctx : 操作対象
 * @return 処理結果
 * @retval uSuccess : 処理成功. ctx内部情報を更新する
 * @retval uSuccess 以外 : 処理失敗
 */
inline static UError_t CST328Drv_getPosition(CST328DrvContext_t* ctx);

/**
 * @brief ラズパイ側の利用ブロック初期化
 * I2C, GPIO の初期化, 設定を行う
 * @param [in] ctx : 対象のタッチパッドインスタンス
 * @return 処理結果
 */
static UError_t CST328Drv_platformInit(CST328DrvContext_t* ctx);

/**
 * @brief デバイス側のハードウェア初期化
 * リセット信号を操作
 * @param [in] ctx : 操作対象のタッチパッド
 * @return 処理結果
 */
static UError_t CST328Drv_hwInit(CST328DrvContext_t* ctx);

/**
 * @brief タッチパッドデバイス側初期化処理
 * @param ctx : 操作対象
 * @return 処理結果
 */
static UError_t CST328Drv_deviceInit(CST328DrvContext_t* ctx);

/**
 * タッチパッドを初期化する
 * @param [in] ctx : 操作対象
 * @return 処理結果
 * @note SDA(灰色) SCL(青) RST(紫:out) INT(黄:in)
 */
static UError_t CST328Drv_init(CST328DrvContext_t* ctx);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

// タッチパッド コマンドバイト列
static const uint8_t CST328_DEBUG_INFO_MODE[] = {0xd1, 0x01};
static const uint8_t CST328_NORMAL_MODE[] = {0xd1, 0x09};
static const uint8_t CST328_DEBUG_INFO_BOOTTIME[] = {0xd1, 0xfc};
static const uint8_t CST328_DEBUG_INFO_RES[] = {0xd1, 0xf8};
static const uint8_t CST328_DEBUG_INFO_TP_NTX[] = {0xd1, 0xf4};

static const uint8_t CST328_READ_NUMBER[] = {0xd0, 05};
static const uint8_t CST328_CLEAR_NUMBER[] = {0xd0, 0x05, 0x00};
static const uint8_t CST328_READ_XY[] = {0xd0, 0x00};

static CST328_I2CBus_t builtinCST328Bus[] = {
    {
        .i2c = CST328_I2C,
        .addr = CST328_I2C_ADDR,
        .sda = CST328_SDA,
        .scl = CST328_SCL,
        .bus =
            {
                .init = &CST328_i2c_init,
                .write = &CST328_i2c_write,
                .read = &CST328_i2c_read,
            },
    },
};

static CST328DrvContext_t builtinContext[] = {
    {
        .bus = (CST328Bus_t*)&builtinCST328Bus[0],
        .rst = CST328_RST,
        .intr = CST328_INTR,
        .active = false,
        .event = 0,
        .spec = {0},
        .data = {0},
        .err = {0},
    },
};

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

#define RESERVED_ADDR(ADDR) ((ADDR & 0x78) == 0 || (ADDR & 0x78) == 0x78)

/**
 * i2c バスからデータを読み込みます
 * PICO SDK i2c_read_blocking の改良処理.
 * @param i2c
 * @param addr
 * @param dst
 * @param len
 * @param nostop
 * @return
 */
static int read_blocking(i2c_inst_t* i2c, uint8_t addr, uint8_t* dst, size_t len, bool nostop) {
  if (RESERVED_ADDR(addr)) {
    return PICO_ERROR_GENERIC;
  }
  i2c->hw->enable = 0;
  i2c->hw->tar = addr;
  i2c->hw->enable = 1;

  bool abort = false;
  volatile uint32_t abort_reason = 0u;
  int byte_ctr;
  int ilen = (int)len;
  for (byte_ctr = 0; byte_ctr < ilen; ++byte_ctr) {
    bool first = (0 == byte_ctr);
    bool last = ((ilen - 1) == byte_ctr);

    while (!i2c_get_write_available(i2c)) {
      tight_loop_contents();
    }

    i2c->hw->data_cmd = (bool_to_bit(first && i2c->restart_on_next) << I2C_IC_DATA_CMD_RESTART_LSB) |
                        (bool_to_bit(last && !nostop) << I2C_IC_DATA_CMD_STOP_LSB) | I2C_IC_DATA_CMD_CMD_BITS;  // 1 for read

    do {
      if (i2c->hw->raw_intr_stat & I2C_IC_INTR_STAT_R_TX_ABRT_BITS) {
        abort_reason = i2c->hw->tx_abrt_source;
        abort = (bool)i2c->hw->clr_tx_abrt;
      }
    } while (!abort && !i2c_get_read_available(i2c));

    if (abort) {
      break;
    }

    *dst++ = (uint8_t)i2c->hw->data_cmd;
  }

  int rval;
  if (abort) {
    if (!abort_reason || (abort_reason & I2C_IC_TX_ABRT_SOURCE_ABRT_7B_ADDR_NOACK_BITS)) {
      rval = PICO_ERROR_GENERIC;
    } else {
      rval = PICO_ERROR_GENERIC;
    }
  } else {
    rval = byte_ctr;
  }

  i2c->restart_on_next = nostop;

  return rval;
}

static UError_t CST328_i2c_init(void* self) {
  UError_t err = uSuccess;
  CST328_I2CBus_t* bus = (CST328_I2CBus_t*)self;

  if (uSuccess == err) {
    if (NULL == bus || NULL == bus->i2c) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    i2c_init(bus->i2c, 400 * 1000);          // 400kHz;
    gpio_set_pulls(bus->scl, false, false);  // デバイス側でPULLUP
    gpio_set_pulls(bus->sda, false, false);  // デバイス側でPULLUP
    gpio_set_function_masked((1 << bus->sda) | (1 << bus->scl), GPIO_FUNC_I2C);
  }

  return err;
}

static int32_t CST328_i2c_write(void* self, const void* data, size_t size, bool nostop) {
  UError_t err = uSuccess;
  CST328_I2CBus_t* bus = (CST328_I2CBus_t*)self;

  if (uSuccess == err) {
    if (NULL == bus || NULL == bus->i2c) {
      err = uFailure;
    }
  }

  int32_t ret = 0;
  if (uSuccess == err) {
    int result = i2c_write_blocking(bus->i2c, bus->addr, data, size, nostop);
    if (PICO_ERROR_GENERIC == result) {
      err = uFailure;
    } else {
      ret = (int32_t)result;
    }
  }

  return ret;
}

static int32_t CST328_i2c_read(void* self, void* data, size_t size, bool nostop) {
  UError_t err = uSuccess;
  CST328_I2CBus_t* bus = (CST328_I2CBus_t*)self;

  if (uSuccess == err) {
    if (NULL == bus || NULL == bus->i2c) {
      err = uFailure;
    }
  }

  int32_t ret = 0;

  if (uSuccess == err) {
    int result = read_blocking(bus->i2c, bus->addr, data, size, nostop);
    if (PICO_ERROR_GENERIC == result) {
      err = uFailure;
    } else {
      ret = (int32_t)result;
    }
  }

  return ret;
}

static void CST328Drv_INTR_callback(uint gpio, uint32_t events) {
  for (size_t i = 0; i < sizeof(builtinContext) / sizeof(builtinContext[0]); ++i) {
    if (builtinContext[i].intr == gpio && builtinContext[i].active) {
      builtinContext[i].event = 1u;
      break;
    }
  }
}

static inline size_t CST328Drv_Write(CST328DrvContext_t* ctx, const void* data, size_t size) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == data || 0 == size || NULL == ctx->bus || NULL == ctx->bus->write) {
      err = uFailure;
    }
  }

  size_t ret = 0u;
  if (uSuccess == err) {
    int result = ctx->bus->write(ctx->bus, data, size, false);
    // int result = i2c_write_blocking(ctx->i2c, ctx->addr, data, size, false);
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

static inline size_t CST328Drv_Read(CST328DrvContext_t* ctx, void* data, size_t size) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == data || 0 == size || NULL == ctx->bus || NULL == ctx->bus->read) {
      err = uFailure;
    }
  }

  size_t ret = 0u;
  if (uSuccess == err) {
    int result = ctx->bus->read(ctx->bus, data, size, false);
    // int result = i2c_read_blocking(ctx->i2c, ctx->addr, data, size, false);
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

inline static UError_t CST328Drv_setupDebugInfoMode(CST328DrvContext_t* ctx) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  // DEBUG INFO MODE へ遷移
  if (uSuccess == err) {
    if (2u != CST328Drv_Write(ctx, CST328_DEBUG_INFO_MODE, 2)) {
      ctx->err[CST328_ERR_DEBUGMODE_CMD]++;
      err = uFailure;
    }
  }

  return err;
}

inline static UError_t CST328Drv_setupNormalMode(CST328DrvContext_t* ctx) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    // NORMALモードへ遷移
    if (2u != CST328Drv_Write(ctx, CST328_NORMAL_MODE, 2)) {
      ctx->err[CST328_ERR_NORMALMODE_CMD]++;
      err = uFailure;
    }
  }
}

inline static UError_t CST328Drv_getDebugBootTime(CST328DrvContext_t* ctx) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    // BOOT TIME 取得
    uint8_t data[4] = {0};
    if (2u != CST328Drv_Write(ctx, CST328_DEBUG_INFO_BOOTTIME, 2)) {
      ctx->err[CST328_ERR_INFO_BOOTTIME_CMD]++;
      err = uFailure;
    } else {
      if (4u == CST328Drv_Read(ctx, data, 4)) {
        ctx->spec.boottime = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3] << 0);
      } else {
        ctx->err[CST328_ERR_INFO_BOOTTIME]++;
        err = uFailure;
      }
    }
  }

  return err;
}

inline static UError_t CST328Drv_getResolution(CST328DrvContext_t* ctx) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    uint8_t data[4] = {0};
    // X/Y 解像度取得
    if (2u != CST328Drv_Write(ctx, CST328_DEBUG_INFO_RES, 2)) {
      ctx->err[CST328_ERR_INFO_RES_CMD]++;
      err = uFailure;
    } else {
      if (4u == CST328Drv_Read(ctx, data, 4)) {
        ctx->spec.xmax = data[1] * 256u + data[0];
        ctx->spec.ymax = data[3] * 256u + data[2];
      } else {
        ctx->err[CST328_ERR_INFO_RES]++;
        err = uFailure;
      }
    }
  }

  return err;
}

inline static UError_t CST328Drv_getSpec(CST328DrvContext_t* ctx) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    uint8_t data[24] = {0};
    // 諸元取得
    if (2u != CST328Drv_Write(ctx, CST328_DEBUG_INFO_TP_NTX, 2)) {
      ctx->err[CST328_ERR_INFO_TP_NTX_CMD]++;
      err = uFailure;
    } else {
      if (24u == CST328Drv_Read(ctx, &ctx->spec.info, 24)) {
        if (0xca != ctx->spec.info[10] || 0xca != ctx->spec.info[11]) {
          err = uFailure;
        }
      } else {
        ctx->err[CST328_ERR_INFO_TP_NTX]++;
        err = uFailure;
      }
    }
  }

  return err;
}

inline static UError_t CST328Drv_getPosition(CST328DrvContext_t* ctx) {
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

  // ポジション取得
  size_t num = 0u;
  uint8_t infoData[27] = {0};
  if (uSuccess == err) {
    if (2u != CST328Drv_Write(ctx, CST328_READ_XY, 2)) {
      ctx->err[CST328_ERR_READ_XY_CMD]++;
      err = uFailure;
    } else {
      if (27u != CST328Drv_Read(ctx, infoData, 27)) {
        ctx->err[CST328_ERR_READ_XY]++;
        err = uFailure;
      } else {
        num = 0x0f & infoData[0x05];
        num = (5 < num) ? 0 : num;
      }
    }
  }

  // ポジション情報クリア
  if (uSuccess == err) {
    if (3u != CST328Drv_Write(ctx, CST328_CLEAR_NUMBER, 3)) {
      ctx->err[CST328_ERR_CLEAR_NUMBER_CMD]++;
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    // 内部の位置情報を更新
    ctx->data.points = num;
    for (size_t i = 0; i < num; ++i) {
      size_t n = (i > 0) ? 2 : 0;
      ctx->data.coords[i].id = (0x0f & (infoData[(i * 5) + 0 + n] >> 4));
      ctx->data.coords[i].status = (0x0f & (infoData[(i * 5) + 1 + n]));
      ctx->data.coords[i].x = (infoData[(i * 5) + 1 + n] << 4) + ((infoData[(i * 5) + 3 + n] & 0xf0) >> 4);
      ctx->data.coords[i].y = (infoData[(i * 5) + 2 + n] << 4) + (infoData[(i * 5) + 3 + n] & 0x0f);
      ctx->data.coords[i].strength = (infoData[(i * 5) + 4 + n]);
    }
  } else {
    ctx->data.points = 0;
  }

  return err;
}

static UError_t CST328Drv_platformInit(CST328DrvContext_t* ctx) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == ctx->bus || NULL == ctx->bus->init) {
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
    // データバス設定
    err = ctx->bus->init(ctx->bus);
  }

  if (uSuccess == err) {
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

static UError_t CST328Drv_hwInit(CST328DrvContext_t* ctx) {
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
    gpio_set_irq_enabled_with_callback(ctx->intr, GPIO_IRQ_EDGE_FALL, true, &CST328Drv_INTR_callback);
  }

  return err;
}

static UError_t CST328Drv_deviceInit(CST328DrvContext_t* ctx) {
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

  // ハードウェア諸元の取得

  // DEBUG INFO MODE へ遷移
  if (uSuccess == err) {
    err = CST328Drv_setupDebugInfoMode(ctx);
  }

  if (uSuccess == err) {
    // BOOT TIME 取得
    err = CST328Drv_getDebugBootTime(ctx);
  }

  if (uSuccess == err) {
    // X/Y 解像度取得
    err = CST328Drv_getResolution(ctx);
  }

  if (uSuccess == err) {
    // 諸元取得
    err = CST328Drv_getSpec(ctx);
  }

  if (uSuccess == err) {
    // NORMALモードへ遷移
    err = CST328Drv_setupNormalMode(ctx);
  }

  if (uSuccess == err) {
    ctx->active = true;
  }

  return err;
}

static UError_t CST328Drv_init(CST328DrvContext_t* ctx) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    err = CST328Drv_platformInit(ctx);
  }

  if (uSuccess == err) {
    err = CST328Drv_hwInit(ctx);
  }

  if (uSuccess == err) {
    err = CST328Drv_deviceInit(ctx);
  }
  return err;
}

// interface

UError_t CST328Drv_Open(CST328DrvHandle_t* pHandle) {
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

UError_t CST328Drv_Init(CST328DrvHandle_t handle) {
  UError_t err = uSuccess;
  CST328DrvContext_t* ctx = (CST328DrvContext_t*)handle;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    err = CST328Drv_init(ctx);
  }

  return err;
}

UError_t CST328Drv_UpdateCoord(CST328DrvHandle_t handle) {
  UError_t err = uSuccess;
  CST328DrvContext_t* ctx = (CST328DrvContext_t*)handle;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (ctx->event) {
      ctx->event = 0;
      err = CST328Drv_getPosition(ctx);
    } else {
      ctx->data.points = 0;
    }
  }
  return err;
}

UError_t CST328Drv_GetCoord(CST328DrvHandle_t handle, CST328Data_t* pCoord) {
  UError_t err = uSuccess;
  CST328DrvContext_t* ctx = (CST328DrvContext_t*)handle;

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
