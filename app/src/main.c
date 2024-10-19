/**
 * @file src/main.c
 * @date 2024.09.23 kshibata@seekers.jp
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <pico/binary_info.h>
#include <pico/stdlib.h>

#include <user/types.h>

// デバイス操作
#include <user/lcddrv.h>    // LCD
#include <user/spidrv.h>    // SPI
#include <user/cst328drv.h>  // タッチパッド

// 部品
#include <user/canvas.h>
#include <user/fontx2.h>
#include <user/textbox.h>
#include <user/utf8string.h>  // 文字処理

#include <stdio.h>
#include <string.h>

#include <user/log.h>
#include <user/macros.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

#define FRAMEBUF_SZ (240 * 320)
#define FRAMEBUF_NUM (2)

#define CONSOLEHEAP_SZ (50 * 256)
#define FRAME_CONSOLEHEAP_SZ (4 * 256)

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

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

/**
 * canvas の指定位置に字形を出力します
 * @param [in] canvas : 出力対象
 * @param [in] posx : 出力先x座標値
 * @param posy
 * @param color
 * @param fgraph
 * @param fw
 * @param fh
 * @param fsz
 * @return
 */
UError_t renderFont(Canvas_t const* canvas, const size_t posx, const size_t posy, const uint16_t color, const uint8_t* fgraph, const size_t fw, const size_t fh,
                    const size_t fsz) {
  UError_t err = uSuccess;
  if (uSuccess == err) {
    if (NULL == canvas || NULL == fgraph || 0 == fsz) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    LOG_I("w[%d] h[%d] fsz[%d]", fw, fh, fsz);
    const size_t fbw = (fw + (8u - 1u)) / 8u;  // フォントのバイト幅
    const uint8_t* addr = fgraph;
    for (uint32_t fy = 0; fy < fh; ++fy) {
      for (uint32_t fx = 0; fx < fw; ++fx) {
        const size_t bpos = (fx >> 3);       //(fx / 8);
        const size_t bshift = (fx & 0b111);  //(fx % 8);
        const uint8_t c = *(addr + bpos) << bshift;
        if (c & 0b10000000) {
          Canvas_DrawPixel(canvas, posx + fx, posy + fy, color);
        }
      }
      addr += fbw;
    }
  }
}

/**
 * Textbox を描画
 * @param canvas : 描画先
 * @param textbox : 出力するTextboxインスタンス
 * @param posx : 出力座標 x
 * @param posy : 出力座標 y
 * @param color : 色
 * @return
 */
UError_t renderTextbox(Canvas_t const* canvas, TextboxHandle_t textbox, const uint32_t posx, const uint32_t posy, const uint16_t color) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == canvas || NULL == textbox) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    const size_t orgX = posx;
    const size_t orgY = posy;

    const size_t rows = Textbox_CountRow(textbox);
    size_t curY = orgY;
    for (size_t i = 0; i < rows; ++i) {
      size_t dlen = 0u;
      size_t curX = orgX;
      const char* data = (const char*)Textbox_GetRow(textbox, &dlen, i);
      if (NULL != data && 0 < dlen) {
        // CanvasにTextboxから取得したデータを描画する
        size_t offset = 0;
        size_t r = 0;
        uint32_t utf32 = 0u;
        while (dlen > offset) {
          if (uSuccess == UTF8String_ToUTF32(&utf32, &r, data + offset, dlen - offset)) {
            uint16_t sjis = UTF8String_UTF32toSJIS(utf32);
            uint32_t fw, fh;
            size_t fsz;
            const uint8_t* fgraph = FontX2_GetFont(sjis, &fw, &fh, &fsz);
            if (NULL != fgraph) {
              renderFont(canvas, curX, curY, color, fgraph, fw, fh, fsz);
            } else {
              // 字形無し
            }
            curX += fw;
            offset += r;
          } else {
            break;
          }
        }  // while(dlen > offset ...
      }
      curY += 16;
    }
  }
  return err;
}

typedef struct tagCirclePointer_t {
  uint16_t x;
  uint16_t y;
  uint16_t r;
  uint8_t red;
  uint8_t green;
  uint8_t blue;

  bool active;
  uint16_t frame;
} CirclePointer_t;

typedef struct tagCirclePointerEvent_t {
  bool enable;
  uint16_t x;
  uint16_t y;
} CirclePointerEvent_t;

void CirclePointer_Event(CirclePointer_t* self, CirclePointerEvent_t* event) {
  if (!self->active && event->enable) {
    self->x = event->x;
    self->y = event->y;
    self->frame = 0;
    self->r = 1;
    self->active = true;
  } else if (self->active) {
    self->frame++;
    if (16 > self->frame) {
      self->r += 2;
    } else {
      self->active = false;
    }
  }
}

void CirclePointer_Render(CirclePointer_t* self, const Canvas_t* canvas) {
  if (self->active) {
    Canvas_DrawCircle(canvas, self->x, self->y, self->r, RGB888toRGB565(self->red, self->green, self->blue));
  }
}

/**
 * @brief レンダリング処理
 * @param canvas
 * @param f
 * @return
 */
static UError_t Render(Canvas_t const* canvas, const uint32_t f, CirclePointer_t* circle) {
  UError_t err = uSuccess;
  if (NULL == canvas) {
    err = uFailure;
  }
  if (uSuccess == err) {
    // 罫線描画
    Canvas_DrawLine(canvas, 0, 0, 239, 319, RGB888toRGB565(0xff, 0, 0));
    Canvas_DrawLine(canvas, 239, 0, 0, 319, RGB888toRGB565(0xff, 0, 0));

    Canvas_DrawLine(canvas, 0, 0, 0, 319, RGB888toRGB565(0xff, 0, 0));
    Canvas_DrawLine(canvas, 100, 0, 100, 319, RGB888toRGB565(0xff, 0, 0));
    Canvas_DrawLine(canvas, 200, 0, 200, 319, RGB888toRGB565(0xff, 0, 0));

    Canvas_DrawLine(canvas, 0, 0, 239, 0, RGB888toRGB565(0xff, 0, 0));
    Canvas_DrawLine(canvas, 0, 100, 239, 100, RGB888toRGB565(0xff, 0, 0));
    Canvas_DrawLine(canvas, 0, 200, 239, 200, RGB888toRGB565(0xff, 0, 0));
    Canvas_DrawLine(canvas, 0, 300, 239, 300, RGB888toRGB565(0xff, 0, 0));

    // サークル描画
    Canvas_DrawCircle(canvas, 100, 100, (f % 30) + 1, RGB888toRGB565(0, 0xff, 0));

    // サークル描画2
    Canvas_DrawFillCircle(canvas, 200, 200, (f % 20) + 1, RGB888toRGB565(0x0f + (f % 20) * 10, 0x0f + (f % 20) * 10, 0xff));

    CirclePointer_Render(circle, canvas);
  }
  return err;
}

/**
 * エントリポイント
 */
int main(void) {
  stdio_init_all();

  printf("PICO2 LCD Controller \r\n");

  CST328DrvHandle_t hTouch = NULL;
  CST328Drv_Open(&hTouch);

  SPIDrvHandle_t hSpi = NULL;
  SPIDrv_Open(&hSpi);

  LCDDrvHandle_t hLcd = NULL;
  LCDDrv_Open(&hLcd, hSpi);

  // Canvas生成
  Canvas_t canvas[2] = {0};
  Canvas_Create(&canvas[0], 240, 320, 240, &gFramebuf[FRAMEBUF_SZ * 0]);
  Canvas_Create(&canvas[1], 240, 320, 240, &gFramebuf[FRAMEBUF_SZ * 1]);

  // コンソール表現に使用するTextbox インスタンス生成
  TextboxHandle_t hTextbox = NULL;
  Textbox_Create(&hTextbox, gConsoleHeap, CONSOLEHEAP_SZ, 64, 18);

  TextboxHandle_t hFrameTextbox = NULL;
  Textbox_Create(&hFrameTextbox, gFrameConsoleHeap, FRAME_CONSOLEHEAP_SZ, 64, 1);

  // SPI初期化
  SPIDrv_Init(hSpi, 25 * 1000 * 1000);

  // LCD初期化
  LCDDrv_Init(hLcd);
  // LCD黒でクリア
  LCDDrv_Clear(hLcd, 0u, 0u, 0u);

  // タッチパッド初期化
  CST328Drv_Init(hTouch);

  // LCD輝度調整
  LCDDrv_SetBrightness(hLcd, 0x7fff);

  // 円ポインタ
  CirclePointer_t circleP = {.active = false, .red = 0xff, .green = 0x00, .blue = 0x00};

  LOG_D("Enter EventLoop");

  uint32_t f = 0u;  //< フレームカウンタ

  absolute_time_t btime = get_absolute_time();  //< フレーム更新間隔計測用
  absolute_time_t etime = get_absolute_time();  //< フレーム更新間隔計測用
  absolute_time_t difftime = 0;                 //< フレーム更新間隔計測用

  while (true) {
    // absolute_time_t b = get_absolute_time();
    // LCDDrv_SetBrightness(hLcd, bright++);
    // absolute_time_t e = get_absolute_time();
    // printf("LCDDrv_Clear():%lld %lld %lld\n", b, e, absolute_time_diff_us(b,
    // e));

    Canvas_t* frame = (f % 2) ? &canvas[0] : &canvas[1];  // 使用するCanvas(フレームバッファ)の選択

    CST328Drv_UpdateCoord(hTouch);  // タッチパッド情報(入力)を更新

    // CirclePointer 処理
    {
      CST328Data_t data;
      CST328Drv_GetCoord(hTouch, &data);

      CirclePointerEvent_t ev = {
          .enable = (data.points > 0),
          .x = data.coords[0].x,
          .y = data.coords[0].y,
      };
      CirclePointer_Event(&circleP, &ev);
    }

    Canvas_Clear(frame, RGB888toRGB565(0x90, 0x90, 0x90));  // クリア
    Render(frame, f, &circleP);                             // 描画処理

    {
      CST328Data_t data;
      static uint8_t debounce = 0x00;
      CST328Drv_GetCoord(hTouch, &data);
      if (0 < data.points) {
        char sbuf[128] = {0};
        for (size_t i = 0; i < data.points; ++i) {
          sprintf(sbuf, "%d: X[%3d] Y[%3d] S[%3d]", i, data.coords[i].x, data.coords[i].y, data.coords[i].strength);
          Textbox_Push(hTextbox, sbuf, strlen(sbuf));
        }
      }
      renderTextbox(frame, hTextbox, 10, 26, RGB888toRGB565(0xf, 0xf, 0xf));
    }

    {
      char sbuf[128] = {0};
      sprintf(sbuf, "フレーム周期: %6lld us\n", difftime);
      Textbox_Push(hFrameTextbox, sbuf, strlen(sbuf));

      // テキストボックスの内容を描画
      renderTextbox(frame, hFrameTextbox, 10, 10, RGB888toRGB565(0xf, 0xf, 0xf));
    }

    etime = get_absolute_time();
    difftime = absolute_time_diff_us(btime, etime);
    btime = etime;
    LCDDrv_SwapBuff(hLcd, Canvas_GetBuf(frame), 0, 0, 240, 320);
    f++;
  }
  return 0;
}
