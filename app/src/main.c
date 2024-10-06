/**
 * @file src/main.c
 * @date 2024.09.23 kshibata@seekers.jp
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include <pico/binary_info.h>
#include <pico/stdlib.h>

#include <user/canvas.h>
#include <user/console.h>
#include <user/font.h>
#include <user/lcddrv.h>
#include <user/macros.h>
#include <user/spidrv.h>
#include <user/types.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

typedef struct tagStringContext_t {
  const Canvas_t* canvas;
  uint32_t orgX;
  uint32_t orgY;
  uint32_t curX;
  uint32_t curY;
  uint16_t color;
  Font_DrawPixelFn_t drawPixelFn;
} StringContext_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

static uint16_t framebuf[240 * 320 * 2] = {0};    //< フレームバッファメモリ
static uint8_t gConsoleDataHeap[50 * 256] = {0};  //< コンソール用データ領域
static LineBuffer_t gConsoleLineHeap[50] = {0};   //< コンソール用行バッファ領域

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief 文字描画処理
 * @param ctx
 * @param x
 * @param y
 * @param fp
 * @param c
 * @param fw
 * @param fh
 * @param fsz
 * @return
 */
static UError_t drawCharactor(void* arg, uint32_t x, uint32_t y, const void* fp, uint8_t c, uint32_t fw, uint32_t fh, size_t fsz) {
  // printf("[DEBUG] x[%lu] y[%lu] fp[0x%08lx] c[%c] w[%lu] h[%lu] fsz[%lu]\n",
  // x, y,
  //        (uint32_t)fp, c, fw, fh, fsz);
  if (NULL != arg) {
    StringContext_t* ctx = (StringContext_t*)arg;
    if (NULL != fp) {
      const uint8_t* addr = fp;
      const uint32_t posx = ctx->curX + (x * fw);
      const uint32_t posy = ctx->curY + (y * fh);
      for (uint32_t h = 0; h < fh; ++h) {
        uint8_t c = *addr;
        for (uint32_t w = 0; w < fw; ++w) {
          if ((c & 0b10000000)) {
            Canvas_DrawPixel(ctx->canvas, posx + w, posy + h, ctx->color);
          } else {
          }
          c = c << 1;
        }  // for(w...)
        addr++;
      }  // for(h...)
    }
  }

  return uSuccess;
}

/**
 * @brief Font_DrawChar() に渡す描画関数
 * arg->canvas の指定座標にcolorコードの点を描画します.
 * @param [in] arg : パラメータパック
 * @param [in] x : 描画先 x座標
 * @param [in] y : 描画先 y座標
 * @param [in] color : カラーコード
 */
static void consoleDrawPixel(void* arg, uint32_t x, uint32_t y, uint32_t color) {
  if (NULL != arg) {
    StringContext_t* ctx = (StringContext_t*)arg;
    Canvas_DrawPixel(ctx->canvas, x, y, color);
  }
}

/**
 * @brief consoleFlush() に渡す行処理関数
 * 行単位で呼び出される想定.
 * ラインバッファに登録されている文字を描画します.
 *
 * @param [in] arg : パラメータパック
 * @param [in] buf : 行バッファ
 * @param [in] rowHeight : コンソールに設定している1行の高さ(単位:pixel)
 * @return
 */
static UError_t consoleProces(void* arg, const LineBuffer_t* buf, const size_t rowHeight) {
  // printf("arg[0x%08lx] buf[0x%08lx]\n", (uint32_t)arg, (uint32_t)buf);
  if (NULL != arg) {
    StringContext_t* ctx = (StringContext_t*)arg;
    uint32_t fw = 0, fh = 0;
    for (size_t i = 0, x = 0; i < buf->bytesize; ++i) {
      Font_DrawChar(((uint8_t*)(buf->ptr))[i], ctx->curX + x, ctx->curY, ctx->color, &fw, &fh, ctx->drawPixelFn, ctx);
      x += fw;  // 描画幅分移動
    }  // for(size_t i...
    ctx->curY += rowHeight;
  }
  return uSuccess;
}

/**
 * @brief レンダリング処理
 * @param canvas
 * @param f
 * @return
 */
static UError_t Render(Canvas_t const* canvas, const uint32_t f) {
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
  }
  return err;
}

/**
 * エントリポイント
 */
int main(void) {
  stdio_init_all();
  printf("PICO2 LCD Controller \r\n");

  SPIDrvContext_t spi;
  SPIDrv_Create(&spi);
  SPIDrvHandle_t hSpi = (SPIDrvHandle_t)&spi;

  LCDDrvContext_t lcd;
  LCDDrv_Create(&lcd, hSpi);
  LCDDrvHandle_t hLcd = (LCDDrvHandle_t)&lcd;

  // Canvas生成
  Canvas_t canvas[2] = {0};
  Canvas_Create(&canvas[0], 240, 320, 240, &framebuf[0]);
  Canvas_Create(&canvas[1], 240, 320, 240, &framebuf[240 * 320]);

  // コンソールインスタンス生成
  ConsoleContext_t console;
  Console_Create(&console, 10, 16, gConsoleLineHeap, gConsoleDataHeap, 256, 50);
  ConsoleHandle_t hConsole = (ConsoleHandle_t)&console;

  // SPI初期化
  SPIDrv_Init(hSpi, 25 * 1000 * 1000);
  LCDDrv_Init(hLcd);

  // LCD初期化
  LCDDrv_InitalizeHW(hLcd);

  // LCD黒でクリア
  LCDDrv_Clear(hLcd, 0u, 0u, 0u);
  // LCD輝度調整
  LCDDrv_SetBrightness(hLcd, 0x7fff);

  printf("[DEBUG] Enter EventLoop\n");

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

    Canvas_Clear(frame, RGB888toRGB565(0x90, 0x90, 0x90));  // クリア
    Render(frame, f);                                       // 描画処理

#if 0
    // 文字列描画
    {
      StringContext_t printCtx = {
          .canvas = canvas,
          .orgX = 0,
          .orgY = 0,
          .curX = 10,
          .curY = 10,
          .color = 0xffff,
      };
      char sbuf[1024];
      sprintf(sbuf, "Frametime: %9lld us\n", difftime);
      Font_Print(sbuf, &drawCharactor, &printCtx);
    }
#endif
    {
      // 最前面にコンソールで, フレーム周期を表示
      char sbuf[1024];
      sprintf(sbuf, "Frametime: %9lld us\n", difftime);
      Console_StringPush(hConsole, sbuf);
      StringContext_t printCtx = {
          .canvas = frame,
          .orgX = 0,
          .orgY = 0,
          .curX = 8,
          .curY = 8,
          .color = 0xffff,
          .drawPixelFn = &consoleDrawPixel,
      };
      // コンソールの内容を走査し, Canvasに出力する
      Console_Flush(hConsole, &consoleProces, &printCtx);
    }

    etime = get_absolute_time();
    difftime = absolute_time_diff_us(btime, etime);
    btime = etime;
    LCDDrv_SwapBuff(hLcd, Canvas_GetBuf(frame), 0, 0, 240, 320);
    f++;
  }
  return 0;
}
