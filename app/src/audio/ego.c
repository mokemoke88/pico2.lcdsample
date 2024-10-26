/**
 * @file prog01/app/src/audio/ego.c
 *
 * エンベロープジェネレータ
 *
 * @date 2024.11.04 k.shibata newly created
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/audio/ego.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief エンベロープジェネレータのパラメータを出力します.
 * ピーク音量(0-65535), アタックタイム(単位:サンプル), ディケイタイム(単位:サンプル), サスティーン音量(0-65535)
 * リリースタイム(単位:サンプル) を入力として パラメータを出力します
 * @param [out] param : パラメータ出力先
 * @param [in] peak : ピーク音量 (0-65535) 最大音量(0-65535 = 0.0 ... 1.0)
 * @param [in] atk : アタック時間 キーオンからピーク音量までの到達時間(単位: サンプル)
 * @param [in] decay : アタック時間到達後, 持続音量(サスティーン音量) までの到達時間(単位:サンプル)
 * @param [in] sus : 持続音量 (0-65535) (0-65535 = 0.0 ... 1.0)
 * @param [in] rel : キーオフで, サスティーン音量から音量が0になるまでの時間(単位:サンプル)
 */
UError_t EGOParam_Create(EGOParam_t* param, uint32_t peak, uint32_t atk, uint32_t decay, uint32_t sus, uint32_t rel) {
  UError_t err = uSuccess;
  if (uSuccess == err) {
    if (NULL == param) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    param->d_atk = peak / atk;
    param->t_atk = atk;
    param->d_decay = ((peak - sus) / decay) * -1;
    param->t_decay = decay;
    param->d_rel = (sus / rel) * -1;
    param->t_rel = rel;
  }

  return err;
}

uint32_t EGO_Get(EGO_t* ctx) {
  UError_t err = uSuccess;
  uint32_t ret = 0u;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == ctx->param) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    ret = ctx->v;

    // リリース状態の場合, d_relに従って増加(減少)
    if (ctx->isRel) {
      ctx->v += ctx->param->d_rel;
      if (0 > ctx->v) {
        ctx->v = 0;
      }
    } else {
      if ((ctx->t) < (ctx->param->t_atk)) {
        // atk 区間なら d_atk 分 増量(減少)
        ctx->v += ctx->param->d_atk;
      } else if ((ctx->t) < (ctx->param->t_atk + ctx->param->t_decay)) {
        // decay 区間なら d_decay 分 増量(減少)
        ctx->v += ctx->param->d_decay;
      }
    }
    ++ctx->t;
  }

  return ret;
}

void EGO_NoteOn(EGO_t* ctx) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == ctx->param) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    ctx->t = 0;
    ctx->v = 0;
    ctx->isRel = false;
  }
}

void EGO_NoteOff(EGO_t* ctx) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == ctx->param) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    ctx->isRel = true;
  }
}
