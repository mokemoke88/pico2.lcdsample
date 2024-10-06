/**
 * @file prog01/app/src/utf8string.c
 * UTF8 文字列 に関する処理
 * @date 2024.10.12 k.shibata newly created
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/types.h>
#include <user/utf8string.h>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <user/log.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

typedef struct tagSJIS2UTF32_t {
  uint16_t sjis;
  int32_t utf32;
} SJIS2UTF32_t;

typedef struct tagUTF32toSJIS_t {
  int32_t utf32;
  uint16_t sjis;
} UTF32toSJIS_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief bsearch() 用比較関数
 * @param [in] key : (uint32_t) UNICODE 文字コード
 * @param [in] element : UTF32toSJIS_t* 変換テーブルのエレメント
 * @return bsearch() の仕様に基づく値を返す.
 */
static int compareUTF32toSJIS(const void* key, const void* element);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

#if 0
static const SJIS2UTF32_t SJIS2UTF32Conv[] = {
#include "charcode/sjis0213_2004_utf32.h"
};
#endif

static const UTF32toSJIS_t UTF32toSJISConv[] = {
#include "charcode/utf32_sjis0213_2004.h"
};

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

UError_t UTF8String_ToUTF32(int32_t* pUtf32, size_t* pRead, const char* utf8s, const size_t length) {
  UError_t err = uSuccess;
  int32_t ret = 0x0u;
  size_t nbyteread = 1u;

  if (uSuccess == err) {
    if (NULL == pUtf32 || NULL == utf8s || 0 == length) {
      err = uFailure;
      nbyteread = 0u;
    }
  }

  if (uSuccess == err) {
    if (0b11000000 == (0b11100000 & utf8s[0])) {
      // utf8 2byte
      if (2 > length) {
        err = uFailure;
      }

      if (uSuccess == err) {
        if (0b10000000 != (0b11000000 & utf8s[1])) {
          err = uFailure;
        }
      }

      if (uSuccess == err) {
        nbyteread = 2;
        ret = ((((0b00011100 & utf8s[0]) >> 2) << 8) | (((0b00000011 & utf8s[0]) << 6) & (0b00111111 & utf8s[1])));
      }
    } else if (0b11100000 == (0b11110000 & utf8s[0])) {
      // utf8 3byte
      if (3 > length) {
        err = uFailure;
      }

      for (size_t i = 1; i < 3; ++i) {
        if (0b10000000 != (0b11000000 & utf8s[i])) {
          err = uFailure;
          break;
        }
      }

      // 特殊ケース
      if (uSuccess == err) {
        if (0xe0 == utf8s[0] && (0x80 <= utf8s[1] && 0x9f >= utf8s[1])) {
          err = uFailure;
        }
        if (0xed == utf8s[0] && 0xa0 <= utf8s[1]) {
          err = uFailure;
        }
      }

      if (uSuccess == err) {
        nbyteread = 3;
        ret =
            ((((0b00001111 & utf8s[0]) << 4) | ((0b00111100 & utf8s[1]) >> 2)) << 8) | ((((0b00000011 & utf8s[1]) << 6) | ((0b00111111 & utf8s[2]) >> 0)) << 0);
      }
    } else if (0b11110000 == (0b11111000 & utf8s[0])) {
      // utf8 4byte
      if (4 > length) {
        err = uFailure;
      }

      for (size_t i = 1; i < 4; ++i) {
        if (0b10000000 != (0b11000000 & utf8s[i])) {
          err = uFailure;
          break;
        }
      }

      // 特殊ケース
      if (uSuccess == err) {
        if (0xf0 == utf8s[0] && (0x80 <= utf8s[1] && 0x8f >= utf8s[1])) {
          err = uFailure;
        }
        if (0xf4 == utf8s[0] && 0x90 <= utf8s[1]) {
          err = uFailure;
        }
      }

      if (uSuccess == err) {
        nbyteread = 4;
        ret = ((((0b00000111 & utf8s[0]) << 2) | ((0b00110000 & utf8s[1]) >> 4)) << 16) |
              ((((0b00001111 & utf8s[1]) << 4) | ((0b00111100 & utf8s[2]) >> 2)) << 8) |
              ((((0b00000011 & utf8s[2]) << 6) | ((0b00111111 & utf8s[3]) >> 0)) << 0);  // lsb
      }
    } else if (0x7fu >= utf8s[0]) {
      // utf8 1byte = ascii.
      ret = utf8s[0];
    }
  }

  if (uSuccess == err) {
    *pUtf32 = ret;
  }

  if (NULL != pRead) {
    *pRead = nbyteread;
  }
  return err;
}

static int compareUTF32toSJIS(const void* key, const void* element) {
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
  int32_t v = (uint32_t)key;
#pragma GCC diagnostic warning "-Wpointer-to-int-cast"

  int32_t c = ((const UTF32toSJIS_t*)element)->utf32;
  return v - c;
}

uint16_t UTF8String_UTF32toSJIS(int32_t utf32) {
  uint16_t retval = 0u;

  const size_t num = ((sizeof(UTF32toSJISConv)) / sizeof(UTF32toSJISConv[0]));
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
  const UTF32toSJIS_t* p = (const UTF32toSJIS_t*)bsearch((const void*)utf32, &UTF32toSJISConv[0], num, sizeof(UTF32toSJISConv[0]), &compareUTF32toSJIS);
#pragma GCC diagnostic warning "-Wint-to-pointer-cast"
  if (NULL != p) {
    retval = p->sjis;
  }

  return retval;
}
