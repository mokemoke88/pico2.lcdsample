/**
 * @file prog01/app/src/textbox.c
 * 
 * 最大指定行数, 各行最大指定バイト数のデータを保持するバッファ構造
 *
 * @date 2024.10.13 k.shibata newly created
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/textbox.h>

#include <user/linkedlist.h>
#include <user/types.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <user/log.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

typedef struct tagTextboxLineBuffer_t {
  LinkedList_t link;  //< リンクノード
  void* ptr;          //< メモリポインタ
  size_t size;        //< メモリサイズ
  size_t length;      //< データサイズ
} TextboxLineBuffer_t;

typedef struct tagTextboxContext_t {
  LinkedList_t linebufPool;
  LinkedList_t linebuf;
  size_t numRows;
} TextboxContext_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

/**
 * 8bit配列 target の bits ビット目が 立っているかを返す
 * @param [in] target : 検査対象
 * @param bits : 検査ビット 0要素目, LSBから数えたビット数
 * @return 検査結果
 * @retval true : 該当ビットが立っている
 * @retval false : 該当ビットが寝ている
 */
inline static bool TESTBIT8(const uint32_t* target, size_t bits);

/**
 * 1になっているビットの個数を返す
 * @param [in] target : 対象
 * @return 1になっているビットの個数
 */
inline static size_t COUNTBIT8(uint8_t target);

/**
 * 指定値以上の最小の2のべき乗を出力する
 * @param [in] value : 指定値
 * @return 最小の2のべき乗
 */
inline static size_t NEARPOW2(size_t value);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

// Linebuf, Textbox のリソースを定義

#define BUILTIN_LINEBUF_NUM (32)
#define BUILTIN_LINEBUF_STATUS_NUM ((BUILTIN_LINEBUF_NUM + 7) / 8)

static TextboxLineBuffer_t builtinLinebuf[BUILTIN_LINEBUF_NUM] = {0};
static uint8_t builtinLinebufs_status[BUILTIN_LINEBUF_STATUS_NUM] = {0};

#define BUILTIN_TEXTBOX_NUM (8)
#define BUILTIN_TEXTBOX_STATUS_NUM ((BUILTIN_TEXTBOX_NUM + 7) / 8)

static TextboxContext_t builtinTextbox[BUILTIN_TEXTBOX_NUM] = {0};
static uint8_t builtinTextboxes_status[BUILTIN_TEXTBOX_STATUS_NUM] = {0};

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

inline static bool TESTBIT8(const uint32_t* target, size_t bits) {
  size_t pos = bits >> 3;
  size_t shift = bits & 0b0111;
  return (target[pos] & (1u << shift)) ? true : false;
}

inline static size_t COUNTBIT8(uint8_t target) {
  size_t ret = 0;
  for (; 0 != target; target &= target - 1) {
    ++ret;
  }
  return ret;
}

inline static size_t NEARPOW2(size_t value) {
  if (0u == value) {
    return value;
  }

  if (0u == (value & (value - 1))) {
    return value;
  }

  size_t ret = 1u;
  while (value > 0) {
    ret <<= 1;
    value >>= 1;
  }
  return ret;
}

/**
 * Textbox インスタンスを生成する
 * @param [out] pHandle : 生成したTextboxハンドル
 * @param [in] mem : メモリリソース
 * @param [in] size : メモリリソースのサイズ
 * @param [in] lineSize : 1ラインバッファのサイズ
 * @param [in] numLine : 出力に使用する行数
 * @return
 */
UError_t Textbox_Create(TextboxHandle_t* pHandle, void* mem, size_t size, size_t lineSize, size_t numLine) {
  UError_t err = uSuccess;

  const size_t realsize = NEARPOW2(lineSize);
  // LOG_D("realsize [%d]", realsize);
  if (uSuccess == err) {
    if (NULL == pHandle || NULL == mem || 0 == size || 0 == realsize || 0 == numLine) {
      err = uFailure;
      // LOG_E("invalid argument");
    }
    if (size < (realsize * (numLine + 1))) {
      err = uFailure;
      // LOG_E("size [%d] < (realsize * (numLine + 1)[%d]", size, (realsize * (numLine + 1)));
    }
  }

  // リソースが確保できるかを確認. Textbox * 1, lineBuffer * (numLine + 1)
  if (uSuccess == err) {
    // Textbox
    if (uSuccess == err) {
      size_t e = 0;
      for (size_t i = 0; i < BUILTIN_TEXTBOX_STATUS_NUM; ++i) {
        e += (8 - COUNTBIT8(builtinTextboxes_status[i]));
      }
      if (1 > e) {
        LOG_W("textbox allocate resource failure");
        err = uFailure;  // 　空き無し.
      }
    }
    // Linebuffer
    if (uSuccess == err) {
      size_t e = 0;
      for (size_t i = 0; i < BUILTIN_LINEBUF_STATUS_NUM; ++i) {
        e += (8 - COUNTBIT8(builtinLinebufs_status[i]));
      }
      if ((numLine + 1) > e) {
        LOG_W("linebuf allocate resource failure");
        err = uFailure;  // 　空き無し.
      }
    }
  }

  // リソース確保
  TextboxContext_t* pCtx = NULL;
  if (uSuccess == err) {
    // Textbox
    {
      for (size_t i = 0; i < BUILTIN_TEXTBOX_STATUS_NUM; ++i) {
        if (8 > COUNTBIT8(builtinTextboxes_status[i])) {
          // 空き発見
          // ビット位置を探す.
          uint8_t p = 0u;
          uint8_t b = builtinTextboxes_status[i];
          for (; (0x1 & b); b = b >> 1) {
            ++p;
          }
          // i 番目の p ビットが開いている = 8 * i + p が空き領域.
          pCtx = &builtinTextbox[8 * i + p];
          uint8_t mask = (1u << p);
          builtinTextboxes_status[i] |= mask;
          break;
        }
      }  // for(size_t i ...
      if (NULL != pCtx) {
        // 初期化
        pCtx->linebuf.next = NULL;
        pCtx->linebuf.prev = NULL;
        pCtx->linebufPool.next = NULL;
        pCtx->linebufPool.prev = NULL;
        pCtx->numRows = numLine;
      } else {
        err = uFailure;
      }
    }  // TextBox

    // LOG_D("check textbox pass");

    // lineBuffer * (numLine + 1)　// 確保しながらtextbox のpoolに追加
    if (NULL != pCtx) {
      for (size_t n = 0; n < (numLine + 1); ++n) {
        TextboxLineBuffer_t* pLine = NULL;
        // LOG_D("n[%d]", n);

        for (size_t i = 0; i < BUILTIN_LINEBUF_STATUS_NUM; ++i) {
          // LOG_D("i[%d]", i);

          if (8 > COUNTBIT8(builtinLinebufs_status[i])) {
            // 空き発見
            // ビット位置を探す.
            uint8_t p = 0u;
            // LOG_D("builtinLinebufs_status[%d] [%02x]", i, builtinLinebufs_status[i]);
            for (uint8_t b = builtinLinebufs_status[i]; (0x1 & b); b = b >> 1) {
              ++p;
            }
            // i 番目の p ビットが開いている = 8 * i + p が空き領域.
            // LOG_D("p[%d]", p);
            pLine = &builtinLinebuf[8 * i + p];
            uint8_t mask = (1u << p);
            builtinLinebufs_status[i] |= mask;
            break;
          }
        }  // for(size_t i ...
        if (NULL == pLine) {
          err = uFailure;
          break;
        }
        // 初期化
        // LOG_D("init pLine");
        pLine->ptr = ((uint8_t*)mem) + (n * realsize);
        pLine->size = realsize;
        pLine->length = 0u;
        // LOG_D("LinkedList_PushBack ...");
        LinkedList_PushBack(&pCtx->linebufPool, (LinkedList_t*)pLine);
        // LOG_D("LinkedList_PushBack ... done");
      }  // for(size_t n...
    }  // linebuffer
  }

  if (uSuccess == err) {
    *pHandle = pCtx;
  }

  return err;
}

/**
 * @brief 破棄する
 * @param handle
 */
void Textbox_Destroy(TextboxHandle_t handle) {
  UError_t err = uSuccess;
  TextboxContext_t* ctx = (TextboxContext_t*)handle;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  size_t index = 0u;
  if (uSuccess == err) {
    for (; (index < BUILTIN_TEXTBOX_NUM) && (&builtinTextbox[index] != ctx); ++index);  //< 検索ループ
    if (BUILTIN_LINEBUF_NUM == index) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    {
      // linebufPool 解放
      for (TextboxLineBuffer_t* l = (TextboxLineBuffer_t*)LinkedList_PopFront(&ctx->linebufPool); NULL != l;
           l = (TextboxLineBuffer_t*)LinkedList_PopFront(&ctx->linebufPool)) {
        l->length = 0;
        size_t indL = 0u;
        for (; (indL < BUILTIN_LINEBUF_NUM) && (&builtinLinebuf[indL] != l); ++indL);  //< 検索ループ
        if (BUILTIN_LINEBUF_NUM != indL) {
          const size_t p = (indL >> 3);
          const size_t m = (indL & 0b111);
          const uint8_t mask = (1u << m);
          builtinLinebufs_status[p] &= ~mask;
        }
      }
      // linebuf 解放
      for (TextboxLineBuffer_t* l = (TextboxLineBuffer_t*)LinkedList_PopFront(&ctx->linebuf); NULL != l;
           l = (TextboxLineBuffer_t*)LinkedList_PopFront(&ctx->linebuf)) {
        l->length = 0;
        size_t indL = 0u;
        for (; (indL < BUILTIN_LINEBUF_NUM) && (&builtinLinebuf[indL] != l); ++indL);  //< 検索ループ
        if (BUILTIN_LINEBUF_NUM != indL) {
          const size_t p = (indL >> 3);
          const size_t m = (indL & 0b111);
          const uint8_t mask = (1u << m);
          builtinLinebufs_status[p] &= ~mask;
        }
      }
    }

    {
      size_t p = (index >> 3);
      size_t m = (index & 0b111);
      uint8_t mask = (1u << m);
      //LOG_D("builtinTextboxes_status[%d]: before [%02x]", p, builtinTextboxes_status[p]);
      builtinTextboxes_status[p] &= ~mask;
      //LOG_D("builtinTextboxes_status[%d]: after  [%02x]", p, builtinTextboxes_status[p]);
    }
  }
}

/**
 * 指定した行のデータを返す.
 * @param [in] handle : 操作対象
 * @param [out] pLength : データバイト数
 * @param [in] pos : 行番号(0 ～)
 * @return 行データ
 * @retval NULL以外 : 指定した行の行データ
 * @retval NULL : 指定した行が存在しない
 */
const void* Textbox_GetRow(TextboxHandle_t handle, size_t* pLength, const size_t pos) {
  UError_t err = uSuccess;
  TextboxContext_t* ctx = (TextboxContext_t*)handle;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == pLength) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (pos >= ctx->numRows) {
      err = uFailure;
    }
  }

  const void* ret = NULL;
  if (uSuccess == err) {
    const TextboxLineBuffer_t* l = (const TextboxLineBuffer_t*)LinkedList_Get(&ctx->linebuf, pos);
    if (NULL != l) {
      ret = l->ptr;
      *pLength = l->length;
    }
  }

  return ret;
}

/**
 * @brief 行数を返す
 * @param handle
 * @return
 */
size_t Textbox_CountRow(TextboxHandle_t handle) {
  UError_t err = uSuccess;
  TextboxContext_t* ctx = (TextboxContext_t*)handle;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  size_t ret = 0;
  if (uSuccess == err) {
    ret = LinkedList_Count(&ctx->linebuf);
  }

  return ret;
}

/**
 * データを追加する. 最大行に達している場合は, 先頭行を削除する.
 * @param handle
 * @param data
 * @param size
 * @return
 */
UError_t Textbox_Push(TextboxHandle_t handle, const void* data, size_t size) {
  UError_t err = uSuccess;
  TextboxContext_t* ctx = (TextboxContext_t*)handle;

  if (uSuccess == err) {
    if (NULL == ctx || NULL == data) {
      err = uFailure;
    }
  }

  TextboxLineBuffer_t* line = NULL;
  if (uSuccess == err) {
    line = (TextboxLineBuffer_t*)LinkedList_PopFront(&ctx->linebufPool);
    if (NULL == line) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    const size_t nbytecopy = (line->size > size) ? size : line->size;
    if (0 < nbytecopy) {
      memcpy(line->ptr, data, nbytecopy);
    }
    line->length = nbytecopy;
  }

  if (uSuccess == err) {
    err = LinkedList_PushBack(&ctx->linebuf, (LinkedList_t*)line);
    if (uSuccess != err) {
      // プールに戻す
      LinkedList_PushBack(&ctx->linebufPool, (LinkedList_t*)line);
    }
  }

  // 行数を超えた分削除
  if (uSuccess == err) {
    if (LinkedList_Count(&ctx->linebuf) > ctx->numRows) {
      TextboxLineBuffer_t* f = (TextboxLineBuffer_t*)LinkedList_PopFront(&ctx->linebuf);
      if (NULL != f) {
        err = LinkedList_PushBack(&ctx->linebufPool, (LinkedList_t*)f);
      }
      if (uSuccess != err) {
        LOG_W("LinkedList_PushBack(&ctx->linebufPool, f) failure");
      }
    }
  }

  return err;
}

/**
 * データを破棄する
 * @param handle
 * @return
 */
UError_t Textbox_Clear(TextboxHandle_t handle) {
  UError_t err = uSuccess;
  TextboxContext_t* ctx = (TextboxContext_t*)handle;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    TextboxLineBuffer_t* b = (TextboxLineBuffer_t*)LinkedList_PopFront(&ctx->linebuf);
    for (; NULL != b; b = (TextboxLineBuffer_t*)LinkedList_PopFront(&ctx->linebuf)) {
      b->length = 0;
      (void)LinkedList_PushBack(&ctx->linebufPool, (LinkedList_t*)b);
    }
  }
  return err;
}

/**
 * @brief Textbox モジュールの状態を出力. デバッグ用途
 * @param
 * @return
 */
UError_t Textbox_Diag(void) {
#if 0
#define BUILTIN_TEXTBOX_NUM (8)
#define BUILTIN_TEXTBOX_STATUS_NUM ((BUILTIN_TEXTBOX_NUM + 7) / 8)

static TextboxContext_t builtinTextbox[BUILTIN_TEXTBOX_NUM] = {0};
static uint8_t builtinTextboxes_status[BUILTIN_TEXTBOX_STATUS_NUM] = {0};
#endif

  {
    LOG_I("BUILTIN_TEXTBOX_NUM(%d)", BUILTIN_TEXTBOX_NUM);
    for (size_t i = 0; i < BUILTIN_TEXTBOX_NUM; ++i) {
      LOG_I("buitinTextbox[%2d]: 0x%08x", i, &builtinTextbox[i]);
    }
    LOG_I("BUILTIN_TEXTBOX_STATUS_NUM(%d)", BUILTIN_TEXTBOX_STATUS_NUM);
    for (size_t i = 0; i < BUILTIN_TEXTBOX_STATUS_NUM; ++i) {
      LOG_I("builtinTextboxes_status[%2d]: 0x%02x", i, builtinTextboxes_status[i]);
    }
  }

#if 0
#define BUILTIN_LINEBUF_NUM (32)
#define BUILTIN_LINEBUF_STATUS_NUM ((BUILTIN_LINEBUF_NUM + 7) / 8)

static TextboxLineBuffer_t builtinLinebuf[BUILTIN_LINEBUF_NUM] = {0};
static uint8_t builtinLinebufs_status[BUILTIN_LINEBUF_STATUS_NUM] = {0};
#endif
  {
    LOG_I("BUILTIN_LINEBUF_NUM(%d)", BUILTIN_LINEBUF_NUM);
    for (size_t i = 0; i < BUILTIN_LINEBUF_NUM; ++i) {
      LOG_I("builtinLinebuf[%2d]: 0x%08x", i, &builtinLinebuf[i]);
    }
    LOG_I("BUILTIN_LINEBUF_STATUS_NUM(%d)", BUILTIN_LINEBUF_STATUS_NUM);
    for (size_t i = 0; i < BUILTIN_LINEBUF_STATUS_NUM; ++i) {
      LOG_I("builtinLinebufs_status[%2d]: 0x%02x", i, builtinLinebufs_status[i]);
    }
  }
}
