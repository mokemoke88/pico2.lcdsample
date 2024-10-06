/**
 * @file prog01/app/inc/user/font.h
 * FONTX2 形式のフォントを使用した文字列出力操作
 *
 **/

#if !defined(USER_FONT_H__)
#define USER_FONT_H__

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

/**
 * Font_Print() から呼び出される描画関数
 * @param [in] arg : パラメータパック
 * @param [in] x : x座標 (単位: キャラクタ)
 * @param [in] y : y座標 (単位: キャラクタ)
 * @param [in] graph : フォントグラフ
 * @param [in] c : 文字コード (JIS(7bit) or SJIS(16bit))
 * @param [in] fw : フォント幅(単位: pixel)
 * @param [in] fh : フォント高(単位: pixel)
 * @param [in] fsz : フォントデータサイズ
 * @return 処理結果
 * @retval uSuccess : 処理完了
 * @retval uSuccess 以外 : 処理失敗. Font_Print() は以降の処理を終了します.
 */
typedef UError_t (*Font_DrawFontFn_t)(void* arg, uint32_t x, uint32_t y, const void* graph, uint8_t c, uint32_t fw, uint32_t fh, size_t fsz);

/**
 * @brief ピクセル描画関数
 * @param [in] arg : パラメータパック
 * @param [in] x : 描画位置 x座標
 * @param [in] y : 描画位置 y座標
 * @param [in] color : カラーコード
 */
typedef void (*Font_DrawPixelFn_t)(void* arg, uint32_t x, uint32_t y, uint32_t color);

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * @brief 文字列リテラル sz を描画します.
 * @param [in] sz : 出力対象の文字列リテラル
 * @param [in] color : カラーコード
 * @param [in] fn : 描画関数
 *   @arg FONTX2形式のグラフ情報とキャラクタ座標(x,y) および
 * パラメータパックが渡されます.
 *   @arg 戻り値が uSuccess 以外の場合, 本関数は異常終了します.
 * @param [in] arg : 描画関数に引き渡すパラメータパック
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t Font_Print(const char* sz, uint32_t color, Font_DrawPixelFn_t fn, void* arg);

/**
 * @brief code に対する ANKフォント情報を直接取得します
 * @param [in] code : 対象の文字コード
 * @param [out] pw : フォント幅(dot) 出力先
 * @param [out] ph : フォント高(dot) 出力先
 * @param [out] pfsz : データサイズ 出力先
 * @return フォント情報データ
 * @retval NULL 以外 : 該当のフォントデータ
 * @retval NULL : 該当するフォント情報がない
 */
const uint8_t* Font_GetAnkFont(const uint16_t code, uint32_t* const pw, uint32_t* const ph, size_t* const pfsz);

/**
 * @brief code で指定した文字を fn関数を使用して描画します.
 * @param [in] code : 文字コード (SJIS)
 * @param [in] x : 描画位置x
 * @param [in] y : 描画位置y
 * @param [in] color : カラーコード
 * @param [out] pw : 出力フォント幅(pixel)
 * @param [out] ph : 出力フォント幅(pixel)
 * @param [in] fn : ピクセル描画関数
 * @param [in] arg : パラメータパック
 * @return 処理結果
 * @retval uSuccess : 処理成功
 * @retval uSuccess 以外 : 処理失敗
 */
UError_t Font_DrawChar(uint16_t code, uint32_t x, uint32_t y, uint32_t color, uint32_t* pw, uint32_t* ph, Font_DrawPixelFn_t fn, void* arg);

#ifdef __cplusplus
}
#endif  // __cplusplus

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

#endif  // !defined(USER_FONT_H__)
