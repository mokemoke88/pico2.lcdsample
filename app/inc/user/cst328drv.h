/**
 * @file app/inc/user/cst328drv.h
 * WAVEWARE-27579 2.0インチ静電容量タッチLCD(240x320) 向け I2C 接続タッチパッド
 * CST328 タッチコントローラ
 * https://www.buydisplay.com/download/ic/CST328_Datasheet.pdf
 * @date 2024.10.14 k.shibata newly created
 */

#if !defined(USER_CST328DRV_H__)
#define USER_CST328DRV_H__

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
typedef void* CST328DrvHandle_t;

/**
 * @brief タッチポイント情報
 */
typedef struct tagCST328DataCoord_t {
  uint8_t id;        //< fingerid
  uint8_t status;    //< finger status press(0x06) or lift
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
UError_t CST328Drv_Open(CST328DrvHandle_t* pHandle);

/**
 * タッチパッドを初期化する
 * @param [in] handle : 処理対象
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t CST328Drv_Init(CST328DrvHandle_t handle);

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
UError_t CST328Drv_UpdateCoord(CST328DrvHandle_t handle);

/**
 * タッチパッド情報を取得する
 *
 * CST328Drv_UpdateCoord()で更新した情報を取得します.
 *
 * @param [in] handle : 処理対象
 * @param [out] pCoord : タッチパッド情報
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t CST328Drv_GetCoord(CST328DrvHandle_t handle, CST328Data_t* pCoord);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

#endif  //! defined(USER_CST328DRV_H__)
