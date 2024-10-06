/**
 * @file prog01/app/src/font.c
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <user/font.h>
#include <user/types.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////
/**
 * @brief FONTX2 データからcodeに該当するフォントイメージの先頭を指すポインタを取得します.
 * @param [in] font : 対象とするフォントデータ
 * @param [in] code : 文字コード JIS(7bit) or SJIS(16bit)
 * @param [out] pw : フォントデータの幅 (ドット)
 * @param [out] ph : フォントデータの高さ (ドット)
 * @param [out] pfsz : フォントデータサイズ(バイト)
 * @return フォントデータの先頭を指すポインタ
 * @retval NULL 以外 : フォントデータの先頭を指すポインタ
 * @retval NULL : 該当データなし
 */
inline static const uint8_t* getFont(const uint8_t* font, const uint16_t code, uint32_t* const pw, uint32_t* const ph, size_t* const pfsz);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////
#include "font/4X8.h"

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

inline static const uint8_t* getFont(const uint8_t* font, const uint16_t code, uint32_t* const pw, uint32_t* const ph, size_t* const pfsz) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == font) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    size_t fsz = (font[14] + 7) / 8 * font[15];  // フォントサイズの取得

    if (NULL != pw) {
      *pw = font[14];
    }

    if (NULL != ph) {
      *ph = font[15];
    }

    if (NULL != pfsz) {
      *pfsz = fsz;
    }

    if (0 == font[16]) {
      if (code < 0x100) {
        return &font[17 + code * fsz];
      }
    } else {
      const uint8_t* cblk = &font[18];
      size_t nc = 0;
      size_t bc = font[17];
      while (bc--) {
        size_t sb = cblk[0] + cblk[1] * 0x100;
        size_t eb = cblk[2] + cblk[3] * 0x100;
        if (code >= sb && code <= eb) {
          nc += code - sb;
          return &font[18 + 4 + font[17] + nc * fsz];
        }
        nc += eb - sb + 1;
        cblk += 4;
      }
    }  // if(0 == font[16])...
  }

  return NULL;
}

UError_t Font_Print(const char* sz, Font_DrawFontFn_t fn, void* arg) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == sz || NULL == fn) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    int32_t px = 0;  // 水平位置
    int32_t py = 0;  // 垂直位置
    const uint8_t* pgraph = NULL;
    uint32_t w = 0;
    uint32_t h = 0;
    size_t fsz = 0;
    for (size_t i = 0; 0 != *(sz + i); ++i) {
      const uint8_t c = (uint8_t)(*(sz + i));
      if (0x7f >= c) {
        // ANK文字
        if ((0x20 <= c) && (0x7e >= c)) {
          // printable
          pgraph = getFont(ank, c, &w, &h, &fsz);
          if (NULL != pgraph) {
            err = fn(arg, px, py, pgraph, c, w, h, fsz);
            if (uSuccess != err) {
              break;
            }
          } else {
            printf("[WARN] %s(): pgraph is null\n", __FUNCTION__);
            printf("[WARN] ank[0x%08lx] c[%c] w[%lu] h[%lu] fsz[%lu]", (uint32_t)ank, c, w, h, fsz);
          }
          px += 1;
        } else if (0x7f == c) {
          // del : 何もしない
        } else if (0x09 == c) {
          // tab : 2キャラクタ進める
          px += 2;
        } else if (0x08 == c) {
          // bs : 何もしない
        } else if (0x10 == c) {
          // CR 改行
          px = 0;
          py++;
        } else {
          // 上記以外 1キャラクタ進める
          px += 1;
        }
      } else if ((0xc2 <= c) && (0xdf >= c)) {
        // UTF8 2byte
        if (0 == *(sz + i)) {
          break;
        }
        i += 1;
        px += 2;
      } else if ((0xe0 <= c) && (0xef >= c)) {
        // UTF8 3byte
        if ((0 == *(sz + i + 1)) || (0 == *(sz + i + 2))) {
          break;
        }
        i += 2;
        px += 2;
      } else if ((0xf0 <= c) && (0xf4 >= c)) {
        // UTF8 4byte
        if ((0 == *(sz + i + 1)) || (0 == *(sz + i + 2))) {
          break;
        }
        i += 3;
        px += 2;
      } else {
        // unknown 2byte とする.
        if (0 == *(sz + i)) {
          break;
        }
        i += 1;
        px += 2;
      }
    }  // for(...
  }

  return err;
}
