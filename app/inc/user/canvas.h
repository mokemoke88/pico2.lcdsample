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

#include <stddef.h>
#include <stdint.h>

#include <user/types.h>

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

UError_t Canvas_Create(Canvas_t* ctx, uint32_t w, uint32_t h, size_t s, void* const buf);

const void* Canvas_GetBuf(const Canvas_t* const ctx);

UError_t Canvas_Clear(const Canvas_t* const ctx, const uint16_t c);

UError_t Canvas_Blt(const Canvas_t* const ctx, uint32_t dx, uint32_t dy, const uint16_t* src, uint32_t sx, uint32_t sy, uint32_t stride, uint32_t sw,
                    uint32_t sh);

UError_t Canvas_DrawPixel(const Canvas_t* const ctx, const uint32_t x, const uint32_t y, const uint16_t c);

UError_t Canvas_DrawLine(const Canvas_t* const ctx, const uint32_t x1, const uint32_t y1, const uint32_t x2, const uint32_t y2, const uint16_t c);

UError_t Canvas_DrawCircle(const Canvas_t* const ctx, const uint32_t x, const uint32_t y, const uint32_t r, const uint16_t c);

UError_t Canvas_DrawFillCircle(const Canvas_t* const ctx, const uint32_t x, const uint32_t y, const uint32_t r, const uint16_t c);

#ifdef __cplusplus
}
#endif  // __cplusplus

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

#endif  // !defined(USER_CANVAS_H__)
