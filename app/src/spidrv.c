/**
 * @file prog01/app/src/spidrv.c
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdio.h>

#include <pico/stdlib.h>

#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <hardware/spi.h>

#include <user/macros.h>
#include <user/spidrv.h>
#include <user/types.h>

#include <pico/binary_info.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

#define SPI0_SCK_PIN (2)
#define SPI0_TX_PIN (3)
#define SPI0_RX_PIN (4)
#define SPI0_CSN_PIN (5)

bi_decl(bi_1pin_with_name(SPI0_SCK_PIN, "SPI0_SCK_PIN"));
bi_decl(bi_1pin_with_name(SPI0_TX_PIN, "SPI0_TX_PIN"));
bi_decl(bi_1pin_with_name(SPI0_RX_PIN, "SPI0_RX_PIN"));
bi_decl(bi_1pin_with_name(SPI0_CSN_PIN, "SPI0_CSN_PIN"));

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

typedef struct tagSPIDrvAsyncContext_t {
  uint32_t tx;
  uint32_t rx;
  absolute_time_t begin;
} SPIDrvAsyncContext_t;

typedef struct tagSPIDrvContext_t {
  spi_inst_t* const hw;  //< PICO SPIハードウェア
  uint32_t baudrate;     //< ボーレート
  //
  const uint32_t sck;
  const uint32_t rx;
  const uint32_t tx;
  const uint32_t csn;
  //
  SPIDrvAsyncContext_t async;
} SPIDrvContext_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief SPI CSN 操作
 * @param
 */
static inline void SPIDrv_CS(const SPIDrvContext_t* ctx, uint32_t value);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

static SPIDrvContext_t builtinContext[] = {
    {
        .hw = spi0,
        .rx = SPI0_RX_PIN,
        .tx = SPI0_TX_PIN,
        .sck = SPI0_SCK_PIN,
        .csn = SPI0_CSN_PIN,
        .baudrate = 25 * 1000 * 1000,
        .async =
            {
                .tx = 0,
                .rx = 0,
                .begin = 0,
            },
    },
};

static SPIDrvContext_t* builtinContextInUse[] = {
    NULL,
};

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

static inline void SPIDrv_CS(const SPIDrvContext_t* ctx, uint32_t value) {
  NOP3();
  gpio_put(ctx->csn, value);
  NOP3();
}

UError_t SPIDrv_Open(SPIDrvHandle_t* handle) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (NULL != builtinContextInUse[0]) {
      err = uFailure;  // already opened
    }
  }

  if (uSuccess == err) {
    builtinContextInUse[0] = &builtinContext[0];
    *handle = builtinContextInUse[0];
  }

  return err;
}

void SPIDrv_Close(SPIDrvHandle_t handle) {
  UError_t err = uSuccess;
  SPIDrvContext_t* ctx = (SPIDrvContext_t*)handle;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    for (size_t i = 0; i < (sizeof(builtinContextInUse) / sizeof(builtinContextInUse[0])); ++i) {
      if (builtinContextInUse[i] == ctx) {
        builtinContextInUse[i] = NULL;
        break;
      }
    }
  }
}

UError_t SPIDrv_Init(SPIDrvHandle_t handle, uint32_t baudrate) {
  UError_t err = uSuccess;
  SPIDrvContext_t* const ctx = (SPIDrvContext_t*)handle;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    ctx->baudrate = spi_init(ctx->hw, baudrate);
    gpio_set_function(ctx->sck, GPIO_FUNC_SPI);
    gpio_set_function(ctx->rx, GPIO_FUNC_SPI);
    gpio_set_function(ctx->tx, GPIO_FUNC_SPI);

    gpio_init(ctx->csn);
    gpio_put(ctx->csn, 1);
    gpio_set_dir(ctx->csn, GPIO_OUT);
  }

  return err;
}

UError_t SPIDrv_SendByte(const SPIDrvHandle_t handle, const uint8_t data) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    const SPIDrvContext_t* const ctx = (const SPIDrvContext_t*)handle;
    SPIDrv_CS(ctx, 0);
    spi_write_blocking(ctx->hw, &data, 1);
    SPIDrv_CS(ctx, 1);
  }

  return err;
}

UError_t SPIDrv_RecvByte(const SPIDrvHandle_t handle, uint8_t* data) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle || NULL == data) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    const SPIDrvContext_t* const ctx = (const SPIDrvContext_t*)handle;
    SPIDrv_CS(ctx, 0);
    spi_read_blocking(ctx->hw, 0u, data, 1);
    SPIDrv_CS(ctx, 1);
  }

  return err;
}

UError_t SPIDrv_TransferByte(const SPIDrvHandle_t handle, const uint8_t tx, uint8_t* rx) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle || NULL == rx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    const SPIDrvContext_t* const ctx = (const SPIDrvContext_t*)handle;
    SPIDrv_CS(ctx, 0);
    int ret = spi_write_read_blocking(ctx->hw, &tx, rx, 1);
    SPIDrv_CS(ctx, 1);
    if (1 != ret) {
      err = uFailure;
    }
  }

  return err;
}

UError_t SPIDrv_SendNBytes(const SPIDrvHandle_t handle, const void* data, size_t size) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    const SPIDrvContext_t* const ctx = (const SPIDrvContext_t*)handle;
    SPIDrv_CS(ctx, 0);
    spi_write_blocking(ctx->hw, data, size);
    SPIDrv_CS(ctx, 1);
  }

  return err;
}

UError_t SPIDrv_RecvNBytes(const SPIDrvHandle_t handle, void* data, size_t size) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle || NULL == data || 0 == size) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    const SPIDrvContext_t* const ctx = (const SPIDrvContext_t*)handle;
    SPIDrv_CS(ctx, 0);
    spi_read_blocking(ctx->hw, 0u, data, size);
    SPIDrv_CS(ctx, 1);
  }

  return err;
}

UError_t SPIDrv_AsyncSend(SPIDrvHandle_t handle, const void* tx, size_t size) {
  UError_t err = uSuccess;
  static uint8_t null = 0u;

  if (uSuccess == err) {
    if (NULL == handle || NULL == tx || 0 == size) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    SPIDrvContext_t* const ctx = (SPIDrvContext_t*)handle;

    const uint32_t dma_tx = dma_claim_unused_channel(true);
    dma_channel_config config = dma_channel_get_default_config(dma_tx);

    // DMAC 書き込み側設定
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    channel_config_set_dreq(&config, spi_get_dreq(spi_default, true));  // 出力(送信)側のDMAトリガー取得
    channel_config_set_read_increment(&config,
                                      true);             // 読込位置をインクリメント
    channel_config_set_write_increment(&config, false);  // 書き込み位置は固定
    dma_channel_configure(dma_tx, &config,
                          &spi_get_hw(ctx->hw)->dr,  // write addr
                          tx,                        // read addr
                          size, false);

    // DMAC 読み出し側設定
    const uint32_t dma_rx = dma_claim_unused_channel(true);
    config = dma_channel_get_default_config(dma_rx);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    channel_config_set_dreq(&config, spi_get_dreq(ctx->hw, false));  // 入力(受信)側のDMAトリガー取得
    channel_config_set_read_increment(&config,
                                      false);  // 読込位置を固定
    channel_config_set_write_increment(&config,
                                       false);  // 受信データは捨てるため, 書き込み位置を固定

    dma_channel_configure(dma_rx, &config,
                          &null,                  // write addr
                          &spi_get_hw(spi0)->dr,  // read addr
                          size, false);

    SPIDrv_CS(ctx, 0);

    ctx->async.begin = get_absolute_time();
    dma_start_channel_mask((1u << dma_tx) | (1u << dma_rx));

    ctx->async.tx = dma_tx;
    ctx->async.rx = dma_rx;
  }

  return err;
}

UError_t SPIDrv_AsyncRecv(SPIDrvHandle_t handle, void* rx, size_t size) {
  UError_t err = uSuccess;
  static uint8_t null = 0u;

  if (uSuccess == err) {
    if (NULL == handle || NULL == rx || 0 == size) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    SPIDrvContext_t* const ctx = (SPIDrvContext_t*)handle;

    const uint32_t dma_tx = dma_claim_unused_channel(true);
    dma_channel_config config = dma_channel_get_default_config(dma_tx);

    // DMAC 書き込み側設定
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    channel_config_set_dreq(&config, spi_get_dreq(spi_default, true));  // 出力(送信)側のDMAトリガー取得
    channel_config_set_read_increment(&config,
                                      false);            // 読込位置を固定
    channel_config_set_write_increment(&config, false);  // 出力を変更しないため, 書き込み位置は固定
    dma_channel_configure(dma_tx, &config,
                          &spi_get_hw(ctx->hw)->dr,  // write addr
                          &null,                     // read addr
                          size, false);

    // DMAC 読み出し側設定
    const uint32_t dma_rx = dma_claim_unused_channel(true);
    config = dma_channel_get_default_config(dma_rx);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    channel_config_set_dreq(&config, spi_get_dreq(ctx->hw, false));  // 入力(受信)側のDMAトリガー取得
    channel_config_set_read_increment(&config,
                                      false);  // 読込位置を固定
    channel_config_set_write_increment(&config,
                                       true);  // 書き込み位置はインクリメント

    dma_channel_configure(dma_rx, &config,
                          rx,                     // write addr
                          &spi_get_hw(spi0)->dr,  // read addr
                          size, false);

    SPIDrv_CS(ctx, 0);

    ctx->async.begin = get_absolute_time();
    dma_start_channel_mask((1u << dma_tx) | (1u << dma_rx));

    ctx->async.tx = dma_tx;
    ctx->async.rx = dma_rx;
  }

  return err;
}

UError_t SPIDrv_AsyncTransfer(SPIDrvHandle_t handle, const void* tx, void* rx, size_t size) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle || NULL == tx || NULL == rx || 0 == size) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    SPIDrvContext_t* const ctx = (SPIDrvContext_t*)handle;

    const uint32_t dma_tx = dma_claim_unused_channel(true);
    dma_channel_config config = dma_channel_get_default_config(dma_tx);

    // DMAC 書き込み側設定
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    channel_config_set_dreq(&config, spi_get_dreq(spi_default, true));  // 出力(送信)側のDMAトリガー取得
    channel_config_set_read_increment(&config,
                                      true);             // 読込位置をインクリメント
    channel_config_set_write_increment(&config, false);  // 書き込み位置は固定
    dma_channel_configure(dma_tx, &config,
                          &spi_get_hw(ctx->hw)->dr,  // write addr
                          tx,                        // read addr
                          size, false);

    // DMAC 読み出し側設定
    const uint32_t dma_rx = dma_claim_unused_channel(true);
    config = dma_channel_get_default_config(dma_rx);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    channel_config_set_dreq(&config, spi_get_dreq(ctx->hw, false));  // 入力(受信)側のDMAトリガー取得
    channel_config_set_read_increment(&config,
                                      false);  // 読込位置を固定
    channel_config_set_write_increment(&config,
                                       true);  // 書き込み位置はインクリメント

    dma_channel_configure(dma_rx, &config,
                          rx,                     // write addr
                          &spi_get_hw(spi0)->dr,  // read addr
                          size, false);

    SPIDrv_CS(ctx, 0);

    ctx->async.begin = get_absolute_time();
    dma_start_channel_mask((1u << dma_tx) | (1u << dma_rx));

    ctx->async.tx = dma_tx;
    ctx->async.rx = dma_rx;
  }

  return err;
}

UError_t SPIDrv_WaitForAsync(SPIDrvHandle_t handle) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    SPIDrvContext_t* const ctx = (SPIDrvContext_t*)handle;

    dma_channel_wait_for_finish_blocking(ctx->async.rx);
    if (dma_channel_is_busy(ctx->async.tx)) {
      panic("RX complete before TX");
    }

    SPIDrv_CS(ctx, 1);
    dma_channel_unclaim(ctx->async.rx);
    dma_channel_unclaim(ctx->async.tx);
  }

  return err;
}

UError_t SPIDrv_TransferNBytes(SPIDrvHandle_t handle, const void* tx, void* rx, size_t size) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle || NULL == tx || NULL == rx || 0 == size) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    err = SPIDrv_AsyncTransfer(handle, tx, rx, size);
  }

  if (uSuccess == err) {
    err = SPIDrv_WaitForAsync(handle);
  }

  return err;
}
