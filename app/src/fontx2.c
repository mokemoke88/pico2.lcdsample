/**
 * @file prog01/app/src/fontx2.c
 *
 * @date 2024.10.13 k.shibata newly created
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/fontx2.h>
#include <user/types.h>

#include <stddef.h>
#include <stdint.h>

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

// 小夏フォント 等幅 16x16ドットフォント
#include "font/KONATSU_16x16_ANK.h"
#include "font/KONATSU_16x16.h"

static const uint8_t* const builtinFonts[] = {
    KONATSU_16x16_ANK,
    KONATSU_16x16,
};

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
        uint16_t sb = cblk[0] | (cblk[1] << 8);  // ブロックの先頭コード
        uint16_t eb = cblk[2] | (cblk[3] << 8);  // ブロック末尾コード
        if ((code >= sb) && (code <= eb)) {
          nc += code - sb;
          return &font[18 + 4 * font[17] + nc * fsz];
        }
        // ブロック移動
        nc += eb - sb + 1;
        cblk += 4;
      }
    }  // if(0 == font[16])...
  }

  return NULL;
}

const uint8_t* FontX2_GetFont(const uint16_t code, uint32_t* const pw, uint32_t* const ph, size_t* const pfsz) {
  const uint8_t* ret = NULL;
  if (0x100 > code) {
    if (NULL != builtinFonts[0]) {
      ret = getFont(builtinFonts[0], code, pw, ph, pfsz);
    }
  } else {
    if (NULL != builtinFonts[1]) {
      ret = getFont(builtinFonts[1], code, pw, ph, pfsz);
    }
  }
  return ret;
}
