/**
 * @file prog01/app/src/app/app.c
 *
 * @date 2024.10.27 k.shibata newly created
 */

//////////////////////////////////////////////////////////////////////////////
// includes
//////////////////////////////////////////////////////////////////////////////

#include <app/app.h>

#include <user/types.h>

#include <user/cst328drv.h>
#include <user/lcddrv.h>

#include <user/canvas.h>
#include <user/texture.h>

#include <user/audio.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <user/macros.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

// 音程定数
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

typedef struct tagCirclePointer_t {
  uint16_t x;
  uint16_t y;
  uint16_t r;
  uint8_t red;
  uint8_t green;
  uint8_t blue;

  bool active;
  uint16_t frame;
} CirclePointer_t;

typedef struct tagCirclePointerEvent_t {
  bool enable;
  uint16_t x;
  uint16_t y;
} CirclePointerEvent_t;

/**
 * @brief GUI部品 スライダー
 * 入力を受けて指示位置を変更する
 */
typedef struct tagSlider_t {
  uint16_t x;
  uint16_t y;
  uint16_t w;
  uint16_t h;
  uint8_t pos;
} Slider_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

#include "resources/wait_icon.h"  // 待機アイコン 192(48 x 4) x 48
#include "resources/wallpaper.h"  // 背景画像データ

static Texture_t texWait = {0};
static Texture_t texWallpaper = {0};

// スライダー
static Slider_t sliderP = {.x = 240 - 16 - 16, .y = 32, .w = 16, .h = 320 - 32 - 32, .pos = 127};
// 円ポインタ
static CirclePointer_t circleP = {.active = false, .red = 0xff, .green = 0x00, .blue = 0x00};

/**
 * @brief NCOリソースの定義.
 * TODO: グローバルではなく, AppNormalに関連付ける
 */
static NCO_t gNco[1];

/**
 * @brief EGO パラメータ定義
 */
static EGOParam_t gEgoParam = {
    .t_atk = 441 * 5,                                 // アタックタイム
    .d_atk = (65535 / 100 * 100) / (441 * 5),         // アタック増量 = ピーク / アタックタイム
    .t_decay = 441 * 3,                               // ディケイタイム
    .d_decay = ((65535 / 100 * 1) / (441 * 3)) * -1,  // ディケイ増量 = サスティーン音量 / ディケイタイム * -1
    .t_rel = 441 * 2,                                 // リリースタイム
    .d_rel = (65535 / 100 * 99) / (441 * 2) * -1,     // リリース増量 = サスティーン音量 / リリースタイム * -1
};

/**
 * @brief EGOリソースの定義
 * TODO: グローバルではなく, AppNormalに関連付ける
 */
static EGO_t gEgo[] = {{
    .param = &gEgoParam,
    .v = 0,
    .t = 0,
    .isRel = true,
}};

/**
 * @brief 譜面データ
 * 遠き山に日は落ちて
 */
static const uint32_t wd[] = {
    U16_TONE_E4, U16_TONE_G4, U16_TONE_G4, U16_TONE_E4, U16_TONE_D4, U16_TONE_C4, U16_TONE_D4, U16_TONE_E4, U16_TONE_G4, U16_TONE_E4, U16_TONE_D4,
    U16_TONE_E4, U16_TONE_G4, U16_TONE_G4, U16_TONE_E4, U16_TONE_D4, U16_TONE_C4, U16_TONE_D4, U16_TONE_E4, U16_TONE_D4, U16_TONE_C4, U16_TONE_C4,

    U16_TONE_A4, U16_TONE_C5, U16_TONE_C5, U16_TONE_B4, U16_TONE_G4, U16_TONE_A4, U16_TONE_A4, U16_TONE_C5, U16_TONE_B4, U16_TONE_G4, U16_TONE_A4,
    U16_TONE_A4, U16_TONE_C5, U16_TONE_C5, U16_TONE_B4, U16_TONE_G4, U16_TONE_A4, U16_TONE_A4, U16_TONE_C5, U16_TONE_B4, U16_TONE_G4, U16_TONE_A4,

    U16_TONE_E4, U16_TONE_G4, U16_TONE_G4, U16_TONE_E4, U16_TONE_D4, U16_TONE_C4, U16_TONE_D4, U16_TONE_E4, U16_TONE_G4, U16_TONE_E4, U16_TONE_D4,
    U16_TONE_E4, U16_TONE_G4, U16_TONE_G4, U16_TONE_C5, U16_TONE_D5, U16_TONE_E5, U16_TONE_D5, U16_TONE_C5, U16_TONE_D5, U16_TONE_A4, U16_TONE_C5,
};

/**
 * @brief 譜面データ
 * C4 を先頭にした譜面
 */
static const uint32_t U16_TONE[] = {
    U16_TONE_C4, U16_TONE_D4, U16_TONE_E4, U16_TONE_F4, U16_TONE_G4, U16_TONE_A4, U16_TONE_B4, U16_TONE_C5, U16_TONE_D5, U16_TONE_E5,
};

typedef struct tagMMD_t {
  uint32_t tone;
  uint32_t length;  // 長さ 44100 で全音符としようか?
} MMD_t;

static MMD_t fumen[] = {
    {U16_TONE_C4, 220500u}, {U16_TONE_D4, 220500u}, {U16_TONE_E4, 220500u}, {U16_TONE_F4, 220500u}, {U16_TONE_G4, 220500u},
};

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief サークルポインタのイベント処理
 * 活性時: 15フレームかけて半径を2づつ増加し, 15フレーム後 非活性状態となる
 * 非活性時: イベントが有効な場合, イベント位置を自身の位置とし活性状態に遷移する
 * @param [in] self : 操作対象
 * @param [in] event : イベント情報
 */
void CirclePointer_Event(CirclePointer_t* self, const CirclePointerEvent_t* event) {
  if (!self->active && event->enable) {
    self->x = event->x;
    self->y = event->y;
    self->frame = 0;
    self->r = 1;
    self->active = true;
  } else if (self->active) {
    self->frame++;
    if (16 > self->frame) {
      self->r += 2;
    } else {
      self->active = false;
    }
  }
}

/**
 * @brief サークルポインタの描画処理
 * 活性時: 描画する
 * 非活性時: 描画しない
 * @param [in] self : 操作対象
 * @param [in] canvas : 描画に使用するリソース
 */
void CirclePointer_Render(CirclePointer_t* self, const Canvas_t* canvas) {
  if (self->active) {
    Canvas_DrawCircle(canvas, self->x, self->y, self->r, RGB888toRGB565(self->red, self->green, self->blue));
  }
}

/**
 * @brief スライダーのイベント処理
 * 入力が自身の入力範囲内なら, 自身のポイント情報を更新する
 * @param self
 * @param absX
 * @param absY
 */
void Slider_Event(Slider_t* self, uint16_t absX, uint16_t absY) {
  // if(self->x > absX){
  //   return;
  // }

  if (self->y > absY) {
    return;
  }

  // if((self->x + self->w) < absX){
  //   return;
  // }

  if ((self->y + self->h) < absY) {
    return;
  }

  // 垂直スライダーとする
  self->pos = ((absY - self->y) << 7) / self->h;
}

/**
 * @brief スライダーのポイント情報を取得する
 * @param self
 * @return
 */
uint8_t Slider_GetPosition(Slider_t* self) {
  if (NULL == self) {
    return 0u;
  }
  return self->pos;
}

/**
 * @brief スライダーの描画処理
 * 描画位置に矩形枠とポイント位置を塗りつぶし円で描画する
 * @param self
 * @param canvas
 */
void Slider_Render(Slider_t* self, const Canvas_t* canvas) {
  if (NULL == self) {
    return;
  }

  uint32_t y = self->y + ((self->pos * self->h) >> 7);

  // ガイド表示
  Canvas_DrawLine(canvas, self->x, self->y, self->x, self->y + self->h, RGB888toRGB565(0, 0, 0xff));
  Canvas_DrawLine(canvas, self->x, self->y, self->x + self->w, self->y, RGB888toRGB565(0, 0, 0xff));
  Canvas_DrawLine(canvas, self->x + self->w, self->y, self->x + self->w, self->y + self->h, RGB888toRGB565(0, 0, 0xff));
  Canvas_DrawLine(canvas, self->x, self->y + self->h, self->x + self->w, self->y + self->h, RGB888toRGB565(0, 0, 0xff));

  Canvas_DrawFillCircle(canvas, self->x + (self->w / 2), y, 8, RGB888toRGB565(0xff, 0, 0));
}

#if 0
/**
 * @brief レンダリング処理
 * @param canvas
 * @param f
 * @return
 */
static UError_t Render(Canvas_t const* canvas, const uint32_t f, CirclePointer_t* circle) {
  UError_t err = uSuccess;
  if (NULL == canvas) {
    err = uFailure;
  }
  if (uSuccess == err) {
    // 罫線描画

#if 0
    Canvas_DrawLine(canvas, 0, 0, 239, 319, RGB888toRGB565(0xff, 0, 0));
    Canvas_DrawLine(canvas, 239, 0, 0, 319, RGB888toRGB565(0xff, 0, 0));

    Canvas_DrawLine(canvas, 0, 0, 0, 319, RGB888toRGB565(0xff, 0, 0));
    Canvas_DrawLine(canvas, 100, 0, 100, 319, RGB888toRGB565(0xff, 0, 0));
    Canvas_DrawLine(canvas, 200, 0, 200, 319, RGB888toRGB565(0xff, 0, 0));

    Canvas_DrawLine(canvas, 0, 0, 239, 0, RGB888toRGB565(0xff, 0, 0));
    Canvas_DrawLine(canvas, 0, 100, 239, 100, RGB888toRGB565(0xff, 0, 0));
    Canvas_DrawLine(canvas, 0, 200, 239, 200, RGB888toRGB565(0xff, 0, 0));
    Canvas_DrawLine(canvas, 0, 300, 239, 300, RGB888toRGB565(0xff, 0, 0));

    // サークル描画
    Canvas_DrawCircle(canvas, 100, 100, (f % 30) + 1, RGB888toRGB565(0, 0xff, 0));

    // サークル描画2
    Canvas_DrawFillCircle(canvas, 200, 200, (f % 20) + 1, RGB888toRGB565(0x0f + (f % 20) * 10, 0x0f + (f % 20) * 10, 0xff));
#endif

    CirclePointer_Render(circle, canvas);
    Slider_Render(&sliderP, canvas);
  }
  return err;
}
#endif

/**
 * @brief 通常状態を表現
 * @param app
 * @param arg
 * @return
 */
static UError_t AppNormal_(AppObject_t* app, const AppArg_t* arg) {
  UError_t err = uSuccess;
  static uint32_t framecnt = 0u;
  static size_t wf = 0;
  static bool audio_act = false;

  if (uSuccess == err) {
    if (NULL == app || NULL == arg || NULL == arg->frame || NULL == arg->touch) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    // 入力を処理

    // サークルポインタ処理
    CirclePointerEvent_t ev = {
        .enable = (arg->touch->points > 0),
        .x = arg->touch->coords[0].x,
        .y = arg->touch->coords[0].y,
    };
    CirclePointer_Event(&circleP, &ev);

    // スライダー処理
    if (arg->touch->points > 0) {
      Slider_Event(&sliderP, arg->touch->coords[0].x, arg->touch->coords[0].y);
    }

    // スライダの位置情報に合わせてLCDバックライトの輝度を更新
    if (NULL != arg->hLCD) {
      uint8_t b = Slider_GetPosition(&sliderP);  // 0 - 127 (0x0 - 0x7f) で値を返す
      LCDDrv_SetBrightness(arg->hLCD, ((uint32_t)0xff * b) >> 7);
    }

    if (arg->touch->points > 0) {
      wf = 0u;
      NCO_SetWeight(&gNco[0], wd[wf]);
      EGO_NoteOn(&gEgo[0]);
      framecnt = 0;
      audio_act = true;
    }
  }

  if (uSuccess == err) {
    // 画面更新
    Canvas_TextureBlt(arg->frame, 0, 0, &texWallpaper, 0, 0, 240, 320);  // 壁紙で塗りつぶし 所要時間:8ms程度

    CirclePointer_Render(&circleP, arg->frame);
    Slider_Render(&sliderP, arg->frame);
  }

  // 譜面再生
  if (uSuccess == err && true == audio_act) {
    static uint32_t wp = 0u;
    static uint32_t wlen = 15;  // 15 = 60秒, 四分音符とすると テンポ 60

    size_t remain = arg->audio_size;  // 出力残り

    for (size_t i = 0; i < remain; ++i) {
      int32_t tone = NCO_Get(&gNco[0]);  // -127 - 0 - + 127 が入っているとする
      int32_t env = EGO_Get(&gEgo[0]);   // 0 - 1 = (0 - 65536) とする.
      // エンベロープとNCOを掛け合わせ, サンプル値を決定
      arg->audio_buff[i] = 0xff & (127 + ((tone * env) >> 16));
    }

    wlen--;

    // 15フレーム = 1秒経過でトーン変更
    if (0 == wlen) {
      wf++;
      wf %= sizeof(wd) / sizeof(wd[0]);
      NCO_SetWeight(&gNco[0], wd[wf]);
      EGO_NoteOn(&gEgo[0]);
      wlen = 15;
      if (0 == wf) {
        audio_act = false;
      }
    }
  }

  framecnt++;
  // TODO: 遷移判定, 遷移

  return err;
}

/**
 * @brief 初期化中 状態を表現
 * @param app
 * @param arg
 * @return
 */
static UError_t AppInitial_(AppObject_t* app, const AppArg_t* arg) {
  UError_t err = uSuccess;
  static uint32_t f = 0;

  if (uSuccess == err) {
    if (NULL == app || NULL == arg || NULL == arg->frame) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    Canvas_Clear(arg->frame, RGB888toRGB565(0x00, 0x00, 0x00));  // クリア = 全画面書き換え 所要時間:8ms程度
    Canvas_TextureTransBlt(arg->frame, (240 / 2) - 24, (320 / 2) - 24, &texWait, 0 + (48 * ((f >> 3) % 4)), 0, 48, 48, 0x0);
  }

  // 30フレーム(2秒)経過後 に AppNormal_ へ遷移
  if (uSuccess == err) {
    f++;
    if (f >= (2 * 15)) {
      app->ActionFn = &AppNormal_;
    }
  }

  return err;
}

/**
 * @brief アプリケーションの状態を進行する
 * @param app
 * @param arg
 * @return
 */
UError_t AppUpdate(AppObject_t* app, const AppArg_t* arg) {
  UError_t err = uSuccess;
  if (uSuccess == err) {
    if (NULL == app || NULL == arg || NULL == app->ActionFn) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    err = app->ActionFn(app, arg);
  }

  return err;
}

/**
 * @brief 初期化
 * @param
 * @return
 */
AppObject_t AppInit(void) {
  Texture_Create(&texWallpaper, 240, 320, 480, &wallpaper[12]);
  Texture_Create(&texWait, 192, 48, 192 * 2, &wait_icon[12]);
  NCO_Create(&gNco[0]);
  EGOParam_Create(&gEgoParam, 65535, 441 * 5, 441 * 5, 65535 * 0.95, 441 * 5);

  AppObject_t app = {
      .ActionFn = &AppInitial_,
  };

  return app;
}
