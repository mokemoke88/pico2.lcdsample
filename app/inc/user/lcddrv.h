/**
 * @file prog01/app/inc/user/lcddrv.h
 * Raspberry PI PICO C-SDK 向け
 * WAVESHARE-27579 2.8インチ静電容量タッチLCD(240x320) 向け SPI 接続LCDドライバ
 *
 * LCD コントローラ: ST7789T3 搭載
 * パネル: 240 x 320
 **/

#if !defined(USER_LCDDRV_H__)
#define USER_LCDDRV_H__

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <stdint.h>

#include <user/spidrv.h>
#include <user/types.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////
typedef struct tagLCDDrvContext_t {
  SPIDrvHandle_t spi;
  uint32_t dc;
  uint32_t rst;
  uint32_t bl;
  uint32_t pwmSlice;

  bool bBusy;
} LCDDrvContext_t;

typedef void* LCDDrvHandle_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * LCDDrvContext_t を初期化します
 * @param [out] ctx : 初期化対象
 * @param [in] spi : LCD制御に使用するSPIデバイス
 * @return 処理結果
 * @retval SUCCESS : 処理成功
 */
UError_t LCDDrv_Create(LCDDrvContext_t* ctx, SPIDrvHandle_t spi);

/**
 * @brief 使用するGPIO等, MCU側の周辺を初期化します
 * @param [in] lcd : 操作対象
 * @return 処理結果
 * @retval SUCCESS : 処理成功
 */
UError_t LCDDrv_Init(LCDDrvHandle_t handle);

/**
 * @brief LCDハードウェア(パネル側)を初期化します
 * @param [in] lcd : 操作対象
 * @return 処理結果
 * @retval SUCCESS : 処理成功
 */
UError_t LCDDrv_InitalizeHW(LCDDrvHandle_t handle);

/**
 * @brief データ転送を行う際の描画範囲を指定します.
 * @param [in] lcd : 操作対象
 * @param [in] x : x位置
 * @param [in] y : y位置
 * @param [in] width : 高さ
 * @param [in] height : 幅
 * @return 処理結果
 * @retval SUCCESS : 処理成功
 */
UError_t LCDDrv_SetWindow(LCDDrvHandle_t handle, const uint16_t x, const uint16_t y, const uint16_t width, const uint16_t height);

/**
 * @brief 指定した色で画面を塗りつぶします.
 * @param [in] lcd : 操作対象
 * @param [in] r : 赤
 * @param [in] g : 緑
 * @param [in] b : 青
 * @return 処理結果
 * @retval SUCCESS : 処理成功
 */
UError_t LCDDrv_Clear(LCDDrvHandle_t handle, const uint8_t r, const uint8_t g, const uint8_t b);

UError_t LCDDrv_SetBrightness(LCDDrvHandle_t handle, uint16_t b);

UError_t LCDDrv_SwapBuff(LCDDrvHandle_t handle, const void* frame, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

#ifdef __cplusplus
}
#endif  // __cplusplus

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

#endif  // !defined(USER_SPIDRV_H__)
