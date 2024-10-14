/**
 * @file app/inc/user/touchdrv.h
 * Raspberry PI PICO C-SDK 向け
 * WAVEWARE-27579 2.0インチ静電容量タッチLCD(240x320) 向け I2C 接続タッチパッド
 * @date 2024.10.14 k.shibata newly created
 */

#if !defined(USER_TOUCHDRV_H__)
#define USER_TOUCHDRV_H__

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
typedef void* TouchDrvHandle_t;

/**
 * @brief タッチポイント情報
 */
typedef struct tagCST328DataCoord_t {
  int32_t x;         //< X座標
  int32_t y;         //< Y座標
  int32_t strength;  //< 強さ
} CST328DataCoord_t;

/**
 * タッチパッド情報
 */
typedef struct tagCST328Data_t {
  size_t points;  //< 通知ポイント数
  CST328DataCoord_t coords[5];
} CST328Data_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

/**
 * タッチパッド操作用のハンドルを取得する
 * @param [out] pHandle : ハンドル出力先
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t TouchDrv_Open(TouchDrvHandle_t* pHandle);

/**
 * タッチパッドを初期化する
 * @param [in] handle : 処理対象
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t TouchDrv_Init(TouchDrvHandle_t handle);

/**
 * タッチ情報を更新する
 *
 * タッチパッドデバイスから情報更新通知があった場合,
 * 内部情報を更新します.
 * @param [in] handle : 処理対象
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t TouchDrv_UpdateCoord(TouchDrvHandle_t handle);

/**
 * タッチパッド情報を取得する
 *
 * TouchDrv_UpdateCoord()で更新した情報を取得します.
 *
 * @param [in] handle : 処理対象
 * @param [out] pCoord : タッチパッド情報
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t TouchDrv_GetCoord(TouchDrvHandle_t handle, CST328Data_t* pCoord);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

#endif  //! defined(USER_TOUCHDRV_H__)
