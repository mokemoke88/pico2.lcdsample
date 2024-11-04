/**
 * @file prog01/app/src/app/diag.c
 *
 * @date 2024.10.27 k.shibata newly created
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/types.h>

#include <pico/types.h>

#include <app/diag.h>

#include <user/cst328drv.h>
#include <user/gui.h>
#include <user/textbox.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <user/macros.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

static UError_t DiagShow_(DiagObject_t* diag, const DiagArg_t* arg);
static UError_t DiagHide_(DiagObject_t* diag, const DiagArg_t* arg);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief 表示状態処理
 * @param diag 
 * @param arg 
 * @return 
 */
static UError_t DiagShow_(DiagObject_t* diag,
                          const DiagArg_t* arg) {  // absolute_time_t diffFrame, absolute_time_t diffProc, TextboxHandle_t text, Canvas_t* frame) {
  char stringBuf[128] = {0};
  sprintf(stringBuf, "フレーム周期: %6lld us\n", arg->frameTime);
  Textbox_Push(arg->text, stringBuf, strlen(stringBuf));
  sprintf(stringBuf, "音声処理時間: %6lld us\n", arg->procTime);
  Textbox_Push(arg->text, stringBuf, strlen(stringBuf));
  // テキストボックスの内容を描画
  {
    GUITextbox_t guiText = {0};
    if (uSuccess == GUITextbox_Create(&guiText, arg->text)) {
      GUITextboxRender(&guiText, arg->frame, 10, 10, RGB888toRGB565(0xf, 0xf, 0xf));
    }
  }

  // 状態遷移判断: ヒットしている場合, 非表示状態へ遷移
  if (diag->hit) {
    diag->ActionFn = &DiagHide_;
  }

  return uSuccess;
}

/**
 * @brief 非表示状態処理
 * @param diag 
 * @param arg 
 * @return 
 */
static UError_t DiagHide_(DiagObject_t* diag, const DiagArg_t* arg) {
  // 状態遷移判断: ヒットしている場合, 表示状態へ遷移
  if (diag->hit) {
    diag->ActionFn = &DiagShow_;
  }

  return uSuccess;
}

UError_t DiagUpdate(DiagObject_t* diag, const DiagArg_t* arg) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == diag || NULL == arg || NULL == arg->touch) {
      err = uFailure;
    }
  }

  // ヒットテスト
  if (uSuccess == err) {
    diag->hit = false;
    if (0 != arg->touch->points) {
      if (!diag->press) {
        diag->press = true;
        if ((0 <= (arg->touch->coords[0].y)) && (24 >= (arg->touch->coords[0].y))) {
          diag->hit = true;
        }
      }
    } else {
      // 離れた判定
      diag->press = false;
    }
  }

  return diag->ActionFn(diag, arg);  // DiagShow(arg->frameTime, arg->procTime, arg->text, arg->frame);
}

DiagObject_t DiagInit(void) {
  DiagObject_t diag = {
      .ActionFn = &DiagShow_,  // 初期状態
      .hit = false,
      .press = false,
  };
  return diag;
}
