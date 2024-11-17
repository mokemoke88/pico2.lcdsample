# 基礎部品集

canvas.h : 画像描画用 部品
  
  画像描画用のメモリ領域管理
  点の描画機能
  線分の描画機能
  円の描画機能
  全画面塗りつぶし機能

utf8string.h : UTF8 関連の文字データ操作

  UTF8バイト列からUTF32文字データ変換機能
  UTF32文字データからSJIS文字データ変換機能

  UTF32 -> SJIS 変換テーブルを含むためバイナリサイズに注意すること.
  (概算で89KiB程度占有する.)
  UTF32 組み合わせ文字列の変換は未サポート

fontx2.h : FONTX2形式のフォントファイル操作 部品

  フォントデータを取り込むため, バイナリサイズに注意すること.

  SJIS文字コード に対する字形データを提供

  文字列の描画は以下の流れ
    1. utf8string で, UTF8からUTF32->SJIS と変換.
    2. SJIS から fontx2 で字形を取得する.

linkedlist.h : 双方向リンクリスト構造 の操作関数

textbox.h : 指定行分のデータを保持するリングバッファ.
  テキストボックス的なものを表現するために作成.

audio.h : オーディオ関連のコンビニヘッダ

audio/ego.h : エンベロープジェネレータ構造 EGO_t, 操作の定義

audio/nco.h : オシレータ構造 NCO_t, 操作関数の定義

audio/score.h : 譜面構造 AudioScore_t, 操作関数の定義

audio/audiocontext.h : 譜面音声再生制御構造 AudioContext_t, 操作関数の定義



