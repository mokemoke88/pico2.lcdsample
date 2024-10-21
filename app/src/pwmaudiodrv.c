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

#define AUDIO_FIFO_DEPTH (25)                                           //< FIFO深さ 500ms 分のデータが格納できるように設定
#define AUDIODRV_FREQ (44100)                                           //< サンプル周波数
#define AUDIOBUF_20MS_SZ (((((AUDIODRV_FREQ / 100) * 2) + 1) / 2) * 2)  //< 20ms 分のサンプルを格納可能なサイズ

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
  AudioBufDesc_t* fifo[AUDIO_FIFO_DEPTH + 1];  //< FIFOバッファ
  volatile size_t rpos;                        //< 読み出し位置
  volatile size_t wpos;                        //< 書き出し位置
} AudioFIFO_t;

/**
 * @brief オーディオドライバ コンテキスト
 */
typedef struct tagAudioDrvContext_t {
  AudioFIFO_t poolBuf;                        //< 利用できるバッファディスクリプタを格納したFIFO
  AudioFIFO_t busyBuf;                        //< 使用中(出力待ち)バッファディスクリプタを格納したFIFO
  AudioBufDesc_t bufDescs[AUDIO_FIFO_DEPTH];  //< バッファディスクリプタ実体
  // 20ms * 25 = 500ms サンプル分のオーディオバッファ
  uint16_t audioBuf[AUDIOBUF_20MS_SZ * AUDIO_FIFO_DEPTH];  //< オーディオデータ実体
  AudioBufDesc_t* workDesc;                                //< ユーザからのデータ入力中のバッファディスクリプタを保持

  /**
   * DMA動作状態(true: 動作中, false: 停止中) @\n
   * DMA割り込みハンドラ内でバッファアンダーランが発生した場合 true -> false 遷移 @\n
   * DMA起動時に false -> true 遷移
   */
  volatile bool busy;

  void (*isr)(void);           //< DMA割り込みハンドラ
  AudioBufDesc_t* activeDesc;  //< 出力中のバッファディスクリプタを保持

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
static bool AudioFIFO_write(AudioFIFO_t* ctx, AudioBufDesc_t* data);

/**
 * @brief FIFOからのデータ取り出し処理
 * @param [in] ctx : 操作対象のFIFOオブジェクト
 * @param [out] pData : 取り出したデータの出力先
 * @return 処理結果
 * @retval true : データ取り出し成功
 * @retval true 以外 : データ取り出しに失敗. パラメータ異常 or バッファエンプティ
 */
static bool AudioFIFO_read(AudioFIFO_t* ctx, AudioBufDesc_t** pData);

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
     .workDesc = NULL,
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

static bool AudioFIFO_write(AudioFIFO_t* ctx, AudioBufDesc_t* data) {
  if (NULL == ctx || NULL == data) {
    return false;
  }

  size_t next = ctx->wpos + 1;
  if ((AUDIO_FIFO_DEPTH + 1) <= next) {
    next = 0;
  }

  if (next == ctx->rpos) {
    printf("[ERROR] next[%d] ctx->rpos[%d]", next, ctx->rpos);
    return false;  // バッファフル
  }

  ctx->fifo[ctx->wpos] = data;

  // メモリバリア
  asm("dsb");

  ctx->wpos = next;  // 書き込み位置移動
  return true;
}

static bool AudioFIFO_read(AudioFIFO_t* ctx, AudioBufDesc_t** pData) {
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
    for (size_t i = 0; i < AUDIO_FIFO_DEPTH; ++i) {
      // メモリ確保
      ctx->bufDescs[i].buf = &ctx->audioBuf[AUDIOBUF_20MS_SZ * i];
      ctx->bufDescs[i].size = AUDIOBUF_20MS_SZ;
      ctx->bufDescs[i].length = 0u;
      if (!AudioFIFO_write(&ctx->poolBuf, &ctx->bufDescs[i])) {
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
    AudioBufDesc_t* desc = NULL;

    if (AudioFIFO_read(&ctx->busyBuf, &desc)) {
      // 出力待ちデータを取得し, DMA転送を開始する
      // printf("[INFO ] buf[%08lx] length[%ld]\n", (uint32_t)desc->buf, (uint32_t)desc->length);
      dma_channel_set_read_addr(ctx->dma_audio, desc->buf, false);
      dma_channel_set_trans_count(ctx->dma_audio, desc->length, true);
    } else {
      // 出力待ちデータがない場合は, ビジーフラグを寝かし, PWMを停止する
      printf("[WARN ]\r\n");
      ctx->busy = false;
      pwm_set_enabled(ctx->pwm_slice, false);
    }

    // 出力中のデータが存在する場合は, 利用可能バッファに戻す.
    if (NULL != ctx->activeDesc) {
      if (!AudioFIFO_write(&ctx->poolBuf, ctx->activeDesc)) {
        printf("[ERROR] \r\n");
      }
    }

    ctx->activeDesc = desc;

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

  if (uSuccess == err) {
    // 5 * 20ms = 100ms 分の空データを投入
    for (size_t i = 0; i < 5; ++i) {
      AudioBufDesc_t* cur = NULL;
      if (!AudioFIFO_read(&ctx->poolBuf, &cur)) {
        err = uFailure;
        break;
      }
      memset(cur->buf, 0x0u, sizeof(uint16_t) * (441 * 2));
      cur->length = 441 * 2;
      if (!AudioFIFO_write(&ctx->busyBuf, cur)) {
        err = uFailure;
        break;
      }
    }
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
    if(false == ctx->busy){
      ctx->busy = true;
      ctx->isr();
    }
  }

  if(uSuccess == err){
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

int32_t AudioDrv_WriteSample(AudioDrvHandle_t handle, const void* src, size_t size) {
  UError_t err = uSuccess;
  AudioDrvContext_t* ctx = (AudioDrvContext_t*)handle;
  int32_t pos = 0;

  if (uSuccess == err) {
    if (NULL == ctx || ctx != builtinInUse[0] || NULL == src || 0 >= size) {
      printf("[ERROR] %s(): invalid param\r\n", __FUNCTION__);
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    // size バイトループ
    const uint8_t* s = (const uint8_t*)src;
    for (; pos < size; ++pos) {
      if (NULL == ctx->workDesc) {
        // ワークバッファがなければ(前回の処理で残っているなら)
        // 利用可能なバッファを取得
        if (!AudioFIFO_read(&ctx->poolBuf, &ctx->workDesc)) {
          // 利用可能なバッファがない = 書き込み終了
          break;
        }
        ctx->workDesc->length = 0;  // サンプル数初期化
      }
      // 書き込み可能(書き込みサンプル数がバッファサイズより小さい)であれば,
      // 書き込み
      if (ctx->workDesc->length < ctx->workDesc->size) {
        ctx->workDesc->buf[ctx->workDesc->length++] = s[pos];
      } else {
        // おそらく起こりえない. アルゴリズムエラー
        printf("[ERROR] ?? length[%ld] >= size[%ld]\n", ctx->workDesc->length, ctx->workDesc->size);
        err = uFailure;
        break;
      }

      if (ctx->workDesc->size <= ctx->workDesc->length) {
        // 結果, バッファを詰め切ったのであれば, 出力待ちFIFOに追加
        if (!AudioFIFO_write(&ctx->busyBuf, ctx->workDesc)) {
          printf("[ERROR] AudioFIFO_write() failure\r\n");
          err = uFailure;
          break;
        }
        ctx->workDesc = NULL;
      }
    }
  }

  if (uSuccess != err) {
    pos = -1;
  }
  return pos;
}