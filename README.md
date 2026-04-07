# 卒業制作：アニメーション付きアラーム時計

## 概要

Webブラウザから設定した時刻に合わせて、メロディの再生とドット絵アニメーションが連動する多機能型アラーム時計

[アラームの動作](https://youtu.be/uBGSTO8ovKI)

これ埋め込みのほうがいいんじゃね？

## 主な機能

**OLED画面表示**

* 情報表示
  * 年月日、曜日、時間を表示
* 簡単設定
  * 設定ページへのQRコード
* ナイトモード
  * 指定した時間帯は、ディスプレイの輝度を下げて眩しさを抑える
  * 深夜帯はディスプレイを消灯し安眠をサポートし、朝になったら自動で点灯する
  
  ディスプレイの表示
  ![alttext](image/OLED_display.jpg)
  
  QRコードが読み込めている様子
  ![alttext](image/OLED_qr.jpg)

  [ナイトモードの変化](https://youtu.be/ZIAWJFeXJUg)

<br>

**アラーム機能**

* 視聴覚演出
  * オーディオモジュールからはメロディを再生
  * OLED上にアラームと対応したアニメーションが流れる
  * LEDストリップによる光の演出
* 非接触停止機能
  * 超音波センサーから5㎝以内に手をかざすことで、本体に触れずともアラームを停止させる

<br>

**Web設定インターフェース**

* 同一ネットワーク内のデバイスから、アラーム時刻、繰り返しの有無、曜日の指定をリアルタイムで変更・保存可能

デザイン<br>
![setup1](image/Alarm_setup1.jpg)

選択式でアラームを設定<br>
![setup2](image/Alarm_setup2.jpg)

設定後<br>
![setup3](image/Alarm_setup3.jpg)

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

* WiFiS3.h: Wi-Fi接続およびWebサーバー機能の制御
* WiFiUdp.h: NTPサーバーからの時刻取得に使用
* Wire.h: OLEDとのI2C通信用
* analogWave.h: 内蔵DAC（A0）を用いたメロディ波形出力
* TimeLib.h: 取得した時刻データの変換・管理用

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
  
  **作成方法:**
  1. `code` フォルダ内に `arduino_secrets.h` という名前のファイルを作成
  2. 以下の内容を記述し、自分のWi-Fi情報を入力:

  ```cpp
  #define SECRET_SSID "your-ssid"
  #define SECRET_PASS "your-password"
  ```

  1. このファイルはGitリポジトリに含めないでください（`.gitignore` に追加推奨）
* melodies.h
  楽曲データを保持するためのヘッダーファイル
* pitches.h
  音階（NOTE_C4等）と周波数を紐付けた辞書を保持するためのヘッダーファイル
* bitmap.h
  画像データを16進数の配列として保持するためのヘッダーファイル
* animation.h
  アニメーションのコマ割りと枚数を一括管理するヘッダーファイル
* website.h
  Webページを設計するHTML・CSSを記述するためのヘッダーファイル
* led_color.h
  LEDストリップの色を管理するためのヘッダーファイル

### 配線図
>
> ※配線図上では省略しているがBreadboard Power Module with Batteryを取り付けている<br>
> ※この配線図はFritzingの都合により、以下の部品を代用品で表現
>
> * オーディオモジュール：実際にはHXJ8002を搭載したオーディオボードを使用

![altText](image/Alarm_clock_ブレッドボード.png)

### 回路図

![altText](image/Alarm_clock_回路図.png)

### フローチャート

```mermaid
graph TD
    Start([開始]) --> Init[各種初期化<br>WiFi/OLED/NTP/DAC/LED]
    Init --> Sync{NTP同期}
    Sync -- 未完了 --> Sync
    Sync -- 完了 --> Disp[QRコードと時刻を表示]
    Disp --> Loop[loop開始]
    
    Loop --> Current{0.5秒経過したか}
    Current -- はい --> Sensor[距離センサー読み取り]
    Current -- いいえ --> Web
    
    %% --- 時刻判定と画面制御 ---
    Sensor --> NightMode
    NightMode -- 23:00~ --> Dim[輝度を下げる]
    NightMode -- 00:00~ --> Off[ディスプレイ消灯]
    NightMode -- 05:00~ --> On[ディスプレイ点灯]
    NightMode -- 06:00~ --> Nomal[輝度を通常に戻す]
    
    Dim --> TimeMatch
    Off --> TimeMatch
    On --> TimeMatch
    Nomal --> TimeMatch

    %% --- アラーム時刻の判定 セクションの書き換え ---
    TimeMatch{設定した時刻になったか}
    TimeMatch -- はい --> SetActive[アラームを「作動中」に切り替え]
    TimeMatch -- いいえ --> Web
    SetActive --> DayCheck{今日は繰り返す曜日か<br>または単発設定か}

    DayCheck -- 単発 --> Web
    DayCheck -- 繰り返す --> Repeat[繰り返しのアラーム]
    
    Repeat --> Web[Webサーバー処理<br>設定の更新など]
    %% ------------------------------------------

    Web --> Mode{アラーム状態}
    
    Mode -- 停止中 --> Idle[通常表示]
    Mode -- 作動中 --> Distance{距離 < 5cm}
    
    Distance -- はい --> Stop[アラームを止める<br>単発なら設定をリセット]
    Distance -- いいえ --> Play[音楽とアニメ再生<br>LEDストリップの光演出]
    
    Play --> CheckMODE
    Idle --> CheckMODE
    Stop --> Idle
    CheckMODE{ナイトモードか}
    CheckMODE -- はい --> StandBy[1秒待つ]
    CheckMODE -- いいえ --> Loop
    StandBy --> Loop
```

### 使用ツール

Arduino IDE

* Arduinoボード向けのプログラムを作成・編集・書き込みできる統合開発環境
* [公式サイト](https://www.arduino.cc/en/software/)

ibis Paint

* イラスト作成アプリ
* アニメーション制作に使用
* [公式サイト](https://ibispaint.com/)

image2cpp

* 画像とバイト配列を相互に変換できるサイト
* [image2cpp リンク](https://javl.github.io/image2cpp/)

### 参考サイト

* [基本プロジェクト：超音波](https://docs.sunfounder.com/projects/elite-explorer-kit/ja/latest/basic_projects/06_basic_ultrasonic_sensor.html)

* [基本プロジェクト：WS2812 RGB LEDストリップ](https://docs.sunfounder.com/projects/elite-explorer-kit/ja/latest/basic_projects/12_basic_ws2812.html)
* [基本プロジェクト：OLED](https://docs.sunfounder.com/projects/elite-explorer-kit/ja/latest/basic_projects/15_basic_oled.html)
* [楽しいプロジェクト：ウェルカム](https://docs.sunfounder.com/projects/elite-explorer-kit/ja/latest/fun_projects/01_fun_welcome.html)
* [IoTプロジェクト：シンプルWebサーバー](https://docs.sunfounder.com/projects/elite-explorer-kit/ja/latest/iot_projects/01_iot_webserver.html)
* [IoTプロジェクト：Weather Timeスクリーン](https://docs.sunfounder.com/projects/elite-explorer-kit/ja/latest/iot_projects/06_iot_weather_oled.html)
* [回る空うさぎ－原曲動画](https://www.youtube.com/watch?v=CLywtSOj4NA)
* [回る空うさぎ－参照動画](https://www.youtube.com/watch?v=1sXb8LwUj3U)
* [千本桜－原曲動画](https://www.youtube.com/watch?v=shs0rAiwsGQ)
* [千本桜－参照動画](https://www.youtube.com/watch?v=3w8hDz-5e9U)

### バッテリー消費

現時点でArduino UNO R4 Wifiへの電力供給を2種類のモバイルバッテリーで行った際のおおよその持続時間は、この[5000ｍAのモバイルバッテリー](https://www.elecom.co.jp/products/DE-C37-5000GN.html)では23~24時間ほど、この[10000mAのモバイルバッテリー](https://www.elecom.co.jp/products/DE-C66-10000BK.html)では46時間
