/**
 * @file src/audio/audiocontext.c
 *
 * @date 2024.11.17 k.shibata newly created
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/audio/audiocontext.h>

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
 * @brief 譜面情報を進行させ, dst に 最大sizeバイトの8bit音声データを出力する
 * @param [in] ctx : 操作対象
 * @param [out] dst : 音声データ出力先
 * @param [in] size : 最大出力バイト数
 * @return 出力したバイト数
 */
static size_t AudioContext_writeScore(AudioContext_t* ctx, void* dst, size_t size);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

UError_t AudioContext_SetScore(AudioContext_t* ctx, AudioScore_t* pScore) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == pScore) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    ctx->curScore = pScore;
  }

  return err;
}

static size_t AudioContext_writeScore(AudioContext_t* ctx, void* dst, size_t size) {
  UError_t err = uSuccess;
  uint8_t* outBuf = (uint8_t*)dst;
  size_t outBytes = 0u;

  if (uSuccess != err) {
    if (NULL == ctx || NULL == outBuf || NULL == ctx->curScore) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    // size_t remain = size; // 出力可能な残りバイト
    for (outBytes = 0u; outBytes < size; ++outBytes) {
      if (AudioScore_IsFinished(ctx->curScore)) {
        break;
      } else if (AudioScore_IsNoteChanged(ctx->curScore)) {
        AudioNote_t note;
        AudioScore_GetNote(ctx->curScore, &note);
        if (0u == note.tone) {
          // 休符
          if (NULL != ctx->noteOffFn) {
            ctx->noteOffFn(note.tone, ctx->egos, ctx->ncos);
          }
        } else {
          // 音程周波数を変更
          if (NULL != ctx->noteOnFn) {
            ctx->noteOnFn(note.tone, ctx->egos, ctx->ncos);
          }
        }
      } else {
        ;
      }

      if (NULL != ctx->outputFn) {
        // 出力データ生成
        outBuf[outBytes] = ctx->outputFn(ctx->egos, ctx->ncos);
      }

      AudioScore_Step(ctx->curScore);
    }  // for(size_t i...
  }

  return outBytes;
}

size_t AudioContext_Write(AudioContext_t* ctx, void* dst, size_t const size) {
  UError_t err = uSuccess;
  size_t nbytewritten = 0u;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == dst) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (ctx->enable) {
      nbytewritten = AudioContext_writeScore(ctx, dst, size);

      if (nbytewritten != size) {
        ctx->enable = false;
      }
    }
  }

  return nbytewritten;
}
