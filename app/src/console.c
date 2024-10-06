/**
 * @file prog01/app/src/console.c
 * 簡易のコンソール出力
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <user/console.h>
#include <user/macros.h>
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
 * @brief コンソールコンテキストを生成する
 * @param [inout] ctx : 生成するコンテキスト
 * @param [in] rows : コンソール表示行数
 * @param [in] rawHeight : 一行の高さ(単位: pixel)
 * @param [in] lineBuf
 * @param [in] blockBuf
 * @param [in] blockSize
 * @param [in] heapSize
 * @return
 */
static UError_t console_create(ConsoleContext_t* ctx, size_t rows, size_t rowHeight, LineBuffer_t* lineBuf, void* blockBuf, size_t blockSize, size_t heapSize);

/**
 * @brief ラインバッファリストの末尾にsrcを追加します.
 * @param [in] base : 追加対象のラインバッファリスト
 * @param [in] src : 追加するラインバッファ
 */
static void console_LineBufferPush(LinkList_t* const base, LineBuffer_t* src);

/**
 * @brief ラインバッファリストの先頭を取り出します
 * @param [in] base : 操作対象のラインバッファリスト
 * @return 取り出したラインバッファ
 * @retval NULL以外 : 有効なラインバッファ
 * @retval NULL : ラインバッファリストが空
 */
static LineBuffer_t* console_LineBufferPop(LinkList_t* const base);

/**
 * @brief ラインバッファリスト内のノード数を返します.
 * @param [in] base : 操作対象のラインバッファリスト
 * @return ノード数
 */
static size_t console_countLineBuffer(LinkList_t* const base);

/**
 * @brief ラインバッファリストを先頭から読み出し, 操作関数fnに引き渡します
 * @param [in] base : 操作対象
 * @param [in] rowHeight : 行高さ
 * @param [in] fn : ラインバッファ操作関数
 * @param [in] arg : ラインバッファ操作関数用パラメータパック
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
static UError_t console_LineBufferWalk(LinkList_t* const base, size_t rowHeight, ConsoleRowProcessFn_t fn, void* arg);

/**
 * @brief ラインバッファ未取得の場合は, ラインバッファを取得し, 2文字分の空白文字列を追加する.
 * @param [inout] ppBuf : ラインバッファを保持先, 内部でラインバッファを取得した場合は値を更新する
 * @param [in] freeList : ラインバッファ取得先
 * @param [in] blockSize : ラインバッファに格納可能な最大バイト数
 * @return
 */
static UError_t console_pushLineBufferZenkakuSPC(LineBuffer_t** ppBuf, LinkList_t* freeList, const size_t blockSize);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

static UError_t console_create(ConsoleContext_t* ctx, size_t rows, size_t rowHeight, LineBuffer_t* lineBuf, void* blockBuf, size_t blockSize, size_t heapSize) {
  UError_t err = uSuccess;
  if (uSuccess == err) {
    if (NULL == ctx || NULL == lineBuf || NULL == blockBuf || 0 == rows || 0 == rowHeight || 0 == blockSize || 0 == heapSize) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    ctx->rows = rows;
    ctx->rowHeight = rowHeight;
    ctx->lineBuf = lineBuf;
    ctx->blockBuf = blockBuf;
    ctx->heapSize = heapSize;    //< ラインバッファの総数
    ctx->blockSize = blockSize;  //< 1行に含める最大バイト数

    // フリーリストの初期化
    {
      ctx->freeLineBlockList.next = NULL;
      ctx->freeLineBlockList.prev = NULL;
      for (size_t i = 0; i < ctx->heapSize; ++i) {
        LineBuffer_t* cur = &ctx->lineBuf[i];
        cur->ptr = ((uint8_t*)ctx->blockBuf) + (ctx->blockSize * i);
        cur->charsize = 0u;
        cur->bytesize = 0u;
        console_LineBufferPush(&ctx->freeLineBlockList, cur);
      }
    }
    // 有効ラインバッファリストの初期化
    {
      ctx->lineBlockList.next = NULL;
      ctx->lineBlockList.prev = NULL;
    }
  }

  return err;
}

static void console_LineBufferPush(LinkList_t* const base, LineBuffer_t* src) {
  LinkList_t* cur = base;
  while (NULL != cur->next) {
    cur = (LinkList_t*)cur->next;
  }
  src->link.prev = cur;
  cur->next = (LinkList_t*)src;
  src->link.next = NULL;
}

static LineBuffer_t* console_LineBufferPop(LinkList_t* const base) {
  LinkList_t* cur = base;
  LinkList_t* next = cur->next;
  if (NULL != next) {
    cur->next = next->next;
    if (NULL != cur->next) {
      cur->next->prev = cur;
    }
    next->prev = NULL;
    next->next = NULL;
  }
  return (LineBuffer_t*)next;
}

static size_t console_countLineBuffer(LinkList_t* const base) {
  size_t ret = 0;
  if (NULL != base) {
    LinkList_t* cur = base;
    while (NULL != cur->next) {
      cur = cur->next;
      ret++;
    }
  }
  return ret;
}

static UError_t console_LineBufferWalk(LinkList_t* const base, size_t rowHeight, ConsoleRowProcessFn_t fn, void* arg) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == base) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    LineBuffer_t* cur = (LineBuffer_t*)base->next;
    //    LOG_D("cur[0x%08lx]", (uint32_t)cur);
    while (NULL != cur) {
      if (NULL != fn) {
        err = fn(arg, cur, rowHeight);
      }
      if (uSuccess != err) {
        break;
      }
      cur = (LineBuffer_t*)cur->link.next;
    }  // while(NULL ...
  }  // if(uSuccess ...

  return err;
}

#if 0
/**
 * @brief 内部構造を初期化する
 * @param
 * @return
 */
static UError_t console_setup(void) {
  // フリーリストの初期化
  {
    LinkList_t* cur = &gConsoleFreeLineBuffers;
    for (size_t i = 0; i < 50; ++i) {
      LineBuffer_t* cur = &gConsoleLineHeap[i];
      cur->ptr = &gConsoleDataHeap[i];
      cur->charsize = 0u;
      cur->bytesize = 0u;
      console_LineBufferPush(&gConsoleFreeLineBuffers, cur);
    }
  }
  // 本体の初期化
  {
    gConsoleLineBuffer.next = NULL;
    gConsoleLineBuffer.prev = NULL;
  }
}
#endif

static UError_t console_pushLineBufferZenkakuSPC(LineBuffer_t** ppBuf, LinkList_t* freeList, const size_t blockSize) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == ppBuf) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    LineBuffer_t* pBuf = *ppBuf;
    if (NULL == pBuf) {
      pBuf = console_LineBufferPop(freeList);
      if (NULL != pBuf) {
        pBuf->bytesize = 0;
        pBuf->charsize = 0;
        *ppBuf = pBuf;
      } else {
        err = uFailure;
      }
    }
    if (uSuccess == err) {
      if (blockSize > pBuf->bytesize) {
        ((uint8_t*)(pBuf->ptr))[pBuf->bytesize++] = 0x20;
        pBuf->charsize++;
      }
      if (blockSize > pBuf->bytesize) {
        ((uint8_t*)(pBuf->ptr))[pBuf->bytesize++] = 0x20;
        pBuf->charsize++;
      }
    }
  }
  return err;
}

UError_t Console_Create(ConsoleContext_t* ctx, size_t rows, size_t rowHeight, LineBuffer_t* lineBuf, void* blockBuf, size_t blockSize, size_t heapSize) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    err = console_create(ctx, rows, rowHeight, lineBuf, blockBuf, blockSize, heapSize);
  }
  return err;
}

UError_t Console_StringPush(ConsoleHandle_t handle, const char* sz) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == handle || NULL == sz) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    ConsoleContext_t* ctx = (ConsoleContext_t*)handle;

    // 追加するラインバッファを取得
    LineBuffer_t* buf = NULL;

    // 文字分割しながらラインバッファに追加する
    for (size_t i = 0; 0 != *(sz + i); ++i) {
      const uint8_t c = (uint8_t)(*(sz + i));
      if (0x7f >= c) {
        if ((0x7f == c) || (0x08 == c)) {
          // 何もしない
        } else {
          // バッファが存在しない場合バッファ取得
          if (NULL == buf) {
            buf = console_LineBufferPop(&ctx->freeLineBlockList);
            if (NULL == buf) {
              err = uFailure;
              break;
            }
            buf->bytesize = 0;
            buf->charsize = 0;
          }

          // 文字種別で処理分岐
          if ((0x20 <= c) && (0x7e >= c)) {
            // printable
            if (ctx->blockSize > buf->bytesize) {
              ((uint8_t*)(buf->ptr))[buf->bytesize] = c;
              buf->bytesize++;
              buf->charsize++;
            }
          } else if (0x09 == c) {
            // tab : 2キャラクタ進める
            if (ctx->blockSize > buf->bytesize) {
              ((uint8_t*)(buf->ptr))[buf->bytesize++] = 0x20;
              buf->charsize++;
            } else {
              // TODO
            }
            if (ctx->blockSize > buf->bytesize) {
              ((uint8_t*)(buf->ptr))[buf->bytesize++] = 0x20;
              buf->charsize++;
            } else {
              // TODO
            }
            // CR 改行
          } else if (0x10 == c) {
            // ラインバッファに投入
            console_LineBufferPush(&ctx->lineBlockList, buf);
            if (18 > console_countLineBuffer(&ctx->lineBlockList)) {
              LineBuffer_t* c = console_LineBufferPop(&ctx->lineBlockList);
              if (c != NULL) {
                c->bytesize = 0;
                c->charsize = 0;
                console_LineBufferPush(&ctx->freeLineBlockList, c);
              }
            }
            buf = NULL;
          } else {
            // 上記以外 1キャラクタ進める
            if (ctx->blockSize > buf->bytesize) {
              ((uint8_t*)(buf->ptr))[buf->bytesize++] = 0x20;
              buf->charsize++;
            } else {
              // todo.
            }
          }  // 文字種別で処理分岐
        }  // if( (0x7...))
      } else if ((0xc2 <= c) && (0xdf >= c)) {
        // UTF8 2byte
        if (0 == *(sz + i + 1)) {
          break;
        }
        err = console_pushLineBufferZenkakuSPC(&buf, &ctx->freeLineBlockList, ctx->blockSize);
        if (uSuccess != err) {
          break;
        }
      } else if ((0xe0 <= c) && (0xef >= c)) {
        // UTF8 3byte
        if ((0 == *(sz + i + 1)) || (0 == *(sz + i + 2))) {
          break;
        }
        err = console_pushLineBufferZenkakuSPC(&buf, &ctx->freeLineBlockList, ctx->blockSize);
        if (uSuccess != err) {
          break;
        }
      } else if ((0xf0 <= c) && (0xf4 >= c)) {
        // UTF8 4byte
        if ((0 == *(sz + i + 1)) || (0 == *(sz + i + 2)) || (0 == *(sz + i + 3))) {
          break;
        }
        err = console_pushLineBufferZenkakuSPC(&buf, &ctx->freeLineBlockList, ctx->blockSize);
        if (uSuccess != err) {
          break;
        }
      } else {
        // unknown = 2byte とする.
        if (0 == *(sz + i)) {
          break;
        }
        err = console_pushLineBufferZenkakuSPC(&buf, &ctx->freeLineBlockList, ctx->blockSize);
        if (uSuccess != err) {
          break;
        }
      }
    }  // for(...

    if (NULL != buf) {
      console_LineBufferPush(&ctx->lineBlockList, buf);
      //      LOG_D("push buffer");
      // 25行以上になっている場合は, バッファを解放し, フリーリストに戻す
      if (18 <= console_countLineBuffer(&ctx->lineBlockList)) {
        //        LOG_D("over 25");
        LineBuffer_t* c = console_LineBufferPop(&ctx->lineBlockList);
        if (c != NULL) {
          c->bytesize = 0;
          c->charsize = 0;
          console_LineBufferPush(&ctx->freeLineBlockList, c);
        }
      }
    }  // if (NULL ...)

  }  // if(uSuccess ..

  return err;
}

UError_t Console_Flush(ConsoleHandle_t handle, ConsoleRowProcessFn_t fn, void* arg) {
  UError_t err = uSuccess;
  if (uSuccess == err) {
    if (NULL == handle || NULL == fn) {
      err = uFailure;
    }
  }
  if (uSuccess == err) {
    ConsoleContext_t* ctx = (ConsoleContext_t*)handle;
    err = console_LineBufferWalk(&ctx->lineBlockList, ctx->rowHeight, fn, arg);
  }
  return err;
}
