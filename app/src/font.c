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
// #include "font/4X8.h"
#include "font/ILGH16XB.h"

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

const uint8_t* Font_GetAnkFont(const uint16_t code, uint32_t* const pw, uint32_t* const ph, size_t* const pfsz) {
  const uint8_t* ret = getFont(ank16, code, pw, ph, pfsz);
  return ret;
}

/**
 * @brief code で指定した文字を fn関数を使用して描画します.
 * @param [in] code : 文字コード (SJIS)
 * @param [in] x : 描画位置x 
 * @param [in] y : 描画位置y 
 * @param [in] color : カラーコード 
 * @param [out] pw : 出力フォント幅(pixel)
 * @param [out] ph : 出力フォント幅(pixel)
 * @param [in] fn : ピクセル描画関数 
 * @param [in] arg : パラメータパック 
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t Font_DrawChar(uint16_t code, uint32_t x, uint32_t y, uint32_t color, uint32_t* pw, uint32_t* ph, Font_DrawPixelFn_t fn, void* arg){
  UError_t err = uSuccess;
  if(uSuccess == err){
    if(NULL == fn){
      err = uFailure;
    }
  }

  if(uSuccess == err){
    const uint8_t* pg = NULL; // フォントデータ受け先
    uint32_t fw = 0;
    uint32_t fh = 0;
    size_t fsz = 0;

    if(0x7f >= code){
      pg = getFont(ank16, code, &fw, &fh, &fsz);
      if(NULL != pw){
        *pw = fw;
      }
      if(NULL != ph){
        *ph = fh;
      }
    }else{
      ;
    }

    if(NULL != pg){
      const size_t fbw = (fw + 7) / 8; // フォントのバイト幅
      const uint8_t* addr = pg;
      for(uint32_t fy = 0; fy < fh; ++fy){
        for(uint32_t fx = 0; fx < fw; ++fx){
          const size_t bpos = (fx / 8);
          const size_t bshift = (fx % 8);
          uint8_t c = *(addr + bpos) << bshift;
          if(c & 0b10000000){
            fn(arg, x + fx, y + fy, color);
          }
        }
        addr += fbw;
      }
    } // if(NULL != pg...
  }

  return err;
}

UError_t Font_Print(const char* sz, uint32_t color, Font_DrawPixelFn_t fn, void* arg) {
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

          Font_DrawChar(c, px, py, color, NULL, NULL, fn, arg);
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
        if (0 == *(sz + i + 1)) {
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
        if ((0 == *(sz + i + 1)) || (0 == *(sz + i + 2)) || (0 == *(sz + i + 3))) {
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
