/**
 * @file prog01/app/inc/user/textbox.h
 *
 * @date 2024.10.13 k.shibata newly created
 */

#if !defined(USER_TEXTBOX_H__)
#define USER_TEXTBOX_H__
//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/types.h>

#include <stddef.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

typedef void* TextboxHandle_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

/**
 * Textbox インスタンスを生成する
 * @param [out] pHandle : 生成したTextboxハンドル
 * @param [in] mem : メモリリソース
 * @param [in] size : メモリリソースのサイズ
 * @param [in] lineSize : 1ラインバッファのサイズ
 * @param [in] numLine : 出力に使用する行数
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗. データは追加されない.
 */
UError_t Textbox_Create(TextboxHandle_t* pHandle, void* mem, size_t size, size_t lineSize, size_t numLine);

/**
 * @brief 破棄する
 * @param [in] handle : 操作対象
 * @note 破棄したhandle に対する操作は 動作不定とする.
 */
void Textbox_Destroy(TextboxHandle_t handle);

/**
 * 指定した行のデータを返す.
 * @param [in] handle : 操作対象
 * @param [out] pLength : データバイト数
 * @param [in] pos : 行番号(0 ～)
 * @return 行データ
 * @retval NULL以外 : 指定した行の行データ
 * @retval NULL : 指定した行が存在しない
 */
const void* Textbox_GetRow(TextboxHandle_t handle, size_t* pLength, const size_t pos);

/**
 * @brief 行数を返す
 * @param [in] handle : 操作対象
 * @return 有効な行数
 */
size_t Textbox_CountRow(TextboxHandle_t handle);

/**
 * データを追加する. 最大行に達している場合は, 先頭行を削除する.
 * @param [in] handle : 操作対象
 * @param [in] data : 追加するデータ
 * @param [in] size : 追加するデータのバイト数
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗. データは追加されない.
 */
UError_t Textbox_Push(TextboxHandle_t handle, const void* data, size_t size);

/**
 * データを破棄する
 * @param [in] handle : 操作対象
 * @return
 */
UError_t Textbox_Clear(TextboxHandle_t handle);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

#endif  // USER_TEXTBOX_H__
