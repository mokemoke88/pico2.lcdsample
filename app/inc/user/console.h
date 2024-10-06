/**
 * @file prog01/app/inc/user/console.h
 **/

#if !defined(USER_CONSOLE_H__)
#define USER_CONSOLE_H__

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <stddef.h>
#include <stdint.h>

#include <user/types.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////
typedef struct tagLinkList_t LinkList_t;

typedef struct tagLinkList_t {
  LinkList_t* prev;
  LinkList_t* next;
} LinkList_t;

typedef struct tagLineBuffer_t {
  LinkList_t link;
  // data
  size_t charsize;  // キャラクタサイズ
  size_t bytesize;  // 実データデータ長
  void* ptr;        // 実データの先頭アドレス
} LineBuffer_t;

typedef struct tagConsoleContext_t {
  size_t rows;
  size_t rowHeight;

  size_t blockSize;
  size_t heapSize;

  void* blockBuf;
  LineBuffer_t* lineBuf;

  LinkList_t lineBlockList;
  LinkList_t freeLineBlockList;
} ConsoleContext_t;

typedef void* ConsoleHandle_t;

typedef UError_t (*ConsoleRowProcessFn_t)(void* arg, const LineBuffer_t* buf, const size_t rowHeight);

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * @brief コンソールコンテキストを生成する
 * @param [inout] ctx : 生成結果出力先
 * @param [in] rows : コンソール表示行数
 * @param [in] rawHeight : 一行の高さ(単位: pixel)
 * @param [in] lineBuf : 使用するラインバッファ配列
 * @param [in] blockBuf : 使用するデータブロック 最低 blockSize * heapSize 必要
 * @param [in] blockSize : 1行あたりのデータブロックサイズ
 * @param [in] heapSize : ラインバッファヒープの個数. 最大行 * 1.5 あたりが目安
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t Console_Create(ConsoleContext_t* ctx, size_t rows, size_t rowHeight, LineBuffer_t* lineBuf, void* blockBuf, size_t blockSize, size_t heapSize);

/**
 * @brief コンソールに文字列リテラルを追加します
 * @param [in] handle : 操作対象のコンソールハンドル
 * @param [in] sz : 追加する文字列リテラル
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t Console_StringPush(ConsoleHandle_t handle, const char* sz);

/**
 * @brief コンソールの内容を先頭から処理します
 * @param [in] handle : 操作対象のコンソールハンドル
 * @param [in] fn : ラインバッファ処理関数
 * @param [in] arg : ラインバッファ処理関数パラメータパック
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t Console_Flush(ConsoleHandle_t handle, ConsoleRowProcessFn_t fn, void* arg);

#ifdef __cplusplus
}
#endif  // __cplusplus

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

#endif  // !defined(USER_CONSOLE_H__)
