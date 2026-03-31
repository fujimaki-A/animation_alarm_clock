# 卒業制作：アニメーション付きアラーム時計
## 概要

Web上で設定した時刻にアラームとアニメーションが流れる時計

[アラームの動作](https://youtu.be/aMdnyNK32CE)

## 主な機能

**OLED画面表示**
* 設定ページへのQRコード
* 年月日、曜日、時間
* ナイトモード
  * 設定時刻になったら画面の輝度を変更する
  * 設定時刻になったらディスプレイの表示をオフまたはオンにする
  
  ディスプレイの表示
  ![alttext](image/OLED_display.jpg)
  
  QRコードが読み込めている様子
  ![alttext](image/OLED_qr.jpg)

  ナイトモードの変化

**アラーム機能**
* オーディオモジュールとスピーカーからはメロディを再生
* OLED上にアラームと対応したアニメーションが流れる
* LEDストリップが光る
* 超音波センサーから5㎝以内に手をかざすと止まる

**Webページ**
* アラームを鳴らす時間、繰り返しの有無や繰り返す曜日の設定が可能

## 仕様書
### 使用モジュール
|部品|個数|用途|接続ピン|
|-|-|-|-|
|Arduino UNO R4 Wifi|1|全体制御<br>時刻取得<br>Webサーバー|USBケーブル|
|OLED|1|画面表示|GND：GND<br>VCC：5V<br>SCL：SCL<br>SDA：SDA|
|オーディオモジュール（HXJ8002）|1|音声データの再生|GND：GND<br>VCC：5V<br>IN：A0(DAC)|
|スピーカー|1|音声出力|オーディオモジュール V1 V2|
|超音波距離センサー（HC-SR04）|1|非接触型のアラーム停止機能|GND：GND<br>VCC：5V<br>Trig：D4<br>Echo：D3|
|WS2812B LEDストリップ（1×8）|1|アラーム中の光演出|GND：GND<br>VCC：5V<br>Data：D6|
|Breadboard Power Module with Battery|1|モジュール類への安定的な電源供給|ブレッドボード|

### ソフトウェア
#### 標準ライブラリ
* WiFiS3.h
  Arduino UNO R4 WiFi向けのWi-Fi制御ライブラリ
* SPI.h
  高速なシリアル通信を行うためのライブラリ
* WiFiUdp.h
  I2C通信（OLEDとの接続）を行うためのライブラリ
* analogWave
  内蔵DACを使って波形を出力するArduino UNO R4 WiFi専用ライブラリ

#### 追加ライブラリ
* FastLED.h
  LEDストリップ（WS2812B）の制御ライブラリ
* NTPClient.h
  ネットワークから正確な時刻を取得するためのライブラリ
* Adafruit_GFX.h
  画面描画の基本機能を提供するライブラリ
* Adafruit_SSD1306.h
  今回使用しているOLEDディスプレイ（SSD1306）を制御するための専用ライブラリ
* qrcode.h
  QRコードのデータを生成するためのライブラリ

#### 自作ファイル
* arduino_secrets.h
  Wi-FiのSSIDやパスワード等の機密情報を管理するためのヘッダーファイル
* melodies.h
  楽曲データを保持するためのヘッダーファイル
* pitches.h
  音階（NOTE_C4等）と周波数を紐付けた辞書を保持するためのヘッダーファイル
* bitmap.h
  画像データを16進数の配列として保持するためのヘッダーファイル
* animation.h
  アニメーションのコマ割りと枚数を管理するヘッダーファイル
* website.h
  Webページを設計するHTML・CSSを記述するためのヘッダーファイル
* led_color.h
  LEDストリップの色を管理するためのヘッダーファイル

### 配線図
> ※配線図上では省略しているがBreadboard Power Module with Batteryを取り付けている
> ※この配線図はFritzingの都合により、以下の部品を代用品で表現
> * オーディオモジュール：実際にはHXJ8002を搭載したオーディオボードを使用

![altText](image/Alarm_clock_ブレッドボード.png)
### 回路図
![altText](image/Alarm_clock_回路図.png)

### フローチャート

### 使用ツール
