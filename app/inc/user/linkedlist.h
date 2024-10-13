/**
 * @file prog01/app/inc/user/linkedlist.h
 */

#if !defined(USER_LINKEDLIST_H__)
#define USER_LINKEDLIST_H__

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/types.h>

#include <stddef.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

typedef struct tagLinkedList_t LinkedList_t;

struct tagLinkedList_t {
  LinkedList_t* prev;
  LinkedList_t* next;
};

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

/**
 * 末尾にノードを追加する
 * @param [in] list : 操作対象
 * @param [in] add : 追加するノード
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t LinkedList_PushBack(LinkedList_t* list, LinkedList_t* add);

/**
 * 先頭にノードを追加する
 * @param [in] list : 操作対象
 * @param [in] add : 追加するノード
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t LinkedList_PushFront(LinkedList_t* list, LinkedList_t* add);

/**
 * 要素数を返す
 * @param [in] list : 操作対象
 * @return 要素数
 */
size_t LinkedList_Count(LinkedList_t* list);

/**
 * 先頭からpos番目の要素を返す
 * @param [in] list : 操作対象
 * @param [in] pos : 位置
 * @return 指定した位置の要素
 * @retval NULL以外 : 指定した位置の要素
 * @retval NULL : 指定した位置に要素が存在しない
 */
const LinkedList_t* LinkedList_Get(LinkedList_t* list, const size_t pos);

/**
 * 先頭から要素を取り出す
 * @param [in] list : 操作対象
 * @return 取り出した要素
 * @retval NULL以外 : 取り出した要素
 * @retval NULL : 要素が存在しない
 */
LinkedList_t* LinkedList_PopFront(LinkedList_t* list);

/**
 * 末尾から要素を取り出す
 * @param [in] list : 操作対象
 * @return 取り出した要素
 * @retval NULL以外 : 取り出した要素
 * @retval NULL : 要素が存在しない
 */
LinkedList_t* LinkedList_PopBack(LinkedList_t* list);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

#endif  // USER_LINKEDLIST_H__
