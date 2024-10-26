/**
 * @file prog01/app/inc/user/canvas.h
 * 描画バッファ操作
 *
 * カラーフォーマット: RGB565
 **/

#if !defined(USER_CANVAS_H__)
#define USER_CANVAS_H__

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/types.h>

#include <user/texture.h>

#include <stddef.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

typedef struct tagCanvas_t {
  size_t w;
  size_t h;
  size_t s;
  void* buf;
} Canvas_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * 指定したメモリ位置を使用してCanvasオブジェクトを生成する
 * @param [out] ctx : 初期化する対象
 * @param [in] w : 幅 (単位:pixel)
 * @param [in] h : 高さ (単位:pixel)
 * @param [in] s : 幅バイト数 (単位:byte)
 * @param [in] buf : 割り当てるメモリ位置
 */
UError_t Canvas_Create(Canvas_t* ctx, uint16_t w, uint16_t h, size_t s, void* const buf);

/**
 * Canvasオブジェクトに関連付けたメモリアドレスを返す
 * @param [in] ctx : 処理対象
 * @return メモリ位置
 * @retval NULL : メモリアドレスが関連付けられていない または エラー
 * @retval NULL 以外 : 関連付けされたメモリアドレス
 */
const void* Canvas_GetBuf(const Canvas_t* ctx);

/**
 * 指定した色データで全体を塗りつぶす
 * @param [in] ctx : 処理対象
 * @param [in] c : 色データ RGB=565
 * @return 処理結果
 */
UError_t Canvas_Clear(const Canvas_t* ctx, const uint16_t c);

/**
 * 指定したメモリアドレスから始まる矩形データを 指定位置に転送する
 * @param [in] ctx : 処理対象
 * @param [in] dx : 転送先 x
 * @param [in] dy : 転送先 y
 * @param [in] src : 転送元アドレス
 * @param [in] sx : 転送元 x
 * @param [in] sy : 転送元 y
 * @param [in] stride : 転送元ラインバイト数
 * @param [in] sw : 転送幅
 * @param [in] sh : 転送高さ
 * @return 処理結果
 */
UError_t Canvas_Blt(const Canvas_t* ctx, uint16_t dx, uint16_t dy, const uint16_t* src, uint16_t sx, uint16_t sy, size_t stride, uint16_t sw, uint16_t sh);

/**
 * 指定したテクスチャから矩形データを 指定位置に転送する
 * @param [in] ctx : 処理対象
 * @param [in] dx : 転送先 x
 * @param [in] dy : 転送先 y
 * @param [in] tex : 転送元テクスチャオブジェクト
 * @param [in] sx : 転送元 x
 * @param [in] sy : 転送元 y
 * @param [in] sw : 転送幅
 * @param [in] sh : 転送高さ
 * @return 処理結果
 */
UError_t Canvas_TextureBlt(const Canvas_t* ctx, uint16_t dx, uint16_t dy, const Texture_t* tex, uint16_t sx, uint16_t sy, uint16_t sw, uint16_t sh);

/**
 * 指定したテクスチャから指定した色データ以外の矩形データを指定位置に転送する.
 * @param [in] ctx : 処理対象
 * @param [in] dx : 転送先 x
 * @param [in] dy : 転送先 y
 * @param [in] tex : 転送元テクスチャオブジェクト
 * @param [in] sx : 転送元 x
 * @param [in] sy : 転送元 y
 * @param [in] sw : 転送幅
 * @param [in] sh : 転送高さ
 * @param [in] trans : 転送除外色データ RGB565
 * @return 処理結果
 */
UError_t Canvas_TextureTransBlt(const Canvas_t* ctx, uint16_t dx, uint16_t dy, const Texture_t* tex, uint16_t sx, uint16_t sy, uint16_t sw, uint16_t sh,
                                uint16_t trans);

UError_t Canvas_DrawPixel(const Canvas_t* ctx, uint16_t x, uint16_t y, uint16_t c);

UError_t Canvas_DrawLine(const Canvas_t* ctx, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t c);

UError_t Canvas_DrawCircle(const Canvas_t* ctx, uint16_t x, uint16_t y, uint16_t r, uint16_t c);

UError_t Canvas_DrawFillCircle(const Canvas_t* ctx, uint16_t x, uint16_t y, uint16_t r, uint16_t c);

/**
 * 指定位置に文字を描画する
 * @param [in] canvas : 描画対象
 * @param [in] posx : 描画位置 x
 * @param [in] posy : 描画位置 y
 * @param [in] color : 描画色
 * @param [in] fgraph : フォント画像データ格納先
 * @param [in] fw : フォント幅(単位:pixel)
 * @param [in] fh : フォント高さ(単位:pixel)
 * @param [in] fsz : フォントデータサイズ
 * @return 処理結果
 */
UError_t Canvas_DrawFont(const Canvas_t* canvas, uint16_t posx, uint16_t posy, uint16_t color, const uint8_t* fgraph, uint16_t fw, uint16_t fh, size_t fsz);

#ifdef __cplusplus
}
#endif  // __cplusplus

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

#endif  // !defined(USER_CANVAS_H__)
