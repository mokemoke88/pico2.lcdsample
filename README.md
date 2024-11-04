# Raspberry PI PICO2 C SDK サンプルプログラム

Raspberry PI PICO2 + WAVESHARE製 2.8インチ 静電容量タッチLCD(240 x 320) を使用したサンプルプログラム

LCD描画処理

CST328 タッチパッド処理

FONTX2形式 埋め込みフォントデータを使用した文字描画処理

PWMを利用した波形信号出力

等を行っています.

## ピン配置

PICO2のピン配置, 利用ピンは以下となります.

|PNo| FUNC | | FUNC | PNo|
|--|--|--|--|--|
|1|UART TX| | VBUS | 40|
|2|UART RX| | VSYS | 39|
|3|GND| | GND| 38|
|4|SPI0 SCK| | 3V3_EN |37|
|5|SPI0 MOSI| | 3V3(OUT) |36|
|6|SPI0 MISO| | (N.C.) | 35 |
|7|LCD CS| | (N.C.) | 34 |
|8|GND| | GND | 33 |
|9|LCD DC| | (N.C.) | 32 |
|10|LCD RST| | (N.C.) | 31 |
|11|LCD BL(PWM0)| | (N.C.) | 30 |
|12|(N.C.) | | (N.C.) | 29 |
|13|GND| | GND | 28 |
|14|TP RST| | (N.C.) | 27 |
|15|TP INT| | (N.C.) | 26 |
|16|TP SDA(I2C0)| | (N.C.) | 25 |
|17|TP SCL(I2C0)| | (N.C.) | 24 |
|18|GND| | GND | 23 |
|19|(N.C)| | (N.C.) | 22 |
|20|(N.C)| | PWM1 | 21 |

## 利用素材

本プログラム作成にあたって, 以下の素材を使用させていただいております.

### 組込みフォントデータ

『小夏』フォント

URI: http://blog.masuseki.com/?p=233

License: The MIT License https://opensource.org/license/mit
