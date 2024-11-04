/**
 * @file prog01/app/src/canvas.c
 *
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/canvas.h>

#include <user/types.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

inline static UError_t clear(const Canvas_t* const ctx, const uint16_t c);

inline static UError_t setPixel(const Canvas_t* const ctx, const uint16_t x, const uint16_t y, const uint16_t c);

/**
 * @brief 端点1 端点2 を結ぶ 線分を描画する.
 *
 * ブレセンハムの線分発生アルゴリズムの素直な実装
 * @param [in] ctx : 操作対象
 * @param [in] x1  : 端点1 - x座標
 * @param [in] y1  : 端点1 - y座標
 * @param [in] x2  : 端点2 - x座標
 * @param [in] y2  : 端点2 - y座標
 * @param [in] c  : RGB565 形式の描画色
 * @return 処理結果
 */
inline static UError_t setLine(const Canvas_t* const ctx, const uint16_t x1, const uint16_t y1, const uint16_t x2, const uint16_t y2, const uint16_t c);

/**
 * @brief メモリから指定位置へデータの矩形転送を行う
 * @param [in] ctx : 転送対象
 * @param [in] dx : 転送先x座標
 * @param [in] dy : 転送先y座標
 * @param [in] src : 転送元データ
 * @param [in] sx : 転送元x座標
 * @param [in] sy : 転送元y座標
 * @param [in] stride : 転送元ラインバイト数
 * @param [in] sw : 転送元幅
 * @param [in] sh : 転送元高さ
 * @return 処理結果
 */
inline static UError_t blt(const Canvas_t* const ctx, uint16_t dx, uint16_t dy, const uint16_t* src, uint16_t sx, uint16_t sy, size_t stride, uint16_t sw,
                           uint16_t sh);

inline static UError_t transBlt(const Canvas_t* ctx, uint16_t dx, uint16_t dy, const uint16_t* src, uint16_t sx, uint16_t sy, size_t stride, uint16_t sw,
                                uint16_t sh, uint16_t const trans);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

inline static UError_t clear(const Canvas_t* const ctx, const uint16_t c) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == ctx->buf) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    uint16_t* addr = (uint16_t*)ctx->buf;
    for (size_t y = 0; y < ctx->h; ++y) {
      for (size_t x = 0; x < ctx->w; ++x) {
        *(addr + x) = c;
      }
      addr += ctx->s;
    }
  }
  return err;
}

inline static UError_t blt(const Canvas_t* const ctx, uint16_t dx, uint16_t dy, const uint16_t* src, uint16_t sx, uint16_t sy, size_t stride, uint16_t sw,
                           uint16_t sh) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == ctx->buf || NULL == src) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    // 領域チェック
    if (ctx->h <= dy || ctx->w <= dx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    uint16_t* const base = (uint16_t*)ctx->buf;

    uint16_t* daddr = base + dx + ctx->s * dy;

    const uint16_t* saddr = src + sx + (sy * (stride >> 1));

    const size_t w = (ctx->w - dx) > sw ? sw : (ctx->w - dx);
    const size_t h = (ctx->h - dy) > sh ? sh : (ctx->h - dy);

    if (0 < w) {
      for (size_t y = 0; y < h; ++y) {
        memcpy(daddr, saddr, sizeof(uint16_t) * w);
        daddr += ctx->s;
        saddr += (stride >> 1);
      }
    }
  }

  return err;
}

inline static UError_t transBlt(const Canvas_t* ctx, uint16_t dx, uint16_t dy, const uint16_t* src, uint16_t sx, uint16_t sy, size_t stride, uint16_t sw,
                                uint16_t sh, uint16_t const trans) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == ctx->buf || NULL == src) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    // 領域チェック
    if (ctx->h <= dy || ctx->w <= dx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    const uint16_t* base = src + (sy * (stride >> 1)) + sx;

    for (uint16_t y = 0; y < sh; ++y) {
      for (uint16_t x = 0; x < sw; ++x) {
        if (trans != *(base + x)) {
          setPixel(ctx, dx + x, dy + y, *(base + x));
        }
      }
      base += (stride >> 1);
    }
  }

  return err;
}

inline static UError_t setPixel(const Canvas_t* const ctx, const uint16_t x, const uint16_t y, const uint16_t c) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == ctx->buf) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (ctx->w <= x || ctx->h <= y) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    const size_t offset = (y * ctx->s) + x;
    uint16_t* const addr = (uint16_t*)ctx->buf;
    *(addr + offset) = c;
  }

  return err;
}

inline static UError_t setLine(const Canvas_t* const ctx, const uint16_t x1, const uint16_t y1, const uint16_t x2, const uint16_t y2, const uint16_t c) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == ctx->buf) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    const int32_t dx = (x2 > x1) ? x2 - x1 : x1 - x2;
    const int32_t dy = (y2 > y1) ? y2 - y1 : y1 - y2;

    const int32_t sx = (x2 > x1) ? 1 : -1;
    const int32_t sy = (y2 > y1) ? 1 : -1;

    int32_t px = x1;
    int32_t py = y1;
    if (dx > dy) {
      int32_t e = dx * -1;
      for (int32_t i = 0; i < dx; ++i) {
        (void)setPixel(ctx, px, py, c);
        px += sx;
        e += dy + dy;
        if (e >= 0) {
          py += sy;
          e -= dx + dx;
        }
      }
    } else {
      int32_t e = dy * -1;
      for (int32_t i = 0; i < dy; ++i) {
        (void)setPixel(ctx, px, py, c);
        py += sy;
        e += dx + dx;
        if (e >= 0) {
          px += sx;
          e -= dy + dy;
        }
      }
    }
  }

  return err;
}

UError_t Canvas_Create(Canvas_t* ctx, uint16_t w, uint16_t h, size_t s, void* const buf) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    ctx->w = w;
    ctx->h = h;
    ctx->s = s;
    ctx->buf = buf;
  }

  return err;
}

const void* Canvas_GetBuf(const Canvas_t* const ctx) {
  if (NULL == ctx) {
    return NULL;
  }
  return ctx->buf;
}

UError_t Canvas_Clear(const Canvas_t* const ctx, const uint16_t c) { return clear(ctx, c); }

UError_t Canvas_Blt(const Canvas_t* const ctx, uint16_t dx, uint16_t dy, const uint16_t* src, uint16_t sx, uint16_t sy, size_t stride, uint16_t sw,
                    uint16_t sh) {
  return blt(ctx, dx, dy, src, sx, sy, stride, sw, sh);
}

UError_t Canvas_TextureBlt(const Canvas_t* ctx, uint16_t dx, uint16_t dy, const Texture_t* tex, uint16_t sx, uint16_t sy, uint16_t sw, uint16_t sh) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == tex) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    err = blt(ctx, dx, dy, tex->buf, sx, sy, tex->s, sw, sh);
  }

  return err;
}

UError_t Canvas_TextureTransBlt(const Canvas_t* ctx, uint16_t dx, uint16_t dy, const Texture_t* tex, uint16_t sx, uint16_t sy, uint16_t sw, uint16_t sh,
                                uint16_t trans) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == tex) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    err = transBlt(ctx, dx, dy, tex->buf, sx, sy, tex->s, sw, sh, trans);
  }

  return err;
}

UError_t Canvas_DrawPixel(const Canvas_t* const ctx, const uint16_t x, const uint16_t y, const uint16_t c) { return setPixel(ctx, x, y, c); }

UError_t Canvas_DrawLine(const Canvas_t* const ctx, const uint16_t x1, const uint16_t y1, const uint16_t x2, const uint16_t y2, const uint16_t c) {
  return setLine(ctx, x1, y1, x2, y2, c);
}

UError_t Canvas_DrawCircle(const Canvas_t* const ctx, const uint16_t x, const uint16_t y, const uint16_t r, const uint16_t c) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    int32_t cx = r;
    int32_t cy = 0;
    int32_t ca = r;

    (void)setPixel(ctx, x, y - r, c);
    (void)setPixel(ctx, x, y + r, c);
    (void)setPixel(ctx, x + r, y, c);
    (void)setPixel(ctx, x - r, y, c);

    while (cx >= cy) {
      ca = ca - cy - cy - 1;
      cy = cy + 1;
      if (ca < 0) {
        ca = ca + cx + cx - 1;
        cx = cx - 1;
      }
      (void)setPixel(ctx, x + cx, y + cy, c);
      (void)setPixel(ctx, x + cx, y - cy, c);
      (void)setPixel(ctx, x - cx, y - cy, c);
      (void)setPixel(ctx, x - cx, y + cy, c);

      (void)setPixel(ctx, x + cy, y + cx, c);
      (void)setPixel(ctx, x + cy, y - cx, c);
      (void)setPixel(ctx, x - cy, y - cx, c);
      (void)setPixel(ctx, x - cy, y + cx, c);
    }  // while(cx >= ...
  }
  return err;
}

UError_t Canvas_DrawFillCircle(const Canvas_t* const ctx, const uint16_t x, const uint16_t y, const uint16_t r, const uint16_t c) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    int32_t cx = r;
    int32_t cy = 0;
    int32_t ca = r;

    (void)setPixel(ctx, x, y - r, c);
    (void)setPixel(ctx, x, y + r, c);
    (void)setPixel(ctx, x + r, y, c);
    (void)setPixel(ctx, x - r, y, c);
    (void)setLine(ctx, x - r, y, x + r, y, c);

    while (cx >= cy) {
      ca = ca - cy - cy - 1;
      cy = cy + 1;
      if (ca < 0) {
        ca = ca + cx + cx - 1;
        cx = cx - 1;
      }

      (void)setLine(ctx, x - cx, y + cy, x + cx, y + cy, c);
      (void)setLine(ctx, x - cx, y - cy, x + cx, y - cy, c);
      (void)setLine(ctx, x - cy, y + cx, x + cy, y + cx, c);
      (void)setLine(ctx, x - cy, y - cx, x + cy, y - cx, c);

    }  // while(cx >= ...
  }
  return err;
}

UError_t Canvas_DrawFont(const Canvas_t* canvas, const uint16_t posx, const uint16_t posy, const uint16_t color, const uint8_t* fgraph, const uint16_t fw,
                         const uint16_t fh, const size_t fsz) {
  UError_t err = uSuccess;
  if (uSuccess == err) {
    if (NULL == canvas || NULL == fgraph || 0 == fsz) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    // LOG_I("w[%d] h[%d] fsz[%d]", fw, fh, fsz);
    const size_t fbw = (fw + (8u - 1u)) / 8u;  // フォントのバイト幅
    const uint8_t* addr = fgraph;
    for (uint32_t fy = 0; fy < fh; ++fy) {
      for (uint32_t fx = 0; fx < fw; ++fx) {
        const size_t bpos = (fx >> 3);       //(fx / 8);
        const size_t bshift = (fx & 0b111);  //(fx % 8);
        const uint8_t c = *(addr + bpos) << bshift;
        if (c & 0b10000000) {
          setPixel(canvas, posx + fx, posy + fy, color);
        }
      }
      addr += fbw;
    }
  }
}
