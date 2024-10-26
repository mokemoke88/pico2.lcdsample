/**
 * @file src/main.c
 *
 * 制作した部品の動作確認サンプル
 *
 * @date 2024.09.23 kshibata@seekers.jp
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <pico/binary_info.h>
#include <pico/stdlib.h>

#include <user/types.h>

// デバイス操作
#include <user/cst328drv.h>    // タッチパッド
#include <user/lcddrv.h>       // LCD
#include <user/pwmaudiodrv.h>  // PWM Audio
#include <user/spidrv.h>       // SPI

// 部品
#include <user/canvas.h>      // canvas
#include <user/fontx2.h>      // fontx2
#include <user/textbox.h>     // textbox
#include <user/utf8string.h>  // 文字処理

#include <user/gui.h>  // GUI 部品

#include <app/app.h>
#include <app/diag.h>

#include <stdio.h>
#include <string.h>

#include <user/log.h>
#include <user/macros.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

#define FRAMEBUF_SZ (240 * 320)  //< フレームバッファの縦横サイズ
#define FRAMEBUF_NUM (2)         //< フレームバッファの面数

#define CONSOLEHEAP_SZ (50 * 256)       // コンソールヒープのバイト数
#define FRAME_CONSOLEHEAP_SZ (4 * 256)  // DIAGコンソールヒープのバイト数

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

static uint16_t gFramebuf[FRAMEBUF_SZ * FRAMEBUF_NUM] = {0};   //< フレームバッファメモリ
static uint8_t gConsoleHeap[CONSOLEHEAP_SZ] = {0};             //< コンソール用データ領域
static uint8_t gFrameConsoleHeap[FRAME_CONSOLEHEAP_SZ] = {0};  //< フレーム情報表示用
static uint8_t audio_data[441 * 6 * 2] = {0};                  //< 60ミリ分のオーディオデータ

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

#define ToneDelta(X) (((8192 * 65536 * X) / 44100) >> 16)

/**
 * エントリポイント
 */
int main(void) {
  stdio_init_all();

  printf("PICO2 LCD Controller \r\n");

  // タッチパッドデバイス準備
  CST328DrvHandle_t hTouch = NULL;
  CST328Drv_Open(&hTouch);

  // SPIデバイス準備
  SPIDrvHandle_t hSpi = NULL;
  SPIDrv_Open(&hSpi);

  // LCDデバイス準備
  LCDDrvHandle_t hLcd = NULL;
  LCDDrv_Open(&hLcd, hSpi);

  // Canvas生成
  Canvas_t canvas[2] = {0};
  Canvas_Create(&canvas[0], 240, 320, 240, &gFramebuf[FRAMEBUF_SZ * 0]);
  Canvas_Create(&canvas[1], 240, 320, 240, &gFramebuf[FRAMEBUF_SZ * 1]);

  // コンソール表現に使用するTextbox インスタンス生成
  TextboxHandle_t hTextbox = NULL;
  Textbox_Create(&hTextbox, gConsoleHeap, CONSOLEHEAP_SZ, 64, 17);

  TextboxHandle_t hFrameTextbox = NULL;
  Textbox_Create(&hFrameTextbox, gFrameConsoleHeap, FRAME_CONSOLEHEAP_SZ, 64, 2);

  // SPI初期化
  SPIDrv_Init(hSpi, 25 * 1000 * 1000);

  // LCD初期化
  LCDDrv_Init(hLcd);
  // LCD黒でクリア
  LCDDrv_Clear(hLcd, 0u, 0u, 0u);

  // タッチパッド初期化
  CST328Drv_Init(hTouch);

  // LCD輝度調整
  LCDDrv_SetBrightness(hLcd, 0xff);

  // サンプルデータを初期化(全部127に)
  memset(audio_data, 1, sizeof(audio_data) / sizeof(audio_data[0]));

  // オーディオデバイス初期化
  AudioDrvHandle_t hAudio = NULL;
  if (uSuccess != AudioDrv_Open(&hAudio)) {
    printf("[ERROR] AudioDrv_Open() failure\r\n");
    while (1);
  }

  // Appオブジェクトを初期化
  AppObject_t app = AppInit();

  // Diagオブジェクトを初期化
  DiagObject_t diag = DiagInit();

  LOG_D("Enter EventLoop");

  uint32_t frame_counter = 0u;  //< フレームカウンタ

  // フレーム更新間隔計測用
  absolute_time_t frame_starttime = get_absolute_time();                                 //< フレーム更新間隔計測用
  absolute_time_t frame_time = absolute_time_diff_us(frame_starttime, frame_starttime);  //< フレーム更新間隔計測用

  // フレーム処理時間計測用
  absolute_time_t proc_time = 0;

  while (true) {
    absolute_time_t proc_starttime = get_absolute_time();             // フレーム所要時間計時
    Canvas_t* frame = (frame_counter % 2) ? &canvas[0] : &canvas[1];  // 使用するCanvas(フレームバッファ)の選択

    const size_t audio_size = 2940;       // オーディオサンプル数
    memset(audio_data, 0x0, audio_size);  // オーディオデータをフラットに

    CST328Drv_UpdateCoord(hTouch);  // タッチパッド情報(入力)を更新
    // TODO: 情報の抽象化: タッチパッド情報からアプリケーション入力情報への変換処理
    CST328Data_t touch_data;
    CST328Drv_GetCoord(hTouch, &touch_data);

    // アプリケーション処理を移す
    // note: オーディオは, フレーム分のオーディオバッファを渡し,
    //  アプリケーション側で内容を更新, その内容を出力する
    {
      AppArg_t appArg = {
          .frame = frame,
          .frameCount = frame_counter,
          .hLCD = hLcd,
          .touch = &touch_data,
          .audio_buff = audio_data,
          .audio_size = audio_size,
      };
      AppUpdate(&app, &appArg);
    }

    // タッチ情報を描画
    {
      if (0 < touch_data.points) {
        char sbuf[128] = {0};
        for (size_t i = 0; i < touch_data.points; ++i) {
          sprintf(sbuf, "%d:I[%1d]X[%3d]Y[%3d]S[%3d]P[%1x]", i, touch_data.coords[i].id, touch_data.coords[i].x, touch_data.coords[i].y,
                  touch_data.coords[i].strength, touch_data.coords[i].status);
          Textbox_Push(hTextbox, sbuf, strlen(sbuf));
        }
      }
      {
        GUITextbox_t guiText = {0};
        if (uSuccess == GUITextbox_Create(&guiText, hTextbox)) {
          GUITextboxRender(&guiText, frame, 10, 10 + 32, RGB888toRGB565(0xf, 0xf, 0xf));
        }
      }
    }

    // diag 表示(画面上端タッチで表示, 非表示切替)
    {
      DiagArg_t arg = {.frame = frame, .text = hFrameTextbox, .frameTime = frame_time, .procTime = proc_time, .touch = &touch_data};
      DiagUpdate(&diag, &arg);
    }

    // オーディオドライバでオーディオデータ供給
    // 1/15 = 0.06666....
    // 44100/15 = 2940 となるため, フレーム更新間隔の調整は
    // オーディオデータの出力間隔基準で処理した方が単純
    // !! 結果として1フレーム分 (1/15) 秒 の時間調整が行われるため !!
    // !! 出力するオーディオデータがなくとも空データを出力している  !!
    absolute_time_t audio_starttime = get_absolute_time();
    {
      // 今フレーム分のオーディオデータを出力
      size_t remain = audio_size;
      int32_t written = 0;
      size_t offset = 0;
      while (0 < remain) {
        written = AudioDrv_WriteSample(hAudio, audio_data + offset, remain);
        if (0 > written) {
          // TODO: エラー処理
          break;
        }
        remain -= written;
        offset += written;
      }
    }
    absolute_time_t audio_endtime = get_absolute_time();

    // 計時更新
    absolute_time_t endtime = get_absolute_time();                      // フレーム処理終了時刻
    frame_time = absolute_time_diff_us(frame_starttime, endtime);       // フレーム周期(描画更新間隔)
    proc_time = absolute_time_diff_us(audio_starttime, audio_endtime);  // フレーム内処理時間
    frame_starttime = endtime;
    frame_counter++;

    LCDDrv_SwapBuff(hLcd, Canvas_GetBuf(frame), 0, 0, 240, 320);
  }
  return 0;
}
