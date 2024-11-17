/**
 * @file user/audio/audiocontext.h
 *
 * @date 2024.11.17 k.shibata newly created
 */

#if !defined(USER_AUDIO_AUDIOCONTEXT_H__)
#define USER_AUDIO_AUDIOCONTEXT_H__

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <user/audio/ego.h>
#include <user/audio/nco.h>
#include <user/audio/score.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief オーディオコンテキスト
 * オーディオ 譜面, オシレータ, エンベロープ等 出力全般の情報を管理
 */
typedef struct tagAudioContext_t {
  bool enable;             //< オーディオ有効
  NCO_t* ncos;             //< NCO配列へのポインタ
  size_t numNco;           //< NCO要素数
  EGO_t* egos;             //< EGO配列のポインタ
  size_t numEgo;           //< EGO要素数
                           //  AudioScore_t* scores;                                                                          //< スコア配列のポインタ
                           //  size_t numScore;                                                                               //< スコア要素数
  AudioScore_t* curScore;  //< 現在選択しているスコア
  // ハンドラ
  void (*noteOffFn)(uint32_t, EGO_t*, NCO_t*);  //< ノートOFF時の処理
  void (*noteOnFn)(uint32_t, EGO_t*, NCO_t*);   //< ノートON時の処理
  uint8_t (*outputFn)(EGO_t*, NCO_t*);          //< 音声データ出力処理
} AudioContext_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif  //__cplusplus

/**
 * @brief 譜面情報を設定します
 * @param [in] ctx : 対象のAudioContext_t
 * @param [in] pScore : 設定するAudioScore_t インスタンス
 * @return 処理結果
 */
UError_t AudioContext_SetScore(AudioContext_t* ctx, AudioScore_t* pScore);

/**
 * @brief ctxを使用して dst に音声データを出力します
 * @param [in] ctx : 対象のAudioContext_t
 * @param [out] dst : 出力先
 * @param [in] size : 出力可能バイト数
 * @return 出力したバイト数
 */
size_t AudioContext_Write(AudioContext_t* ctx, void* dst, size_t const size);

#ifdef __cplusplus
}
#endif  //__cplusplus

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

#endif  // USER_AUDIO_AUDIOCONTEXT_H__
