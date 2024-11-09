/**
 * @file prog01/app/inc/user/audio/ego.h
 *
 * エンベロープジェネレータ
 *
 * @date 2024.11.04 k.shibata newly created
 */

#if !defined(USER_AUDIO_EGO_H__)
#define USER_AUDIO_EGO_H__

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <user/types.h>

#include <stdbool.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

#if 0
エンベロープパラメータ

     l1
      +    l3
     / \   +
    /   \ / \
   /     +   \
  /     l2    \
-+             +---
 l4            l4

// r1 = (l1 - l4) / t1
// r2 = (l2 - l1) / t2
// r3 = (l3 - l2) / t3
// r4 = (l4 - l3) / t4

// state : cond
// t1 : v >= l1
// t2 : r2 < 0 ?
//        v <= l2
//      r2 > 0 ?
//        v >= l2
//      r2 == 0
// t3 : r3 < 0 ?
//      v <= l3
//      r3 > 0 ?
//      v >= l3
//      r3 == 0
// t4 : v <= l4

#endif

/**
 * @brief エンベロープジェネレータパラメータ構造体
 */
typedef struct tagEGOParam_t {
  int32_t l1;
  int32_t l2;
  int32_t l3;
  int32_t l4;
  int32_t r1;
  int32_t r2;
  int32_t r3;
  int32_t r4;
} EGOParam_t;

typedef enum tagEGO_State_t {
  EGO_STATE_T1,
  EGO_STATE_T2,
  EGO_STATE_T3,
  EGO_STATE_T4,
  EGO_STATE_KEEP,
  EGO_STATE_FINISH,
} EGO_State_t;
/**
 * @brief エンベロープジェネレータオブジェクト
 */
typedef struct tagEGO_t {
  const EGOParam_t* param;  //< パラメータ
  EGO_State_t state;        //< 状態
  int32_t v;                //< 出力値
} EGO_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif  //__cplusplus

/**
 * @brief エンベロープジェネレータのパラメータを出力します.
 * ピーク音量(0-65535), アタックタイム(単位:サンプル), ディケイタイム(単位:サンプル), サスティーン音量(0-65535)
 * リリースタイム(単位:サンプル) を入力として パラメータを出力します
 * @param [out] param : パラメータ出力先
 * @param [in] peak : ピーク音量 (0-65536) 最大音量(0-65536 = 0.0 ... 1.0)
 * @param [in] atk : アタック時間 キーオンからピーク音量までの到達時間(単位: サンプル)
 * @param [in] decay : アタック時間到達後, 持続音量(サスティーン音量) までの到達時間(単位:サンプル)
 * @param [in] sus : 持続音量 (0-65536) (0-65536 = 0.0 ... 1.0)
 * @param [in] rel : キーオフで, サスティーン音量から音量が0になるまでの時間(単位:サンプル)
 * @return 処理結果
 */
UError_t EGOParam_Create(EGOParam_t* param, int32_t peak, int32_t atk, int32_t decay, int32_t sus, int32_t rel);

/**
 * @brief 現在値を取得する
 * 現在値を返し, 内部状態を1サンプル進めます
 * @param [in] ctx : 処理対象
 * @return 現在地
 * @retval 正常終了時: EGOの現在値
 * @retval 異常時: 0u
 */
uint32_t EGO_Get(EGO_t* ctx);

/**
 * @brief 発音状態に遷移します
 * @param [in] ctx : 処理対象
 */
void EGO_NoteOn(EGO_t* ctx);

/**
 * @brief リリース状態に遷移します
 * @param [in] ctx : 処理対象
 * @return なし
 */
void EGO_NoteOff(EGO_t* ctx);

#ifdef __cplusplus
}
#endif  //__cplusplus

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

#endif  // USER_AUDIO_EGO_H__
