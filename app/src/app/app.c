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

#include <stdio.h>

#include <user/macros.h>

//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief 矩形表現
 */
typedef struct tagRect_t {
  int32_t x;
  int32_t y;
  int32_t w;
  int32_t h;
} Rect_t;

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

/**
 * @brief 位置イベント条件
 */
typedef struct tagHitEvent_t {
  Rect_t hitbox;                         //< 範囲条件, この範囲に入力が入った際にイベントハンドラを実行
  UError_t (*doEvent)(const void* arg);  //< イベントハンドラ
} HitEvent_t;

//////////////////////////////////////////////////////////////////////////////
// prototype
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief サークルポインタのイベント処理
 * 活性時: 15フレームかけて半径を2づつ増加し, 15フレーム後 非活性状態となる
 * 非活性時: イベントが有効な場合, イベント位置を自身の位置とし活性状態に遷移する
 * @param [in] self : 操作対象
 * @param [in] event : イベント情報
 */
void CirclePointer_Event(CirclePointer_t* self, const CirclePointerEvent_t* event);

/**
 * @brief サークルポインタの描画処理
 * 活性時: 描画する
 * 非活性時: 描画しない
 * @param [in] self : 操作対象
 * @param [in] canvas : 描画に使用するリソース
 */
void CirclePointer_Render(CirclePointer_t* self, const Canvas_t* canvas);

/**
 * @brief スライダーのイベント処理
 * 入力が自身の入力範囲内なら, 自身のポイント情報を更新する
 * @param self
 * @param absX
 * @param absY
 */
void Slider_Event(Slider_t* self, uint16_t absX, uint16_t absY);

/**
 * @brief スライダーのポイント情報を取得する
 * @param self
 * @return
 */
uint8_t Slider_GetPosition(Slider_t* self);

/**
 * @brief スライダーの描画処理
 * 描画位置に矩形枠とポイント位置を塗りつぶし円で描画する
 * @param self
 * @param canvas
 */
void Slider_Render(Slider_t* self, const Canvas_t* canvas);

/**
 * @brief 位置入力 イベントハンドラ
 * @param [in] arg : 引数
 * @return 処理結果
 */
UError_t regionEvent_Top(const void* arg);

/**
 * @brief 位置入力 イベントハンドラ
 * @param [in] arg : 引数
 * @return 処理結果
 */
UError_t regionEvent_Middle(const void* arg);

/**
 * @brief 入力位置に応じた範囲イベントを実行します
 * @param [in] x : 入力座標
 * @param [in] y : 入力座標
 * @param [in] eventArg : イベントハンドラへの引数
 * @param [in] events : イベントハンドラ配列
 * @param [in] numEvents : イベントハンドラ数
 * @return 処理結果
 */
UError_t doRegionEvent(int32_t const x, int32_t const y, const void* eventArg, const HitEvent_t events[], size_t const numEvents);

// 音声データ処理ハンドラ

static void noteOffHandler(uint32_t tone, EGO_t* egos, NCO_t* ncos);
static void noteOnHandler(uint32_t tone, EGO_t* egos, NCO_t* ncos);
static uint8_t outputHandler(EGO_t* egos, NCO_t* ncos);

//////////////////////////////////////////////////////////////////////////////
// variable
//////////////////////////////////////////////////////////////////////////////

#include "resources/scores.h"     // 譜面データを取り込み
#include "resources/wait_icon.h"  // 待機アイコン 192(48 x 4) x 48
#include "resources/wallpaper.h"  // 背景画像データ

static Texture_t texWait = {0};
static Texture_t texWallpaper = {0};

// スライダー
static Slider_t sliderP = {.x = 240 - 16 - 16, .y = 32, .w = 16, .h = 320 - 32 - 32, .pos = 127};

// 円ポインタ
static CirclePointer_t circleP = {.active = false, .red = 0xff, .green = 0x00, .blue = 0x00};

/**
 * @brief EGO パラメータ定義
 */
static EGOParam_t gEgoParam = {.l1 = 0, .l2 = 0, .l3 = 0, .l4 = 0, .r1 = 0, .r2 = 0, .r3 = 0, .r4 = 0};

/**
 * @brief EGOリソースの定義
 * TODO: グローバルではなく, AppNormalに関連付ける
 */
static EGO_t gEgo[] = {{
    .param = &gEgoParam,
    .state = EGO_STATE_FINISH,
    .v = 0,
}};

/**
 * @brief NCOリソースの定義.
 * TODO: グローバルではなく, AppNormalに関連付ける
 */
static NCO_t gNco[3];

/**
 * @brief 楽譜
 */
static AudioScore_t gAudioScore[2];

/**
 * @brief オーディオコンテキスト定義
 */
static AudioContext_t gAuidoContext = {
    .enable = false,                           //< 出力状態
    .ncos = gNco,                              //< 使用するNCO
    .numNco = sizeof(gNco) / sizeof(gNco[0]),  //< NCO要素数
    .egos = gEgo,                              //< 使用するEGO
    .numEgo = sizeof(gEgo) / sizeof(gEgo[0]),  //< EGO要素数
                                               //    .scores = gAudioScore,                                     //< 楽譜
                                               //    .numScore = sizeof(gAudioScore) / sizeof(gAudioScore[0]),  //< 楽譜数
    .curScore = NULL,                          //< 選択している楽譜
    .noteOffFn = &noteOffHandler,
    .noteOnFn = &noteOnHandler,
    .outputFn = &outputHandler,
};

/**
 * @brief 入力イベント配列
 */
static HitEvent_t gRegionEvent[] = {
    {
        {
            .x = 0,
            .y = 0,
            .w = 240,
            .h = 120,
        },
        &regionEvent_Top,
    },
    {
        {
            .x = 0,
            .y = 240,
            .w = 240,
            .h = 120,
        },
        &regionEvent_Middle,
    },
};

//////////////////////////////////////////////////////////////////////////////
// function
//////////////////////////////////////////////////////////////////////////////

// 円形ポインタ /////////////////////////////

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

void CirclePointer_Render(CirclePointer_t* self, const Canvas_t* canvas) {
  if (self->active) {
    Canvas_DrawCircle(canvas, self->x, self->y, self->r, RGB888toRGB565(self->red, self->green, self->blue));
  }
}

// 円形ポインタ /////////////////////////////

// スライダー /////////////////////////////

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

uint8_t Slider_GetPosition(Slider_t* self) {
  if (NULL == self) {
    return 0u;
  }
  return self->pos;
}

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

// スライダー /////////////////////////////

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

UError_t regionEvent_Top(const void* arg) {
  UError_t err = uSuccess;
  AudioContext_t* ctx = (AudioContext_t*)arg;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (!ctx->enable) {
      AudioScore_Init(&gAudioScore[0]);
      AudioContext_SetScore(ctx, &gAudioScore[0]);
      ctx->enable = true;
    }
  }

  return err;
}

UError_t regionEvent_Middle(const void* arg) {
  UError_t err = uSuccess;
  AudioContext_t* ctx = (AudioContext_t*)arg;

  if (uSuccess == err) {
    if (NULL == ctx) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    if (!ctx->enable) {
      AudioScore_Init(&gAudioScore[1]);
      AudioContext_SetScore(ctx, &gAudioScore[1]);
      ctx->enable = true;
    }
  }

  return err;
}

UError_t doRegionEvent(int32_t const x, int32_t const y, const void* eventArg, const HitEvent_t events[], size_t const numEvents) {
  UError_t err = uSuccess;

  if (uSuccess == err) {
    if (NULL == events) {
      err = uFailure;
    }
  }

  if (uSuccess == err) {
    for (size_t i = 0; i < numEvents; ++i) {
      const int32_t x1 = events[i].hitbox.x;
      const int32_t x2 = events[i].hitbox.x + events[i].hitbox.w;
      const int32_t y1 = events[i].hitbox.y;
      const int32_t y2 = events[i].hitbox.y + events[i].hitbox.h;

      if (((x1 <= x) && (x2 > x)) && ((y1 <= y) && (y2 > y))) {
        if (NULL != events[i].doEvent) {
          err = events[i].doEvent(eventArg);
        }
        break;
      }

    }  // for(size_t i...
  }

  return err;
}

// 譜面処理 //////////////////////////////////////////////////////////////////////

static void noteOffHandler(uint32_t tone, EGO_t* egos, NCO_t* ncos) {
  if (NULL == egos || NULL == ncos) {
    return;
  }

  EGO_NoteOff(&egos[0]);
}

static void noteOnHandler(uint32_t tone, EGO_t* egos, NCO_t* ncos) {
  if (NULL == egos || NULL == ncos) {
    return;
  }

  NCO_SetWeight(&ncos[0], tone);       // ベース
  NCO_SetWeight(&ncos[1], tone >> 1);  // 1オクターブ下
  NCO_SetWeight(&ncos[2], tone << 1);  // 1オクターブ上
  // エンベロープを初期状態へ移行
  EGO_NoteOn(&egos[0]);
}

static uint8_t outputHandler(EGO_t* egos, NCO_t* ncos) {
  // オシレータの値を取得
  const int32_t c0 = NCO_Get(&ncos[0]);   // キャリア0
  const int32_t c1 = NCO_Get(&ncos[1]);   // キャリア1 (c0の1オクターブ下)
  const int32_t op0 = NCO_Get(&ncos[2]);  // オペレータとして使用 (gNco[0] の 1オクターブ上)

  // エンベロープジェネレータの値を取得
  const int32_t env = EGO_Get(&egos[0]);

  // キャリア0 と オペレータ, エンベロープから出力値を決定(=オペレータ変調あり)
  const int32_t out0 = (((c0 * op0) >> 7) * env);  // -8323072 - +8323072
  //const int32_t out0 = (c0 * env);  // -8323072 - +8323072

  // キャリア1 とエンベロープから出力値を決定 (=オペレータ変調無し)
  const int32_t out1 = (c1 * env);  // -8323072 - +8323072

  // オペレータ0 とエンベロープから出力値を決定 (=オペレータ変調無し)
  //const int32_t out2 = (op0 * env);  // -8323072 - +8323072

  // ミキシング
  const int32_t mux = ((out0 + out0 + out1 + out0) >> 2) >> 16;
  //const int32_t mux = out0 >> 16;

  // 0 - 255 (0x00 - 0xff) にクリップして出力
  return 0xff & (128 + (int)(-127 > mux ? -127 : 127 < mux ? 127 : mux));
}

// 譜面処理 //////////////////////////////////////////////////////////////////////

/**
 * @brief 通常状態を表現
 * @param app
 * @param arg
 * @return
 */
static UError_t AppNormal_(AppObject_t* app, const AppArg_t* arg) {
  UError_t err = uSuccess;
  static bool audio_act = false;  // 音声有効フラグ

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
      // タッチ位置に依存したイベント処理を実行
      err = doRegionEvent(arg->touch->coords[0].x, arg->touch->coords[0].y, (const void*)&gAuidoContext, gRegionEvent,
                          sizeof(gRegionEvent) / sizeof(gRegionEvent[0]));
    }
  }

  if (uSuccess == err) {
    // 画面更新
    Canvas_TextureBlt(arg->frame, 0, 0, &texWallpaper, 0, 0, 240, 320);  // 壁紙で塗りつぶし 所要時間:8ms程度

    CirclePointer_Render(&circleP, arg->frame);
    Slider_Render(&sliderP, arg->frame);
  }

  if (uSuccess == err) {
    // オーディオ出力 (譜面再生)
    (void)AudioContext_Write(&gAuidoContext, arg->audio_buff, arg->audio_size);
  }

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
  // テクスチャデータ(背景画像)を初期化
  Texture_Create(&texWallpaper, 240, 320, 480, &wallpaper[12]);

  // テクスチャデータ(キャラクタ)を初期化
  Texture_Create(&texWait, 192, 48, 192 * 2, &wait_icon[12]);

  // オーディオデータ生成用のオシレータ初期化
  NCO_Create(&gNco[0]);
  NCO_Create(&gNco[1]);
  NCO_Create(&gNco[2]);

  // オーディオデータ生成用のエンベロープ初期化
  // ピーク音量, 持続音量, アタック時間(単位:サンプル), 減衰時間(単位:サンプル), リリース時間(単位:サンプル)
  EGOParam_Create(&gEgoParam, 65536.0 * 1.0, 65536.0 * 0.75, 441, 441.0 * 50.0, 441 * 300.0);

  // 譜面データ作成
  AudioScore_Create(&gAudioScore[0], Notes0, sizeof(Notes0) / sizeof(Notes0[0]));
  AudioScore_Create(&gAudioScore[1], Notes1, sizeof(Notes1) / sizeof(Notes1[0]));

  AppObject_t app = {
      // 初期状態をAppInitial_ に設定
      .ActionFn = &AppInitial_,
  };

  return app;
}
