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

UError_t EGOParam_Create(EGOParam_t* param, int32_t peak, int32_t atk, int32_t decay, int32_t sus, int32_t rel) {
  UError_t err = uSuccess;
  if (uSuccess == err) {
    if (NULL == param) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (65536 < peak || 0 > peak) {
      err = uFailure;
    }
    if (65536 < sus || 0 > sus) {
      err = uFailure;
    }
    if (0 >= atk) {
      err = uFailure;
    }
    if (0 >= decay) {
      err = uFailure;
    }
    if (0 >= rel) {
      err = uFailure;
    }
    if (sus > peak) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    // 内部は10bit固定少数
    param->l4 = 0;
    param->l1 = peak << 10;
    param->l2 = sus << 10;
    param->l3 = sus << 10;
    param->r1 = (param->l1 - param->l4) / atk;
    param->r2 = (param->l2 - param->l1) / decay;
    param->r3 = 0;
    param->r4 = (param->l4 - param->l3) / rel;
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
    // 出力をクリップ
    ret = (ctx->v >> 10);
    ret = (ret > 65536) ? 65536 : (ret < 0) ? 0 : ret;

    // 状態更新
    switch (ctx->state) {
      case EGO_STATE_T1: {
        ctx->v += ctx->param->r1;
        // 遷移条件の確認と遷移
        if (ctx->v >= ctx->param->l1) {
          ctx->v = ctx->param->l1;
          ctx->state = (0 != ctx->param->r2) ? EGO_STATE_T2 : (0 != ctx->param->r3) ? EGO_STATE_T3 : EGO_STATE_T4;
        }
      } break;
      case EGO_STATE_T2: {
        ctx->v += ctx->param->r2;
        // 遷移条件の確認と遷移
        if (0 < ctx->param->r2) {
          if (ctx->v >= ctx->param->l2) {
            ctx->v = ctx->param->l2;
            ctx->state = (0 != ctx->param->r3) ? EGO_STATE_T3 : EGO_STATE_T4;
          }
        } else {
          if (ctx->v <= ctx->param->l2) {
            ctx->v = ctx->param->l2;
            ctx->state = (0 != ctx->param->r3) ? EGO_STATE_T3 : EGO_STATE_T4;
          }
        }
      } break;
      case EGO_STATE_T3: {
        ctx->v += ctx->param->r3;
        // 遷移条件の確認と遷移
        if (0 < ctx->param->r3) {
          if (ctx->v >= ctx->param->l3) {
            ctx->v = ctx->param->l3;
            ctx->state = EGO_STATE_T4;
          }
        } else {
          if (ctx->v <= ctx->param->l3) {
            ctx->v = ctx->param->l3;
            ctx->state = EGO_STATE_T4;
          }
        }
      } break;
      case EGO_STATE_T4: {
        ctx->v += ctx->param->r4;
        // 遷移条件の確認と遷移
        if (ctx->v <= ctx->param->l4) {
          ctx->v = ctx->param->l4;
          ctx->state = EGO_STATE_FINISH;
        }
      } break;
      default:
        break;
    }
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
    ctx->state = EGO_STATE_T1;
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
    if (EGO_STATE_FINISH != ctx->state) {
      ctx->state = EGO_STATE_T4;
    }
  }
}
