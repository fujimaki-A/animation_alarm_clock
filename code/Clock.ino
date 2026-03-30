// LEDストリップ
#include <FastLED.h>
#include "led_color.h"
#define NUM_LEDS 8
#define DATA_PIN 6

CRGB leds[NUM_LEDS];

unsigned long lastLEDTime = 0;
int targetIdx = 0;

// Wi-Fi
#include "WiFiS3.h"
#include "arduino_secrets.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;
WiFiServer server(80);

// NTP
#include <NTPClient.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.nict.jp");

// OLED 
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // OLED の横幅（ピクセル）
#define SCREEN_HEIGHT 64  // OLED の高さ（ピクセル）
#define OLED_RESET -1     // リセットピン番号

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define dark 0x01
#define light 0x7F
bool isDisplayOn = true;

// QRコード
#include <qrcode.h>

QRCode qrcode;
uint8_t qrcodeData[200];

// アニメーション
#include "animation.h"

//超音波モジュール
const int echoPin = 3;
const int trigPin = 4;

// オーディオモジュール
#include "analogWave.h"
#include "melodies.h"

analogWave wave(DAC);

// HTML
#include "website.h"

// アニメとアラームの更新間隔と長さを管理
unsigned long lastNoteTime = 0;
unsigned long lastAnimTime = 0;

int currentNoteIndex = 0;
int currentFrame = 0;
bool isAlarmPlaying = false;

unsigned long waitTime = 500;
bool isWaitingNextLoop = false;

// アラームをかける時間を保存する変数
int alarm1_h = -1, alarm1_m = -1;
bool alarm1_days[7] = {false};
bool repeat1_enabled = false;
int alarm2_h = -1, alarm2_m = -1;
bool alarm2_days[7] = {false};
bool repeat2_enabled = false;

// アラームを管理する変数
bool isAlarm1Active = false;
bool isAlarm2Active = false;

// 時間の更新
int lastCheckedMinute = -1;

void setup() {
  // 通信にかける時間を調整
  Wire.begin();
  Wire.setWireTimeout(100000, true);
  
  Serial.begin(9600);
  
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // OLED ディスプレイを初期化して画面をクリア
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(light); 
  display.display();
  display.clearDisplay();

  // NTP クライアントを初期化
  timeClient.begin();
  timeClient.setTimeOffset(3600 * 9);  

  Serial.println("NTP同期を試行中...");

  while (timeClient.getEpochTime() < 1700000000) {
    timeClient.begin();
    delay(500);
    bool success = timeClient.forceUpdate(); // 強制的に更新をかける
    if (success) {
      Serial.println("同期成功！");
    } else {
      Serial.print("同期失敗... 再試行中: ");
      Serial.println(timeClient.getEpochTime());
    }
    delay(2000); // サーバーに負荷をかけないよう2秒待機
  }

  Serial.print("現在の時刻（秒）: ");
  Serial.println(timeClient.getEpochTime());
  
  // QRコードの設定（動的IPアドレスに対応）
  String url = "http://" + WiFi.localIP().toString();
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, url.c_str());
  
  Serial.print("QRコードのURL: ");
  Serial.println(url);

  // 超音波モジュールのピンに入出力を設定
  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT);

  // DACの設定
  analogWrite(A0, 0);
  wave.sine(10);

  // LEDストリップの初期化
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(32);

  server.begin();

  // ディスプレイに表示する関数を呼ぶ
  displayData();
}

void loop() {
  unsigned long currentTime = millis();
  float distance =readDistance();

  checkAlarm();

  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    const char* weekKeys1[] = {"CHK_MON1", "CHK_TUE1", "CHK_WED1", "CHK_THU1", "CHK_FRI1", "CHK_SAT1", "CHK_SUN1" };
    const char* weekKeys2[] = {"CHK_MON2", "CHK_TUE2", "CHK_WED2", "CHK_THU2", "CHK_FRI2", "CHK_SAT2", "CHK_SUN2" };
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            String response = String(index_html);
            if (repeat1_enabled) {
                response.replace("CHK_R1", "checked"); 
            } else {
                response.replace("CHK_R1", ""); 
            }
            if (repeat2_enabled) {
                response.replace("CHK_R2", "checked"); 
            } else {
                response.replace("CHK_R2", ""); 
            }

            // アラーム1の置換
            if (alarm1_h == -1) {
                response.replace("VAL_H1", ""); // -1なら空文字
                response.replace("VAL_M1", "");
            } else {
                response.replace("VAL_H1", String(alarm1_h));
                response.replace("VAL_M1", String(alarm1_m));
            }

            // アラーム2の置換
            if (alarm2_h == -1) {
                response.replace("VAL_H2", "");
                response.replace("VAL_M2", "");
            } else {
                response.replace("VAL_H2", String(alarm2_h));
                response.replace("VAL_M2", String(alarm2_m));
            }

            // アラーム1の繰り返し曜日の置換
            for (int i = 0; i < 7; i++){
              if (alarm1_days[i]) {
                response.replace(weekKeys1[i], "checked");
              } else {
                response.replace(weekKeys1[i], "");
              }
            }

            // アラーム2の繰り返し曜日の置換
            for (int i = 0; i < 7; i++){
              if (alarm2_days[i]) {
                response.replace(weekKeys2[i], "checked");
              } else {
                response.replace(weekKeys2[i], "");
              }
            }

            client.print(response);
            client.println();
            break;
          } else {
            parseAlarmSetUp(currentLine);
            currentLine = "";
          }
        } else if (c != '\r'){
          currentLine += c;
        }
      }
    }
    client.stop();
  }

  if ((isAlarm1Active || isAlarm2Active) && distance > 0 && distance < 5) {
    stopAlarm();
  } else if (isAlarm1Active) {
    playMusicWithAnime(SONG_1);
    updateLEDs(colorRGB1, 2);
  } else if (isAlarm2Active) {
    playMusicWithAnime(SONG_2);
    updateLEDs(colorRGB2, 3);
  } else {
    stopAlarm();
  }
}

// アラームの時間か判定する関数
void checkAlarm() {
  timeClient.update();
  int currentH = timeClient.getHours();
  int currentM = timeClient.getMinutes();
  int currentW = timeClient.getDay();
  checkForOLED(currentH, currentM);
  if (currentM != lastCheckedMinute) {
    if (!isAlarm1Active && !isAlarm2Active) {
      bool isMatch1 = true;
      bool isMatch2 = true;
      if (repeat1_enabled) {
        int idx1 = (currentW == 0) ? 6 : currentW - 1;
        isMatch1 = alarm1_days[idx1];
      }
      if (repeat2_enabled) {
        int idx2 = (currentW == 0) ? 6 : currentW - 1;
        isMatch2 = alarm2_days[idx2];
      }
      if (isMatch1 && currentH == alarm1_h && currentM == alarm1_m) {
        isAlarm1Active = true;
          if (!isDisplayOn) {
            isDisplayOn = true;
          }
        currentNoteIndex = 0;
        lastCheckedMinute = currentM;
        if (!repeat1_enabled) {
          alarm1_h = -1;
          alarm1_m = -1;
          repeat1_enabled = false;
          for (int i=0; i<7; i++) alarm1_days[i] = false;
        }
      } else if (isMatch2 && currentH == alarm2_h && currentM == alarm2_m) {
        isAlarm2Active = true;
        currentNoteIndex = 0;
        if (!isDisplayOn) {
          isDisplayOn = true;
        }
        lastCheckedMinute = currentM;
        if (!repeat2_enabled) {
          alarm2_h = -1;
          alarm2_m = -1;
          repeat2_enabled = false;
          for (int i=0; i<7; i++) alarm2_days[i] = false;
        }
      }
    }
  }
}

// OLEDのナイトモード
void checkForOLED(int currentH, int currentM){
  if (currentH == 23 && currentM == 0){
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(dark); 
    isDisplayOn = false;
  }
  if (currentH == 0 && currentM == 0) {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }
  if (currentH == 4 && currentM == 30) {
    display.ssd1306_command(SSD1306_DISPLAYON);
    isDisplayOn == true;
    displayData();
  } 
  if (currentH == 5 && currentM == 30 ){
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(light); 
  }
}

// 繰り返しの有無や時間を保存する関数
void parseAlarmSetUp(String line) {
  if (line.indexOf("h1=") != -1 && line.indexOf("m1=") != -1) {
    if (line.indexOf("repeat1=on") != -1){
      repeat1_enabled = true;
      if (repeat1_enabled){
        alarm1_days[0] = (line.indexOf("mon1=on") != -1);
        alarm1_days[1] = (line.indexOf("tue1=on") != -1);
        alarm1_days[2] = (line.indexOf("wed1=on") != -1);
        alarm1_days[3] = (line.indexOf("thu1=on") != -1);
        alarm1_days[4] = (line.indexOf("fri1=on") != -1);
        alarm1_days[5] = (line.indexOf("sat1=on") != -1);
        alarm1_days[6] = (line.indexOf("sun1=on") != -1);
      } else {
        for(int i = 0; i < 7; i++) alarm1_days[i] = false;
      }
    }
    int hPos = line.indexOf("h1=") + 3;
    int hEnd = line.indexOf("&", hPos);
    if (hEnd == -1){
      hEnd = line.indexOf(" ", hPos); 
    }
    int mPos = line.indexOf("m1=") + 3;
    int mEnd = line.indexOf("&", mPos);
    if (mEnd == -1){
      mEnd = line.indexOf(" ", mPos); 
    }
    alarm1_h = line.substring(hPos, hEnd).toInt();
    alarm1_m = line.substring(mPos, mEnd).toInt();
  }
  if (line.indexOf("h2=") != -1 && line.indexOf("m2=") != -1) {
    if (line.indexOf("repeat2=on") != -1){
      repeat2_enabled = true;
      if (repeat2_enabled){
        alarm2_days[0] = (line.indexOf("mon2=on") != -1);
        alarm2_days[1] = (line.indexOf("tue2=on") != -1);
        alarm2_days[2] = (line.indexOf("wed2=on") != -1);
        alarm2_days[3] = (line.indexOf("thu2=on") != -1);
        alarm2_days[4] = (line.indexOf("fri2=on") != -1);
        alarm2_days[5] = (line.indexOf("sat2=on") != -1);
        alarm2_days[6] = (line.indexOf("sun2=on") != -1);
      } else {
        for(int i = 0; i < 7; i++) alarm2_days[i] = false;
      }
    }
    int hPos = line.indexOf("h2=") + 3;
    int hEnd = line.indexOf("&", hPos);
      if (hEnd == -1){
        hEnd = line.indexOf(" ", hPos); 
      }
    int mPos = line.indexOf("m2=") + 3;
    int mEnd = line.indexOf("&", mPos);
    if (mEnd == -1){
      mEnd = line.indexOf(" ", mPos); 
    }
    alarm2_h = line.substring(hPos, hEnd).toInt();
    alarm2_m = line.substring(mPos, mEnd).toInt();
  }
  if (line.indexOf("h1=") != -1) {
    Serial.print("アラーム1 設定時刻: ");
    Serial.print(alarm1_h);
    Serial.print(":");
    Serial.println(alarm1_m);
    Serial.print("繰り返し: ");
    Serial.println(repeat1_enabled ? "ON" : "OFF");
    if (repeat1_enabled) {
      Serial.print("曜日: ");
      const char *dNames[] = {"月", "火", "水", "木", "金", "土", "日"};
      for (int i = 0; i < 7; i++) {
        if (alarm1_days[i]) Serial.print(dNames[i]);
      }
    }
    Serial.println();
  }
  if (line.indexOf("h2=") != -1) {
    Serial.print("アラーム2 設定時刻: ");
    Serial.print(alarm2_h);
    Serial.print(":");
    Serial.println(alarm2_m);
    Serial.print("繰り返し: ");
    Serial.println(repeat2_enabled ? "ON" : "OFF");
    if (repeat2_enabled) {
      Serial.print("曜日: ");
      const char *dNames[] = {"月", "火", "水", "木", "金", "土", "日"};
      for (int i = 0; i < 7; i++) {
        if (alarm2_days[i])   Serial.print(dNames[i]);
      }
    }
    Serial.println();
  }
}

// アラームとアニメを流す関数
void playMusicWithAnime(const ALARM &melody){
  unsigned long currentTime = millis();

  if (&melody == &SONG_1) {
    rabitAnime(currentTime);
  } else if (&melody == &SONG_2) {
    sakuraAnime(currentTime);
  } else {
    commonAnime(currentTime);
  }

  if (isWaitingNextLoop) {
    if (currentTime - lastNoteTime >= waitTime) {
      isWaitingNextLoop = false;
      currentNoteIndex = 0;
      lastNoteTime = currentTime;
    }
    return;
  }
  int noteDuration = melody.tempo / melody.durations[currentNoteIndex];
  int soundActiveDuration = (melody.slur[currentNoteIndex] == 1) ? noteDuration : ((noteDuration * 9) / 10);

  if (currentTime - lastNoteTime >= soundActiveDuration) {
    wave.stop();
  }
  if (currentTime - lastNoteTime >= noteDuration) {
    lastNoteTime = currentTime;
    currentNoteIndex++;
    if (currentNoteIndex >= melody.length) {
      wave.stop();
      isWaitingNextLoop = true;
      return;
    }
    if (melody.notes[currentNoteIndex] != REST) {
      wave.freq((float)melody.notes[currentNoteIndex]);
    } else {
      wave.stop();
    }
  }
}

// 共通アニメーションを流す関数
void commonAnime(unsigned long currentTime) {
  if (currentTime - lastAnimTime >= 500){
    lastAnimTime = currentTime;
    display.clearDisplay();

    display.drawBitmap(0, 0, circuit_commonAnime[currentFrame], 128, 64, WHITE);

    display.display();

    currentFrame = (currentFrame + 1) % commonAnime_frames;
  }
}

// 回る空うさぎ専用アニメーション
void rabitAnime(unsigned long currentTime) {
  if (currentTime - lastAnimTime >= 750){
    lastAnimTime = currentTime;
    display.clearDisplay();

    display.drawBitmap(0, 0, circuit_rabitAnime[currentFrame], 128, 64, WHITE);

    display.display();

    currentFrame = (currentFrame + 1) % rabitAnime_frames;
  }
}

// 千本桜専用アニメーション
void sakuraAnime(unsigned long currentTime) {
  if (currentTime - lastAnimTime >= 500){
    lastAnimTime = currentTime;
    display.clearDisplay();

    display.drawBitmap(0, 0, circuit_sakuraAnime[currentFrame], 128, 64, WHITE);

    display.display();

    currentFrame = (currentFrame + 1) % sakuraAnime_frames;
  }
}

// LEDストリップを光らせる関数
void updateLEDs(int colorRGB[][3], int colorNum) {
  unsigned long currentTime = millis();
  if (currentTime - lastLEDTime >= 500){
    lastLEDTime = currentTime;
    targetIdx = (targetIdx + 1) % colorNum;

    for (int i=0; i < NUM_LEDS; i++) {
      leds[i]=CRGB(
        colorRGB[targetIdx][0],
        colorRGB[targetIdx][1],
        colorRGB[targetIdx][2]
      );
    }
    FastLED.show();
  }
}

// アラームを止めたときの関数
void stopAlarm() {
  wave.stop();
  isAlarm1Active = false;
  isAlarm2Active = false;
  currentNoteIndex = 0;
  lastNoteTime = 0;
  for(int i=0; i<NUM_LEDS; i++) leds[i] = CRGB::Black;
  FastLED.show();
  time_t rawtime = (time_t)timeClient.getEpochTime();
  struct tm * ti = localtime(&rawtime);
  if ((ti->tm_hour >= 0 && ti->tm_hour < 4) || (ti->tm_hour == 4 && ti->tm_min <= 30)) {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    isDisplayOn = false;
  } else {
    displayData();
  }
}

// 超音波モジュールの値から距離を計算する関数
float readDistance() {
  digitalWrite(trigPin, LOW);   // トリガーピンを LOW にしてパルスをリセット
  delayMicroseconds(2);         // 2 マイクロ秒待つ
  digitalWrite(trigPin, HIGH);  // 10 マイクロ秒のパルスを送信
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);   // トリガーピンを LOW に戻す
  float distance = pulseIn(echoPin, HIGH) / 58.00;  // 計算式: (音速 340m/s → 1us あたりの距離) / 2
  return distance;
}

// ディスプレイに表示させる関数
void displayData() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);  // 白色で文字を描画
  qrdata();
  dateTimeData();
  display.display();
}

// 日時を管理する関数
void dateTimeData() {
  // 年月日を取得
  time_t rawtime = (time_t)timeClient.getEpochTime();
  struct tm * ti = localtime(&rawtime);

  if (ti == NULL) return;

  int textX = 66 ;

  display.setTextSize(1);
  display.setCursor(textX, 10);

  display.print(ti->tm_year + 1900);

  display.setCursor(textX, 20);

  if(ti->tm_mon + 1 < 10) display.print("0");
  display.print(ti->tm_mon + 1);
  display.print("/");
  if(ti->tm_mday < 10) display.print("0");
  display.print(ti->tm_mday);

  display.print(" ");

  // 曜日を表示
  const char* daysOfTheWeek[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  display.print(daysOfTheWeek[ti->tm_wday]);

  display.setTextSize(2);
  display.setCursor(textX, 35);

  // 時を表示（10未満は0埋め）
  if (ti->tm_hour < 10) display.print("0");
  display.print(ti->tm_hour);

  display.print(":");

  // 分を表示（10未満は0埋め）
  if (ti->tm_min < 10) display.print("0");
  display.print(ti->tm_min);
}

// QRコードを作る関数
void qrdata() {
  int scale = 2; // 1セルを2×2pxで描画
  int qroffsetX = 0, qroffsetY = (SCREEN_HEIGHT - (qrcode.size * scale)) / 2;

  for (int y = 0; y < qrcode.size; y++) {
    for (int x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        display.fillRect(
          qroffsetX + x * scale,
          qroffsetY + y * scale,
          scale, scale, SSD1306_WHITE
        );
      }
    }
  }
}
