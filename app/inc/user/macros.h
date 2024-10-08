/**
 * @file prog01/app/inc/user/macros.h
 * 共通マクロ
 **/

#if !defined(USER_MACROS_H__)
#define USER_MACROS_H__

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <stddef.h>
#include <stdint.h>

#include <user/types.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

/**
 * RGB888 のカラーフォーマットを RGB565に変換します
 */
#define RGB888toRGB565(R, G, B) (uint16_t)(((((G & 0b11111100) << 3) | ((B & 0b11111000) >> 3)) << 8) | ((R & 0b11111000) | ((G & 0b11111100) >> 5)))

/**
 * 3サイクル待機します
 */
#define NOP3() asm volatile("nop \n nop \n nop")


//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

#endif  // !defined(USER_MACROS_H__)
