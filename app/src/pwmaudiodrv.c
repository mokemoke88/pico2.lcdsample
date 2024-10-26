/**
 * @file prog01/app/src/pwmaudiodrv.c
 *
 * PWM + DMA を利用したオーディオ再生処理
 *
 * @date 2024.10.21 k.shibata newly created
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/pwmaudiodrv.h>

#include <pico/binary_info.h>
#include <pico/stdlib.h>

#include <hardware/dma.h>
#include <hardware/pwm.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <user/types.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

#define AUDIODRV_FREQ (44100)                                         //< サンプル周波数
#define AUDIOBUF_CHANK_SZ ((((AUDIODRV_FREQ / 30 / 2) + 3) / 4) * 4)  //< 1/60秒 分のサンプルを格納可能なサイズ, DMAC割り込みの周期に影響する
#define AUDIO_FIFO_DEPTH (25)                                         //< FIFO深さ 25段 = 0.41666..秒分のデータが格納できるように設定

#define AUDIODRV0_PWM (16)  //< AUDIODRV0 で使用する GPIOピン

// for pico tool
bi_decl(bi_1pin_with_name(AUDIODRV0_PWM, "AUDODRV0 PWM"));

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief オーディオデータ用 バッファディスクリプタ
 */
typedef struct AudioBufDesc_t {
  uint16_t* buf;  //< データ本体. PWM WRAPレジスタに書き込む値
  size_t size;    //< 最大サイズ
  size_t length;  //< データサイズ
} AudioBufDesc_t;

/**
 * @brief オーディオデータ用 ロックフリーFIFO
 * ブロック配列は所望のFIFO深さ + 1が必要
 */
typedef struct tagAudioFIFO_t {
  volatile AudioBufDesc_t* fifo[AUDIO_FIFO_DEPTH + 1];  //< FIFOバッファ
  volatile size_t rpos;                                 //< 読み出し位置
  volatile size_t wpos;                                 //< 書き出し位置
} AudioFIFO_t;

/**
 * @brief オーディオドライバ コンテキスト
 */
typedef struct tagAudioDrvContext_t {
  AudioFIFO_t poolBuf;                                      //< 利用できるバッファディスクリプタを格納したFIFO
  AudioFIFO_t busyBuf;                                      //< 使用中(出力待ち)バッファディスクリプタを格納したFIFO
  AudioBufDesc_t bufDescs[AUDIO_FIFO_DEPTH];                //< バッファディスクリプタ実体
  uint16_t audioBuf[AUDIOBUF_CHANK_SZ * AUDIO_FIFO_DEPTH];  //< オーディオデータ実体

  /**
   * DMA動作状態(true: 動作中, false: 停止中) @\n
   * DMA割り込みハンドラ内でバッファアンダーランが発生した場合 true -> false 遷移 @\n
   * DMA起動時に false -> true 遷移
   */
  volatile bool busy;

  void (*isr)(void);                    //< DMA割り込みハンドラ
  volatile AudioBufDesc_t* activeDesc;  //< 出力中のバッファディスクリプタを保持

  // pico デバイス関係
  uint32_t dma_audio;             //< DMAチャンネル
  dma_channel_config dma_config;  //< DMA設定
  uint pwm_slice;                 //< PWMスライス
  uint pwm_ch;                    //< PWMチャンネル

} AudioDrvContext_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief FIFOへのデータ投入処理
 * @param [in] ctx : 操作対象のFIFOオブジェクト
 * @param [in] data : 投入するオーディオバッファ情報
 * @return 処理結果
 * @retval true : データの投入成功
 * @retval true 以外 : データの投入に失敗. パラメータ異常 or バッファフル
 */
static bool AudioFIFO_push(AudioFIFO_t* ctx, volatile AudioBufDesc_t* data);

/**
 * @brief FIFOからのデータ取り出し処理
 * @param [in] ctx : 操作対象のFIFOオブジェクト
 * @param [out] pData : 取り出したデータの出力先
 * @return 処理結果
 * @retval true : データ取り出し成功
 * @retval true 以外 : データ取り出しに失敗. パラメータ異常 or バッファエンプティ
 */
static bool AudioFIFO_pop(AudioFIFO_t* ctx, volatile AudioBufDesc_t** pData);

/**
 * @brief AudioContext のFIFO情報の初期化を行う
 * バッファディスクリプタの初期化.
 * 利用可能バッファへの投入
 * @param [in] ctx : 処理対象
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
static UError_t AudioDrv_initPool(AudioDrvContext_t* ctx);

/**
 * @brief DMA割り込みハンドラ AudioDrv0用
 * DMA転送完了を受け
 * 出力待ちディスクリプタを取得し, 出力中のディスクリプタとしてマーク.
 * 次のDMA転送を開始する.
 * 出力待ちディスクリプタがない場合は, DMA転送を停止する(busy = false)
 *
 * 出力中のディスクリプタを利用可能FIFOに投入する.
 */
static void AudioDrv0_DMAHandler(void);

/**
 * @brief PWMデバイスを利用できるよう初期化を行う
 * PWMスライス, チャンネルの割り当て
 * 分周設定 44100 * 256 Hz
 * Wrap設定 255
 * = DUTY分解能 11.289_600 MHz
 *
 * @param [in] ctx : 処理対象
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
static UError_t AudioDrv_initPWM(AudioDrvContext_t* ctx);

/**
 * @brief DMACデバイスを利用できるよう初期化を行う
 * 前提: PWMデバイスの初期化が完了していること(AudioDrv_initPWM())
 * DMAチャンネルの取得, 割り当て
 * 転送データサイズの設定 16bit (8bitはNGだった)
 * リクエスト信号(PWM_WRAP0) の登録
 * DMA設定(書き込みアドレス固定, 読み出しアドレスインクリメント)
 * 書き込み先を PWM CC レジスタに設定(32bitレジスタの下位16bitを変更)
 * 割り込みハンドラ登録
 * DMAの割り込みを有効化
 * 割り込みコントローラのDMA_IRQ_0割り込みを許可
 * DMA転送開始後, 即停止になることを避けるため,
 * 100ms分の無音データを利用可能FIFOから取り出し, 出力待ちFIFOに投入
 *
 * @param [in] ctx : 処理対象
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
static UError_t AudioDrv_initDMA(AudioDrvContext_t* ctx);

/**
 * @brief AudioDrvContext を利用可能な状態にする(AudioDrv0向け)
 *
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
static UError_t AudioDrv0_setup(void);

/**
 * @brief オーディオ出力を開始する
 * @param [in] ctx : 処理対象
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
static UError_t AudioDrv_start(AudioDrvContext_t* ctx);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief 組込みコンテキスト
 */
static AudioDrvContext_t builtinContext[] = {
    {.poolBuf = {.fifo = {0}, .rpos = 0u, .wpos = 0u},
     .busyBuf = {.fifo = {0}, .rpos = 0u, .wpos = 0u},
     .bufDescs = {0},
     .audioBuf = {0},
     .isr = &AudioDrv0_DMAHandler,
     .busy = false,
     .activeDesc = NULL,
     .dma_audio = 0u,
     .dma_config = {0},
     .pwm_slice = 0u,
     .pwm_ch = 0u},
};

static AudioDrvContext_t* builtinInUse[] = {NULL};

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

static bool AudioFIFO_push(AudioFIFO_t* ctx, volatile AudioBufDesc_t* data) {
  if (NULL == ctx || NULL == data) {
    return false;
  }

  size_t next = ctx->wpos + 1;
  if ((AUDIO_FIFO_DEPTH + 1) <= next) {
    next = 0;
  }

  if (next == ctx->rpos) {
    // printf("[ERROR] next[%d] ctx->rpos[%d]", next, ctx->rpos);
    return false;  // バッファフル
  }

  ctx->fifo[ctx->wpos] = data;

  // メモリバリア
  asm("dsb");

  ctx->wpos = next;  // 書き込み位置移動
  return true;
}

static bool AudioFIFO_pop(AudioFIFO_t* ctx, volatile AudioBufDesc_t** pData) {
  if (NULL == ctx || NULL == pData) {
    return false;
  }

  if (ctx->rpos == ctx->wpos) {
    return false;  // バッファエンプティ
  }

  // メモリバリア
  asm("dsb");

  *pData = ctx->fifo[ctx->rpos];

  size_t next = ctx->rpos + 1;
  if ((AUDIO_FIFO_DEPTH + 1) <= next) {
    next = 0;
  }

  ctx->rpos = next;
  return true;
}

static UError_t AudioDrv_initPool(AudioDrvContext_t* ctx) {
  UError_t err = uSuccess;
  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    // バッファプールに情報を登録する
    for (size_t i = 0; i < AUDIO_FIFO_DEPTH; ++i) {
      // バッファディスクリプタ初期化, バッファプールに登録
      ctx->bufDescs[i].buf = &ctx->audioBuf[AUDIOBUF_CHANK_SZ * i];
      ctx->bufDescs[i].size = AUDIOBUF_CHANK_SZ;
      ctx->bufDescs[i].length = 0u;
      if (!AudioFIFO_push(&ctx->poolBuf, &ctx->bufDescs[i])) {
        printf("[ERROR] AudioFIFO_write() [%d]\r\n", i);
        err = uFailure;
        break;
      }
    }
  }
  return err;
}

static void AudioDrv0_DMAHandler(void) {
  AudioDrvContext_t* const ctx = &builtinContext[0];
  if (NULL != ctx) {
    volatile AudioBufDesc_t* desc = NULL;

    // 出力中のデータが存在する場合は, 利用可能バッファに戻す.
    if (NULL != ctx->activeDesc) {
      if (!AudioFIFO_push(&ctx->poolBuf, ctx->activeDesc)) {
        printf("[ERROR] \r\n");
      }
      ctx->activeDesc = NULL;
    }

    if (AudioFIFO_pop(&ctx->busyBuf, &desc)) {
      // 出力待ちデータを取得し, DMA転送を開始する
      // printf("[INFO ] buf[%08lx] length[%ld]\n", (uint32_t)desc->buf, (uint32_t)desc->length);
      dma_channel_set_read_addr(ctx->dma_audio, desc->buf, false);
      dma_channel_set_trans_count(ctx->dma_audio, desc->length, true);
      ctx->activeDesc = desc;
    } else {
      // 出力待ちデータがない場合(under flow)は, PWMを停止し, busyフラグを寝かす
      pwm_set_enabled(ctx->pwm_slice, false);
      ctx->busy = false;
      printf("[W]\r\n");
    }

    // 割り込みクリア
    dma_hw->ints0 = (1u << ctx->dma_audio);
  }
}

static UError_t AudioDrv_initPWM(AudioDrvContext_t* ctx) {
  UError_t err = uSuccess;
  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    gpio_set_function(AUDIODRV0_PWM, GPIO_FUNC_PWM);
    ctx->pwm_slice = pwm_gpio_to_slice_num(AUDIODRV0_PWM);
    ctx->pwm_ch = pwm_gpio_to_channel(AUDIODRV0_PWM);

    pwm_config config = pwm_get_default_config();
    pwm_init(ctx->pwm_slice, &config, false);
    pwm_set_clkdiv_int_frac(ctx->pwm_slice, 13, 4);
    pwm_set_wrap(ctx->pwm_slice, 255);
    pwm_set_chan_level(ctx->pwm_slice, ctx->pwm_ch, 0u);
  }

  return err;
}

static UError_t AudioDrv_initDMA(AudioDrvContext_t* ctx) {
  UError_t err = uSuccess;
  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    ctx->dma_audio = dma_claim_unused_channel(true);
    ctx->dma_config = dma_channel_get_default_config(ctx->dma_audio);

    channel_config_set_transfer_data_size(&ctx->dma_config, DMA_SIZE_16);
    channel_config_set_dreq(&ctx->dma_config, DREQ_PWM_WRAP0 + ctx->pwm_slice);
    channel_config_set_write_increment(&ctx->dma_config, false);
    channel_config_set_read_increment(&ctx->dma_config, true);
    dma_channel_configure(ctx->dma_audio, &ctx->dma_config, &pwm_hw->slice[ctx->pwm_slice].cc, NULL, 0, false);

    irq_set_exclusive_handler(DMA_IRQ_0, ctx->isr);
    dma_channel_set_irq0_enabled(ctx->dma_audio, true);
    irq_set_enabled(DMA_IRQ_0, true);
  }

  return err;
}

static UError_t AudioDrv0_setup(void) {
  UError_t err = uSuccess;
  AudioDrvContext_t* ctx = &builtinContext[0];

  if (uSuccess == err) {
    err = AudioDrv_initPool(ctx);
    if (uSuccess != err) {
      printf("[ERROR] AudioDrv_initPool(): failure\r\n");
    }
  }

  if (uSuccess == err) {
    err = AudioDrv_initPWM(ctx);
    if (uSuccess != err) {
      printf("[ERROR] AudioDrv_initPWM(): failure\r\n");
    }
  }

  if (uSuccess == err) {
    err = AudioDrv_initDMA(ctx);
    if (uSuccess != err) {
      printf("[ERROR] AudioDrv_initDMA(): failure\r\n");
    }
  }

  return err;
}

static UError_t AudioDrv_start(AudioDrvContext_t* ctx) {
  UError_t err = uSuccess;
  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    // TODO: DMA割り込み禁止
    if (false == ctx->busy) {
      ctx->busy = true;
      ctx->isr();
    }
    // TODO: DMA割り込み解除
  }

  if (uSuccess == err) {
    pwm_set_enabled(ctx->pwm_slice, true);
  }

  return err;
}

UError_t AudioDrv_Open(AudioDrvHandle_t* pHandle) {
  UError_t err = uSuccess;
  AudioDrvContext_t* ctx = &builtinContext[0];

  if (uSuccess == err) {
    if (NULL == ctx || NULL == pHandle) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (NULL != builtinInUse[0]) {
      // already use
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    err = AudioDrv0_setup();
  }

  if (uSuccess == err) {
    builtinInUse[0] = ctx;
    *pHandle = builtinInUse[0];
  }

  return err;
}

UError_t AudioDrv_Start(AudioDrvHandle_t handle) {
  UError_t err = uSuccess;
  AudioDrvContext_t* ctx = (AudioDrvContext_t*)handle;

  if (uSuccess == err) {
    if (NULL == ctx || ctx != builtinInUse[0]) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    err = AudioDrv_start(ctx);
  }

  return err;
}

int32_t AudioDrv_WriteSample(AudioDrvHandle_t handle, const void* const src, size_t const size) {
  UError_t err = uSuccess;
  AudioDrvContext_t* ctx = (AudioDrvContext_t*)handle;
  const uint8_t* ptr = (const uint8_t*)src;
  int32_t ret = 0;

  if (uSuccess == err) {
    if (NULL == ctx || ctx != builtinInUse[0] || NULL == ptr || 0 >= size) {
      printf("[ERROR] %s(): invalid param\r\n", __FUNCTION__);
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    // 指定したバイト数(サンプル数)のデータをオーディオバッファに書き込む
    volatile AudioBufDesc_t* buf = NULL;  // オーディオバッファディスクリプタ
    size_t remain = size;

    while (0 < remain) {
      // バッファプールから書き込み先オーディオバッファを取得
      if (AudioFIFO_pop(&ctx->poolBuf, &buf)) {
        // 指定バイト数か, バッファが埋まるまで書き込む
        const size_t nbytewrite = (buf->size < remain) ? buf->size : remain;
        for (size_t i = 0; i < nbytewrite; ++i) {
          buf->buf[i] = ptr[ret + i];  // buf側はuint16_t なためmemcpyはNG
        }
        buf->length = nbytewrite;
        remain -= nbytewrite;
        ret += nbytewrite;
        // 出力待機FIFOに投入
        if (AudioFIFO_push(&ctx->busyBuf, buf)) {
          if (!ctx->busy) {
            // PWMが停止状態であれば, 動作を開始する
            (void)AudioDrv_start(ctx);
            printf("[I] AudioDrv_start called\r\n");
          }
        }
      } else {
        break;
      }  // if(AudioFIFO_read ...
    }
  }

  if (uSuccess != err) {
    ret = -1;
  }
  return ret;
}
