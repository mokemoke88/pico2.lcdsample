/**
 * @file prog01/app/src/console.c
 * 簡易のコンソール出力. 25行保持
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

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

static uint8_t gConsoleDataHeap[50][256] = {0};
static LineBuffer_t gConsoleLineHeap[50] = {0};

static LinkList_t gConsoleLineBuffer = {
    .next = NULL,
    .prev = NULL,
};

static LinkList_t gConsoleFreeLineBuffers = {.next = NULL, .prev = NULL};

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

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

/**
 * @brief ラインバッファをスキャンする
 * @param base
 * @param fn
 * @return
 */
static UError_t console_LineBufferWalk(LinkList_t* const base, ConsoleProcessFn_t fn, void* arg) {
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
        err = fn(arg, cur);
      }
      if (uSuccess != err) {
        break;
      }
      cur = (LineBuffer_t*)cur->link.next;
    }  // while(NULL ...
  }  // if(uSuccess ...

  return err;
}

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

/**
 * @brief ラインバッファ未取得の場合は, ラインバッファを取得し, 2文字分の空白文字列を追加する.
 * @param [inout] ppBuf : ラインバッファを保持先, 内部でラインバッファを取得した場合は値を更新する
 * @param [in] freeList : ラインバッファ取得先
 * @return
 */
static UError_t console_pushLineBufferZenkakuSPC(LineBuffer_t** ppBuf, LinkList_t* freeList) {
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
      ((uint8_t*)(pBuf->ptr))[pBuf->bytesize] = 0x20;
      ((uint8_t*)(pBuf->ptr))[pBuf->bytesize] = 0x20;
      pBuf->bytesize += 2;
      pBuf->charsize += 2;
    }
  }
  return err;
}

UError_t Console_Create(void) {
  UError_t err = uSuccess;

  return console_setup();
}

/**
 * @brief コンソールバッファに文字列リテラルを追加します.
 * @param sz
 * @return
 */
UError_t Console_StringPush(const char* sz) {
  UError_t err = uSuccess;
  // LOG_D("entry");
  if (uSuccess == err) {
    if (NULL == sz) {
      err = uFailure;
    }
  }

  // 追加するラインバッファを取得
  LineBuffer_t* buf = NULL;
  if (uSuccess == err) {
    // 文字を追いかけながら, 改行 or 終端記号を見つけたら push

    for (size_t i = 0; 0 != *(sz + i); ++i) {
      const uint8_t c = (uint8_t)(*(sz + i));
      if (0x7f >= c) {
        if ((0x7f == c) || (0x08 == c)) {
          // 何もしない
        } else {
          // バッファが存在しない場合バッファ取得
          if (NULL == buf) {
            buf = console_LineBufferPop(&gConsoleFreeLineBuffers);
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
            ((uint8_t*)(buf->ptr))[buf->bytesize] = c;
            buf->bytesize++;
            buf->charsize++;
            // LOG_D("printable. done");
          } else if (0x09 == c) {
            // tab : 2キャラクタ進める
            ((uint8_t*)(buf->ptr))[buf->bytesize] = 0x20;
            ((uint8_t*)(buf->ptr))[buf->bytesize + 1] = 0x20;
            buf->bytesize += 2;
            buf->charsize += 2;
            // CR 改行
          } else if (0x10 == c) {
            // ラインバッファに投入
            console_LineBufferPush(&gConsoleLineBuffer, buf);
            if (25 > console_countLineBuffer(&gConsoleLineBuffer)) {
              LineBuffer_t* c = console_LineBufferPop(&gConsoleLineBuffer);
              if (c != NULL) {
                c->bytesize = 0;
                c->charsize = 0;
                console_LineBufferPush(&gConsoleFreeLineBuffers, c);
              }
            }
            buf = NULL;
          } else {
            // 上記以外 1キャラクタ進める
            ((uint8_t*)(buf->ptr))[buf->bytesize] = 0x20;
            buf->bytesize++;
            buf->charsize++;
          }  // 文字種別で処理分岐
        }  // if( (0x7...))
      } else if ((0xc2 <= c) && (0xdf >= c)) {
        // UTF8 2byte
        if (0 == *(sz + i + 1)) {
          break;
        }
        err = console_pushLineBufferZenkakuSPC(&buf, &gConsoleFreeLineBuffers);
        if (uSuccess != err) {
          break;
        }
      } else if ((0xe0 <= c) && (0xef >= c)) {
        // UTF8 3byte
        if ((0 == *(sz + i + 1)) || (0 == *(sz + i + 2))) {
          break;
        }
        err = console_pushLineBufferZenkakuSPC(&buf, &gConsoleFreeLineBuffers);
        if (uSuccess != err) {
          break;
        }
      } else if ((0xf0 <= c) && (0xf4 >= c)) {
        // UTF8 4byte
        if ((0 == *(sz + i + 1)) || (0 == *(sz + i + 2)) || (0 == *(sz + i + 3))) {
          break;
        }
        err = console_pushLineBufferZenkakuSPC(&buf, &gConsoleFreeLineBuffers);
        if (uSuccess != err) {
          break;
        }
      } else {
        // unknown = 2byte とする.
        if (0 == *(sz + i)) {
          break;
        }
        err = console_pushLineBufferZenkakuSPC(&buf, &gConsoleFreeLineBuffers);
        if (uSuccess != err) {
          break;
        }
      }
    }  // for(...

    if (NULL != buf) {
      console_LineBufferPush(&gConsoleLineBuffer, buf);
      //      LOG_D("push buffer");
      // 25行以上になっている場合は, バッファを解放し, フリーリストに戻す
      if (25 <= console_countLineBuffer(&gConsoleLineBuffer)) {
        //        LOG_D("over 25");
        LineBuffer_t* c = console_LineBufferPop(&gConsoleLineBuffer);
        if (c != NULL) {
          c->bytesize = 0;
          c->charsize = 0;
          console_LineBufferPush(&gConsoleFreeLineBuffers, c);
        }
      }
    }
  }  // if(uSuccess ..

  return err;
}

UError_t Console_Flush(ConsoleProcessFn_t fn, void* arg) {
  UError_t err = uSuccess;
  err = console_LineBufferWalk(&gConsoleLineBuffer, fn, arg);
  return err;
}
