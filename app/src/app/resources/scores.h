/**
 * @file prog01/app/src/app/resources/scores.h
 *
 * 譜面リソース
 *
 * @date 2024.11.10 k.shibata newly created
 */

#if !defined(RESOURCES_SCORE_H__)
#define RESOURCES_SCORE_H__

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////
#include <user/audio/score.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

#define AUDIO_SAMPLE_HZ (44100U)                   //< サンプルレート
#define AUDIO_1MIN_SAMPLE (AUDIO_SAMPLE_HZ * 60U)  //< 1分間のサンプル数
#define AUDIO_TEMPO (88U)                          //< テンポ TODO: AudioScore側で可変したい

// 四分音符の長さを定義
#define QUAT_TLEN (AUDIO_1MIN_SAMPLE / AUDIO_TEMPO)

#if 0
// 音程定数
// [サインテーブルの大きさ] * [(音程周波数 / サンプリング周波数)] * [固定少数点長(=65536)]
#define TONE_C4 (8192 * 261.626 / 44100)  // ど
#define TONE_D4 (8192 * 293.665 / 44100)  // れ
#define TONE_E4 (8192 * 329.628 / 44100)  // み
#define TONE_F4 (8192 * 349.228 / 44100)  // ふぁ
#define TONE_G4 (8192 * 391.995 / 44100)  // そ
#define TONE_A4 (8192 * 440.0 / 44100)    // ら
#define TONE_B4 (8192 * 493.883 / 44100)  // し
#define TONE_C5 (8192 * 523.251 / 44100)  // ど
#define TONE_D5 (8192 * 587.330 / 44100)  // れ
#define TONE_E5 (8192 * 659.255 / 44100)  // み
#endif

// 16bit 固定少数の音程
#define U16_TONE_C4 (3185020U)
#define U16_TONE_D4 (3575061U)
#define U16_TONE_E4 (4012872U)
#define U16_TONE_F4 (4251481U)
#define U16_TONE_G4 (4772125U)
#define U16_TONE_A4 (5356535U)
#define U16_TONE_B4 (6012503U)
#define U16_TONE_C5 (6370028U)
#define U16_TONE_D5 (7150122U)
#define U16_TONE_E5 (8025733U)

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

// 譜面データ: 遠き山に日は落ちて
static const AudioNote_t Notes0[] = {

    {U16_TONE_E4, QUAT_TLEN * 1.5},
    {U16_TONE_G4, QUAT_TLEN * 0.5},
    {U16_TONE_G4, QUAT_TLEN * 2.0},

    {U16_TONE_E4, QUAT_TLEN * 1.5},
    {U16_TONE_D4, QUAT_TLEN * 0.5},
    {U16_TONE_C4, QUAT_TLEN * 2.0},

    {U16_TONE_D4, QUAT_TLEN * 1.5},
    {U16_TONE_E4, QUAT_TLEN * 0.5},
    {U16_TONE_G4, QUAT_TLEN * 1.5},
    {U16_TONE_E4, QUAT_TLEN * 0.5},

    {U16_TONE_D4, QUAT_TLEN * 3.0},
    {0, QUAT_TLEN * 1.0},

    {U16_TONE_E4, QUAT_TLEN * 1.5},
    {U16_TONE_G4, QUAT_TLEN * 0.5},
    {U16_TONE_G4, QUAT_TLEN * 2.0},

    {U16_TONE_E4, QUAT_TLEN * 1.5},
    {U16_TONE_D4, QUAT_TLEN * 0.5},
    {U16_TONE_C4, QUAT_TLEN * 2.0},

    {U16_TONE_D4, QUAT_TLEN * 1.5},
    {U16_TONE_E4, QUAT_TLEN * 0.5},
    {U16_TONE_D4, QUAT_TLEN * 1.5},
    {U16_TONE_C4, QUAT_TLEN * 0.5},

    {U16_TONE_C4, QUAT_TLEN * 3.0},
    {0, QUAT_TLEN * 1.0},

    {U16_TONE_A4, QUAT_TLEN * 1.5},
    {U16_TONE_C5, QUAT_TLEN * 0.5},
    {U16_TONE_C5, QUAT_TLEN * 2.0},

    {U16_TONE_B4, QUAT_TLEN * 1.0},
    {U16_TONE_G4, QUAT_TLEN * 1.0},
    {U16_TONE_A4, QUAT_TLEN * 2.0},

    {U16_TONE_A4, QUAT_TLEN * 1.0},
    {U16_TONE_C5, QUAT_TLEN * 1.0},
    {U16_TONE_B4, QUAT_TLEN * 1.0},
    {U16_TONE_G4, QUAT_TLEN * 1.0},

    {U16_TONE_A4, QUAT_TLEN * 3.0},
    {0, QUAT_TLEN * 1.0},

    {U16_TONE_A4, QUAT_TLEN * 1.5},
    {U16_TONE_C5, QUAT_TLEN * 0.5},
    {U16_TONE_C5, QUAT_TLEN * 2.0},

    {U16_TONE_B4, QUAT_TLEN * 1.0},
    {U16_TONE_G4, QUAT_TLEN * 1.0},
    {U16_TONE_A4, QUAT_TLEN * 2.0},

    {U16_TONE_A4, QUAT_TLEN * 1.0},
    {U16_TONE_C5, QUAT_TLEN * 1.0},
    {U16_TONE_B4, QUAT_TLEN * 1.0},
    {U16_TONE_G4, QUAT_TLEN * 1.0},

    {U16_TONE_A4, QUAT_TLEN * 3.0},
    {0, QUAT_TLEN * 1.0},

    {U16_TONE_E4, QUAT_TLEN * 1.5},
    {U16_TONE_G4, QUAT_TLEN * 0.5},
    {U16_TONE_G4, QUAT_TLEN * 2.0},

    {U16_TONE_E4, QUAT_TLEN * 1.5},
    {U16_TONE_D4, QUAT_TLEN * 0.5},
    {U16_TONE_C4, QUAT_TLEN * 2.0},

    {U16_TONE_D4, QUAT_TLEN * 1.5},
    {U16_TONE_E4, QUAT_TLEN * 0.5},
    {U16_TONE_G4, QUAT_TLEN * 1.5},
    {U16_TONE_E4, QUAT_TLEN * 0.5},

    {U16_TONE_D4, QUAT_TLEN * 4.0},

    {U16_TONE_E4, QUAT_TLEN * 1.5},
    {U16_TONE_G4, QUAT_TLEN * 0.5},
    {U16_TONE_G4, QUAT_TLEN * 2.0},

    {U16_TONE_C5, QUAT_TLEN * 1.5},
    {U16_TONE_D5, QUAT_TLEN * 0.5},
    {U16_TONE_E5, QUAT_TLEN * 2.0},

    {U16_TONE_D5, QUAT_TLEN * 1.5},
    {U16_TONE_C5, QUAT_TLEN * 0.5},
    {U16_TONE_D5, QUAT_TLEN * 1.0},
    {U16_TONE_A4, QUAT_TLEN * 1.0},

    {U16_TONE_C5, QUAT_TLEN * 4.0},

    {0, QUAT_TLEN * 4.0}

};

static const AudioNote_t Notes1[] = {

    {U16_TONE_C4, QUAT_TLEN * 1.0}, {U16_TONE_C4, QUAT_TLEN * 1.0}, {U16_TONE_G4, QUAT_TLEN * 1.0}, {U16_TONE_G4, QUAT_TLEN * 1.0},

    {U16_TONE_A4, QUAT_TLEN * 1.0}, {U16_TONE_A4, QUAT_TLEN * 1.0}, {U16_TONE_G4, QUAT_TLEN * 2.0},

    {U16_TONE_F4, QUAT_TLEN * 1.0}, {U16_TONE_F4, QUAT_TLEN * 1.0}, {U16_TONE_E4, QUAT_TLEN * 1.0}, {U16_TONE_E4, QUAT_TLEN * 1.0},

    {U16_TONE_D4, QUAT_TLEN * 1.0}, {U16_TONE_D4, QUAT_TLEN * 1.0}, {U16_TONE_C4, QUAT_TLEN * 2.0},

    {U16_TONE_G4, QUAT_TLEN * 1.0}, {U16_TONE_G4, QUAT_TLEN * 1.0}, {U16_TONE_F4, QUAT_TLEN * 1.0}, {U16_TONE_F4, QUAT_TLEN * 1.0},

    {U16_TONE_E4, QUAT_TLEN * 1.0}, {U16_TONE_E4, QUAT_TLEN * 1.0}, {U16_TONE_D4, QUAT_TLEN * 2.0},

    {U16_TONE_G4, QUAT_TLEN * 1.0}, {U16_TONE_G4, QUAT_TLEN * 1.0}, {U16_TONE_F4, QUAT_TLEN * 1.0}, {U16_TONE_F4, QUAT_TLEN * 1.0},

    {U16_TONE_E4, QUAT_TLEN * 1.0}, {U16_TONE_E4, QUAT_TLEN * 1.0}, {U16_TONE_D4, QUAT_TLEN * 2.0},

    {U16_TONE_C4, QUAT_TLEN * 1.0}, {U16_TONE_C4, QUAT_TLEN * 1.0}, {U16_TONE_G4, QUAT_TLEN * 1.0}, {U16_TONE_G4, QUAT_TLEN * 1.0},

    {U16_TONE_A4, QUAT_TLEN * 1.0}, {U16_TONE_A4, QUAT_TLEN * 1.0}, {U16_TONE_G4, QUAT_TLEN * 2.0},

    {U16_TONE_F4, QUAT_TLEN * 1.0}, {U16_TONE_F4, QUAT_TLEN * 1.0}, {U16_TONE_E4, QUAT_TLEN * 1.0}, {U16_TONE_E4, QUAT_TLEN * 1.0},

    {U16_TONE_D4, QUAT_TLEN * 1.0}, {U16_TONE_D4, QUAT_TLEN * 1.0}, {U16_TONE_C4, QUAT_TLEN * 2.0},

    {0, QUAT_TLEN * 4.0},
};

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

#endif  // RESOURCES_SCORE_H__
