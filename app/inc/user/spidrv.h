/**
 * @file prog01/app/inc/user/spidrv.h
 * Raspberry PI PICO C-SDK 用 SPI ラッパドライバ
 **/

#if !defined(USER_SPIDRV_H__)
#define USER_SPIDRV_H__

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <stdint.h>

#include <hardware/spi.h>

#include <user/types.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

typedef void* SPIDrvHandle_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

UError_t SPIDrv_Open(SPIDrvHandle_t* handle);
void SPIDrv_Close(SPIDrvHandle_t handle);

UError_t SPIDrv_Init(SPIDrvHandle_t handle, uint32_t baudrate);
UError_t SPIDrv_SendByte(const SPIDrvHandle_t handle, const uint8_t data);
UError_t SPIDrv_RecvByte(const SPIDrvHandle_t handle, uint8_t* data);
UError_t SPIDrv_TransferByte(const SPIDrvHandle_t handle, const uint8_t tx, uint8_t* rx);

/**
 * @brief 同期データ送信を行う
 * @param [in] handle : 処理対象
 * @param [in] tx : 書き込みデータ
 * @param [in] size : 転送サイズ (単位:byte)
 * @return 処理結果
 * @retval uSuccess : 処理完了
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t SPIDrv_SendNBytes(const SPIDrvHandle_t handle, const void* data, size_t size);

/**
 * @brief 同期データ受信を行う
 * @param [in] handle : 処理対象
 * @param [out] rx : 読み出しデータ格納先
 * @param [in] size : 転送サイズ (単位:byte)
 * @return 処理結果
 * @retval uSuccess : 処理完了
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t SPIDrv_RecvNBytes(const SPIDrvHandle_t handle, void* data, size_t size);

/**
 * @brief 同期データ転送を行う
 * @param [in] handle : 処理対象
 * @param [in] tx : 書き込みデータ
 * @param [out] rx : 読み出しデータ格納先
 * @param [in] size : 転送サイズ (単位:byte)
 * @return 処理結果
 * @retval uSuccess : 処理完了
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t SPIDrv_TransferNBytes(SPIDrvHandle_t handle, const void* tx, void* rx, size_t size);

/**
 * @brief 非同期データ送信を行う
 * @param [inout] handle : 処理対象
 * @param [in] tx : 書き込みデータ
 * @param [in] size : 転送サイズ (単位:byte)
 * @return 処理結果
 * @retval uSuccess : 処理完了
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t SPIDrv_AsyncSend(SPIDrvHandle_t handle, const void* tx, size_t size);

/**
 * @brief 非同期データ受信を行う
 * @param [inout] handle : 処理対象
 * @param [out] rx : 読み出しデータ格納先
 * @param [in] size : 転送サイズ (単位:byte)
 * @return 処理結果
 * @retval uSuccess : 処理完了
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t SPIDrv_AsyncRecv(SPIDrvHandle_t handle, void* rx, size_t size);

/**
 * @brief 非同期データ転送を行う
 * @param [inout] handle : 操作対象
 * @param [in] tx : 書き込みデータ
 * @param [out] rx : 読み出しデータ格納先
 * @param [in] size : 転送サイズ (単位:byte)
 * @return 処理結果
 * @retval uSuccess : 処理完了
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t SPIDrv_AsyncTransfer(SPIDrvHandle_t handle, const void* tx, void* rx, size_t size);

/**
 * @brief 非同期転送の完了を待つ
 * @param [in] handle : 操作対象
 * @return 処理結果
 * @retval uSuccess : 処理完了
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t SPIDrv_WaitForAsync(SPIDrvHandle_t handle);

#ifdef __cplusplus
}
#endif  // __cplusplus

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

#endif  // !defined(USER_SPIDRV_H__)
