/**
 * @file prog01/app/src/gui/textbox.c
 *
 * @date 2024.10.26 k.shibata newly created
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/gui/textbox.h>

#include <user/types.h>

#include <user/canvas.h>
#include <user/fontx2.h>
#include <user/textbox.h>
#include <user/utf8string.h>

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

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

/**
 * Textbox を描画
 * @param canvas : 描画先
 * @param textbox : 出力するTextboxインスタンス
 * @param posx : 出力座標 x
 * @param posy : 出力座標 y
 * @param color : 色
 * @return
 */
inline static UError_t textboxRender(GUITextbox_t* ctx, Canvas_t const* canvas, const uint32_t posx, const uint32_t posy, const uint16_t color) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == canvas || NULL == ctx || NULL == ctx->textbox_) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    TextboxHandle_t const textbox = ctx->textbox_;
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
              Canvas_DrawFont(canvas, curX, curY, color, fgraph, fw, fh, fsz);
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

/**
 * GUITextbox_t アダプタに関連付けられたTextbox_tをCanvas_tに描画する
 * @param [in] pGui : 使用するGUITextbox_t アダプタ
 * @param [out] canvas : 描画先
 * @param [in] posx : 出力座標 x
 * @param [in] posy : 出力座標 y
 * @param [in] color : 色
 * @return
 */
UError_t GUITextboxRender(GUITextbox_t* pGui, Canvas_t const* canvas, const uint32_t posx, const uint32_t posy, const uint16_t color) {
  UError_t err = uSuccess;
  GUITextbox_t* ctx = (GUITextbox_t*)pGui;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == canvas) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    err = textboxRender(ctx, canvas, posx, posy, color);
  }

  return err;
}

/**
 * @brief アダプタ GUITextbox_t インスタンスを初期化する
 * @param pGui
 * @param textbox
 * @return
 */
UError_t GUITextbox_Create(GUITextbox_t* pGui, TextboxHandle_t textbox) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == pGui) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    pGui->textbox_ = textbox;
  }

  return err;
}

/**
 * @brief GUITextbox_t に Textbox を関連付ける
 * @param pGui
 * @param textbox
 * @return
 */
UError_t GUITextbox_Attach(GUITextbox_t* pGui, TextboxHandle_t textbox) {
  UError_t err = uSuccess;
  GUITextbox_t* ctx = (GUITextbox_t*)pGui;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    ctx->textbox_ = textbox;
  }

  return err;
}
