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
#include <user/font.h>
#include <user/lcddrv.h>
#include <user/spidrv.h>
#include <user/types.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

#define RGB888toRGB565(R, G, B) (uint16_t)(((((G & 0b11111100) << 3) | ((B & 0b11111000) >> 3)) << 8) | ((R & 0b11111000) | ((G & 0b11111100) >> 5)))

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
} StringContext_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

static uint16_t framebuf[240 * 320 * 2] = {0};  //< フレームバッファメモリ

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
    Canvas_DrawFillCircle(canvas, 200, 200, (f % 20) + 1, RGB888toRGB565(0x0f, 0x0f, 0xff));
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
  LCDDrvContext_t lcd;
  Canvas_t frame[2] = {0};

  SPIDrv_Create(&spi);
  SPIDrvHandle_t hSpi = (SPIDrvHandle_t)&spi;
  LCDDrv_Create(&lcd, hSpi);
  LCDDrvHandle_t hLcd = (LCDDrvHandle_t)&lcd;

  Canvas_Create(&frame[0], 240, 320, 240, &framebuf[0]);
  Canvas_Create(&frame[1], 240, 320, 240, &framebuf[240 * 320]);

  SPIDrv_Init(hSpi, 25 * 1000 * 1000);
  LCDDrv_Init(hLcd);

  LCDDrv_InitalizeHW(hLcd);

  uint16_t bright = 0u;

  LCDDrv_Clear(hLcd, 0u, 0u, 0u);
  LCDDrv_SetBrightness(hLcd, 0x7fff);

  printf("[DEBUG] Enter EventLoop\n");

  uint32_t f = 0u;
  absolute_time_t btime = get_absolute_time();
  absolute_time_t etime = get_absolute_time();
  absolute_time_t difftime = 0;
  while (true) {
    // absolute_time_t b = get_absolute_time();
    // LCDDrv_SetBrightness(hLcd, bright++);
    // absolute_time_t e = get_absolute_time();
    // printf("LCDDrv_Clear():%lld %lld %lld\n", b, e, absolute_time_diff_us(b,
    // e));

    Canvas_t* canvas = (f % 2) ? &frame[0] : &frame[1];  // フレームバッファ切替

    Canvas_Clear(canvas, RGB888toRGB565(0x90, 0x90, 0x90));  // クリア
    Render(canvas, f);

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
    etime = get_absolute_time();
    difftime = absolute_time_diff_us(btime, etime);
    btime = etime;
    LCDDrv_SwapBuff(hLcd, Canvas_GetBuf(canvas), 0, 0, 240, 320);
    f++;
  }
  return 0;
}
