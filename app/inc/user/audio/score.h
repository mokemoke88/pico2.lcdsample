/**
 * @file inc/user/audio/score.h
 *
 * @date 2024.11.10 k.shibata newly created
 */

#if !defined(USER_AUDIO_SCORE_H__)
#define USER_AUDIO_SCORE_H__

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

typedef struct tagAudioNote_t {
  uint32_t tone;    //< 0: 休符. 1 以上: 音階周波数
  uint32_t length;  //< 長さ(単位: サンプル)
} AudioNote_t;

typedef struct tagAudioScore_t {
  const AudioNote_t* notes;

  uint32_t scorePos;
  uint32_t notePos;
  uint32_t numNotes;

} AudioScore_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief 譜面情報を生成します
 * @param [out] score : 出力先
 * @param [in] notes : 音符配列へのポインタ
 * @param [in] numNotes : 音符の数
 * @return 処理結果
 */
UError_t AudioScore_Create(AudioScore_t* score, const AudioNote_t* notes, uint32_t numNotes);

/**
 * @brief 譜面の動作状態を初期状態にします
 * @param [in] score : 操作対象
 * @return
 */
UError_t AudioScore_Init(AudioScore_t* score);

/**
 * @brief 譜面を進行します.
 * @param [in] score : 操作対象
 * @return
 */
UError_t AudioScore_Step(AudioScore_t* score);

/**
 * @brief 譜面の終了状態を取得します.
 * @param score
 * @return
 */
bool AudioScore_IsFinished(const AudioScore_t* score);

/**
 * @brief 音符の変更(=譜面の進行)を取得します
 * @param score
 * @return
 */
bool AudioScore_IsNoteChanged(const AudioScore_t* score);

/**
 * @brief 現在の音符情報を取得します.
 * @param [in] score
 * @param [out] note
 * @return
 */
UError_t AudioScore_GetNote(const AudioScore_t* score, AudioNote_t* note);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

#endif  // USER_AUDIO_SCORE_H__
