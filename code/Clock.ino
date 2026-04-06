// LEDストリップ
#include <FastLED.h>
#include "led_color.h"
#define NUM_LEDS 8
#define DATA_PIN 6

// ========== デバッグマクロ ==========
#define DEBUG 0
#if DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(fmt, ...)
#endif
// =================================

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
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED の横幅（ピクセル）
#define SCREEN_HEIGHT 64 // OLED の高さ（ピクセル）
#define OLED_RESET -1    // リセットピン番号

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

// 超音波モジュール
const int echoPin = 3;
const int trigPin = 4;

// オーディオモジュール
#include "analogWave.h"
#include "melodies.h"

// アラーム設定構造体
struct AlarmConfig
{
  int hour;
  int minute;
  bool days[7];
  bool repeatEnabled;
  bool active;
  const int (*ledColors)[3];
  int ledColorCount;
  const ALARM *melody;
};

analogWave wave(DAC);

// HTML
#include "website.h"

// ========== 定数定義 ==========
const unsigned long CHECK_INTERVAL_MS = 500;             // メインループのチェック間隔（アラーム活性時）
const unsigned long IDLE_CHECK_INTERVAL_MS = 5000;       // メインループのチェック間隔（アラーム非活性時）
const unsigned long NTP_TIMEOUT_MS = 30000;              // NTP同期のタイムアウト（30秒）
const unsigned long NTP_RETRY_INTERVAL_MS = 2000;        // NTP再試行間隔
const unsigned long ANIMATION_INTERVAL_MS = 1000;        // 共通アニメーション間隔（省電力化）
const unsigned long RABBIT_ANIMATION_INTERVAL_MS = 750;  // うさぎアニメーション間隔
const unsigned long SAKURA_ANIMATION_INTERVAL_MS = 1000; // さくらアニメーション間隔（省電力化）
const int LED_BRIGHTNESS = 16;                           // LED輝度 (0-255) 省電力化
const int DAC_INITIAL_VALUE = 0;                         // DAC初期値
const int WIFI_TIMEOUT_MS = 30000;                       // WiFi接続タイムアウト
const int WIFI_RETRY_INTERVAL_MS = 500;                  // WiFi再試行間隔
const int OLED_CONTRAST_DARK = 0x01;                     // OLED暗いコントラスト
const int OLED_CONTRAST_LIGHT = 0x40;                    // OLED明るいコントラスト（省電力化）
const int DISTANCE_THRESHOLD_CM = 5;                     // アラーム停止距離（cm）
const time_t NTP_EPOCH_THRESHOLD = 1700000000;           // NTP同期判定閾値
const int TIMEZONE_OFFSET_HOURS = 9;                     // タイムゾーン（日本時間）
const int WIRE_TIMEOUT_US = 100000;                      // I2Cタイムアウト（マイクロ秒）
const int WAIT_TIME_MS = 500;                            // 曲間待機時間
const float SOUND_SPEED_FACTOR = 58.0;                   // 超音波距離計算係数
const int QR_CODE_SCALE = 2;                             // QRコード拡大率
const unsigned long WIFI_RECONNECT_COOLDOWN_MS = 30000;  // WiFi再接続クールダウン（30秒）
// =============================

// 関数プロトタイプ宣言
void displayData();
void dateTimeData(struct tm *ti);
void qrdata();
float readDistance();
void checkAlarm();
void checkForOLED(int currentH, int currentM);
void parseAlarmSetUp(String line);
void playMusicWithAnime(const ALARM &melody);
void commonAnime(unsigned long currentTime);
void rabitAnime(unsigned long currentTime);
void sakuraAnime(unsigned long currentTime);
void updateLEDs(const int colorRGB[][3], int colorNum);
void stopAlarm();

// アニメとアラームの更新間隔と長さを管理
unsigned long lastNoteTime = 0;
unsigned long lastAnimTime = 0;

int currentNoteIndex = 0;
bool isAlarmPlaying = false;

unsigned long waitTime = 500;
bool isWaitingNextLoop = false;

// ディスプレイ更新用の前回表示時刻
int lastDisplayedHour = -1;
int lastDisplayedMinute = -1;

// WiFi再接続用
float lastDistance = -1;
unsigned long lastWiFiReconnectAttempt = 0;

// アラーム設定構造体のインスタンス
AlarmConfig alarms[2] = {
    {.hour = -1,
     .minute = -1,
     .days = {false, false, false, false, false, false, false},
     .repeatEnabled = false,
     .active = false,
     .ledColors = colorRGB1,
     .ledColorCount = 2,
     .melody = &SONG_1},
    {.hour = -1,
     .minute = -1,
     .days = {false, false, false, false, false, false, false},
     .repeatEnabled = false,
     .active = false,
     .ledColors = colorRGB2,
     .ledColorCount = 3,
     .melody = &SONG_2}};

// 時間の更新
int lastCheckedMinute = -1;

void setup()
{
  // 通信にかける時間を調整
  Wire.begin();
  Wire.setWireTimeout(100000, true);

  // Serial.begin(9600);

  WiFi.begin(ssid, pass);
  unsigned long wifiStartTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStartTime < WIFI_TIMEOUT_MS)
  {
    delay(WIFI_RETRY_INTERVAL_MS);
    // Serial.print(".");
  }

  // Serial.println("\nWiFi connected!");
  // Serial.print("IP address: ");
  // Serial.println(WiFi.localIP());

  // OLED ディスプレイを初期化して画面をクリア
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(light);
  display.display();
  display.clearDisplay();

  // NTP クライアントを初期化
  timeClient.begin();
  timeClient.setTimeOffset(3600 * 9);

  // Serial.println("NTP同期を試行中...");

  unsigned long ntpStartTime = millis();
  bool ntpSuccess = false;
  while (timeClient.getEpochTime() < NTP_EPOCH_THRESHOLD && millis() - ntpStartTime < NTP_TIMEOUT_MS)
  {
    timeClient.begin();
    delay(WIFI_RETRY_INTERVAL_MS);
    bool success = timeClient.forceUpdate(); // 強制的に更新をかける
    if (success)
    {
      ntpSuccess = true;
      break; // 同期成功したらループを抜ける
    }
    delay(NTP_RETRY_INTERVAL_MS); // サーバーに負荷をかけないよう待機
  }

  // NTP同期後にWiFiを切断（省電力化）
  if (ntpSuccess)
  {
    WiFi.disconnect();
  }

  // Serial.print("現在の時刻（秒）: ");
  // Serial.println(timeClient.getEpochTime());

  // QRコードの設定（動的IPアドレスに対応）
  String url = "http://" + WiFi.localIP().toString();
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, url.c_str());

  // Serial.print("QRコードのURL: ");
  // Serial.println(url);

  // 超音波モジュールのピンに入出力を設定
  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT);

  // DACの設定
  analogWrite(A0, DAC_INITIAL_VALUE);
  wave.sine(10);

  // LEDストリップの初期化
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);

  server.begin();

  // ディスプレイに表示する関数を呼ぶ
  displayData();
}

void loop()
{
  unsigned long currentTime = millis();
  static unsigned long lastCheckTime = 0;

  // アラーム活性状態に応じて間隔を動的に変更
  bool isAnyAlarmActive = alarms[0].active || alarms[1].active;
  unsigned long interval = isAnyAlarmActive ? CHECK_INTERVAL_MS : IDLE_CHECK_INTERVAL_MS;

  if (currentTime - lastCheckTime >= interval)
  {
    checkAlarm();
    if (isAnyAlarmActive)
    {
      lastDistance = readDistance();
    }
    else
    {
      lastDistance = -1;
    }
    lastCheckTime = currentTime;
  }

  // 超音波検知でWiFi再接続（手をかざす）
  if (WiFi.status() != WL_CONNECTED &&
      currentTime - lastWiFiReconnectAttempt >= WIFI_RECONNECT_COOLDOWN_MS)
  {
    float distance = readDistance();
    if (distance > 0 && distance < DISTANCE_THRESHOLD_CM)
    {
      WiFi.begin(ssid, pass);
      unsigned long wifiStartTime = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - wifiStartTime < WIFI_TIMEOUT_MS)
      {
        delay(WIFI_RETRY_INTERVAL_MS);
      }
      if (WiFi.status() == WL_CONNECTED)
      {
        // QRコードを更新
        String url = "http://" + WiFi.localIP().toString();
        qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, url.c_str());
        lastWiFiReconnectAttempt = currentTime;
      }
    }
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClient client = server.available();
    if (client)
    {
      String currentLine = "";
      const char *weekKeys[2][7] = {
          {"CHK_MON1", "CHK_TUE1", "CHK_WED1", "CHK_THU1", "CHK_FRI1", "CHK_SAT1", "CHK_SUN1"},
          {"CHK_MON2", "CHK_TUE2", "CHK_WED2", "CHK_THU2", "CHK_FRI2", "CHK_SAT2", "CHK_SUN2"}};
      while (client.connected())
      {
        if (client.available())
        {
          char c = client.read();
          if (c == '\n')
          {
            if (currentLine.length() == 0)
            {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();
              String response = String(index_html);
              for (int i = 0; i < 2; i++)
              {
                AlarmConfig &alarm = alarms[i];

                // 繰り返しフラグ
                char repeatKey[8];
                sprintf(repeatKey, "CHK_R%d", i + 1);
                if (alarm.repeatEnabled)
                {
                  response.replace(repeatKey, "checked");
                }
                else
                {
                  response.replace(repeatKey, "");
                }

                // 時間・分
                char hKey[8], mKey[8];
                sprintf(hKey, "VAL_H%d", i + 1);
                sprintf(mKey, "VAL_M%d", i + 1);
                if (alarm.hour == -1)
                {
                  response.replace(hKey, "");
                  response.replace(mKey, "");
                }
                else
                {
                  response.replace(hKey, String(alarm.hour));
                  response.replace(mKey, String(alarm.minute));
                }

                // 曜日
                for (int d = 0; d < 7; d++)
                {
                  if (alarm.days[d])
                  {
                    response.replace(weekKeys[i][d], "checked");
                  }
                  else
                  {
                    response.replace(weekKeys[i][d], "");
                  }
                }
              }

              client.print(response);
              client.println();
              break;
            }
            else
            {
              parseAlarmSetUp(currentLine);
              currentLine = "";
            }
          }
          else if (c != '\r')
          {
            currentLine += c;
          }
        }
      }
      client.stop();
    }
  }

  if ((alarms[0].active || alarms[1].active) && lastDistance > 0 && lastDistance < DISTANCE_THRESHOLD_CM)
  {
    stopAlarm();
    delay(20);
  }
  else if (alarms[0].active)
  {
    playMusicWithAnime(*alarms[0].melody);
    updateLEDs(alarms[0].ledColors, alarms[0].ledColorCount);
  }
  else if (alarms[1].active)
  {
    playMusicWithAnime(*alarms[1].melody);
    updateLEDs(alarms[1].ledColors, alarms[1].ledColorCount);
  }
  else
  {
    stopAlarm();
    delay(20);
  }
  if (!isDisplayOn)
  {
    delay(CHECK_INTERVAL_MS);
  }
}

// アラームの時間か判定する関数
void checkAlarm()
{
  if (!timeClient.update())
  {
    DEBUG_PRINTLN("NTP update failed");
    return;
  }
  time_t epoch = timeClient.getEpochTime();
  if (epoch < NTP_EPOCH_THRESHOLD)
  {
    DEBUG_PRINTLN("Invalid epoch time");
    return;
  }
  int currentH = timeClient.getHours();
  int currentM = timeClient.getMinutes();
  int currentW = timeClient.getDay();
  checkForOLED(currentH, currentM);

  if (currentM != lastCheckedMinute)
  {
    if (!alarms[0].active && !alarms[1].active)
    {
      for (int i = 0; i < 2; i++)
      {
        AlarmConfig &alarm = alarms[i];
        if (alarm.hour == -1)
          continue;

        bool isMatch = true;
        if (alarm.repeatEnabled)
        {
          int idx = (currentW == 0) ? 6 : currentW - 1;
          isMatch = alarm.days[idx];
        }

        if (isMatch && currentH == alarm.hour && currentM == alarm.minute)
        {
          alarm.active = true;
          if (!isDisplayOn)
          {
            isDisplayOn = true;
            display.ssd1306_command(SSD1306_DISPLAYON);
          }
          currentNoteIndex = 0;
          lastCheckedMinute = currentM;

          if (!alarm.repeatEnabled)
          {
            alarm.hour = -1;
            alarm.minute = -1;
            alarm.repeatEnabled = false;
            for (int j = 0; j < 7; j++)
              alarm.days[j] = false;
          }
          break;
        }
      }
    }
  }
}

// OLEDのナイトモード
void checkForOLED(int currentH, int currentM)
{
  if (currentH == 23 && currentM == 0)
  {
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(OLED_CONTRAST_DARK);
  }
  if (currentH == 0 && currentM == 0)
  {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    isDisplayOn = false;
  }
  if (currentH == 5 && currentM == 0)
  {
    display.ssd1306_command(SSD1306_DISPLAYON);
    isDisplayOn = true;
    displayData();
  }
  if (currentH == 6 && currentM == 0)
  {
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(OLED_CONTRAST_LIGHT);
  }
}

// アラーム設定を解析する関数
void parseAlarmSetUp(String line)
{
  for (int alarmIdx = 0; alarmIdx < 2; alarmIdx++)
  {
    char hKey[6], mKey[6], repeatKey[8];
    sprintf(hKey, "h%d=", alarmIdx + 1);
    sprintf(mKey, "m%d=", alarmIdx + 1);
    sprintf(repeatKey, "repeat%d=", alarmIdx + 1);

    if (line.indexOf(hKey) != -1)
    {
      AlarmConfig &alarm = alarms[alarmIdx];

      // 繰り返し判定
      String repeatParam = String(repeatKey) + "on";
      alarm.repeatEnabled = (line.indexOf(repeatParam) != -1);

      if (alarm.repeatEnabled)
      {
        const char *days[7] = {"mon", "tue", "wed", "thu", "fri", "sat", "sun"};
        for (int i = 0; i < 7; i++)
        {
          char dayKey[8];
          sprintf(dayKey, "%s%d=", days[i], alarmIdx + 1);
          String dayParam = String(dayKey) + "on";
          alarm.days[i] = (line.indexOf(dayParam) != -1);
        }
      }
      else
      {
        for (int i = 0; i < 7; i++)
          alarm.days[i] = false;
      }

      // 数値の切り出し
      int hPos = line.indexOf(hKey) + strlen(hKey);
      int hEnd = line.indexOf("&", hPos);
      if (hEnd == -1)
        hEnd = line.indexOf(" ", hPos);

      int mPos = line.indexOf(mKey) + strlen(mKey);
      int mEnd = line.indexOf("&", mPos);
      if (mEnd == -1)
        mEnd = line.indexOf(" ", mPos);

      if (hPos > 2 && mPos > 2)
      {
        int h = line.substring(hPos, hEnd).toInt();
        int m = line.substring(mPos, mEnd).toInt();
        // 範囲チェック
        if (h >= 0 && h <= 23 && m >= 0 && m <= 59)
        {
          alarm.hour = h;
          alarm.minute = m;
        }
      }
      break;
    }
  }
}

// アラームとアニメを流す関数
void playMusicWithAnime(const ALARM &melody)
{
  unsigned long currentTime = millis();

  if (&melody == &SONG_1)
  {
    rabitAnime(currentTime);
  }
  else if (&melody == &SONG_2)
  {
    sakuraAnime(currentTime);
  }
  else
  {
    commonAnime(currentTime);
  }

  if (isWaitingNextLoop)
  {
    if (currentTime - lastNoteTime >= WAIT_TIME_MS)
    {
      isWaitingNextLoop = false;
      currentNoteIndex = 0;
      lastNoteTime = currentTime;
    }
    return;
  }
  int noteDuration = melody.tempo / melody.durations[currentNoteIndex];
  int soundActiveDuration = (melody.slur[currentNoteIndex] == 1) ? noteDuration : ((noteDuration * 9) / 10);

  if (currentTime - lastNoteTime >= soundActiveDuration)
  {
    wave.stop();
  }
  if (currentTime - lastNoteTime >= noteDuration)
  {
    lastNoteTime = currentTime;
    currentNoteIndex++;
    if (currentNoteIndex >= melody.length)
    {
      wave.stop();
      isWaitingNextLoop = true;
      return;
    }
    if (melody.notes[currentNoteIndex] != REST)
    {
      wave.freq((float)melody.notes[currentNoteIndex]);
    }
    else
    {
      wave.stop();
    }
  }
}

// アニメーション管理構造体
struct AnimationState
{
  int currentFrame;
  unsigned long lastUpdateTime;
  AnimationState() : currentFrame(0), lastUpdateTime(0) {}
};

// 共通アニメーションを流す関数
void commonAnime(unsigned long currentTime)
{
  static AnimationState state;
  if (currentTime - state.lastUpdateTime >= ANIMATION_INTERVAL_MS)
  {
    state.lastUpdateTime = currentTime;
    display.clearDisplay();

    display.drawBitmap(0, 0, circuit_commonAnime[state.currentFrame], 128, 64, WHITE);

    display.display();

    state.currentFrame = (state.currentFrame + 1) % commonAnime_frames;
  }
}

// 回る空うさぎ専用アニメーション
void rabitAnime(unsigned long currentTime)
{
  static AnimationState state;
  if (currentTime - state.lastUpdateTime >= RABBIT_ANIMATION_INTERVAL_MS)
  {
    state.lastUpdateTime = currentTime;
    display.clearDisplay();

    display.drawBitmap(0, 0, circuit_rabitAnime[state.currentFrame], 128, 64, WHITE);

    display.display();

    state.currentFrame = (state.currentFrame + 1) % rabitAnime_frames;
  }
}

// 千本桜専用アニメーション
void sakuraAnime(unsigned long currentTime)
{
  static AnimationState state;
  if (currentTime - state.lastUpdateTime >= SAKURA_ANIMATION_INTERVAL_MS)
  {
    state.lastUpdateTime = currentTime;
    display.clearDisplay();

    display.drawBitmap(0, 0, circuit_sakuraAnime[state.currentFrame], 128, 64, WHITE);

    display.display();

    state.currentFrame = (state.currentFrame + 1) % sakuraAnime_frames;
  }
}

// LEDストリップを光らせる関数
void updateLEDs(const int colorRGB[][3], int colorNum)
{
  unsigned long currentTime = millis();
  if (currentTime - lastLEDTime >= CHECK_INTERVAL_MS)
  {
    lastLEDTime = currentTime;
    targetIdx = (targetIdx + 1) % colorNum;

    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CRGB(
          colorRGB[targetIdx][0],
          colorRGB[targetIdx][1],
          colorRGB[targetIdx][2]);
    }
    FastLED.show();
  }
}

// アラームを止めたときの関数
void stopAlarm()
{
  wave.stop();
  analogWrite(A0, 0);
  alarms[0].active = false;
  alarms[1].active = false;
  currentNoteIndex = 0;
  lastNoteTime = 0;
  isWaitingNextLoop = false;
  for (int i = 0; i < NUM_LEDS; i++)
    leds[i] = CRGB::Black;
  FastLED.show();
  time_t rawtime = (time_t)timeClient.getEpochTime();
  struct tm *ti = localtime(&rawtime);
  if (ti->tm_hour >= 0 && ti->tm_hour < 5)
  {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    isDisplayOn = false;
  }
  else
  {
    // 前回表示時刻をリセットして強制更新
    lastDisplayedHour = -1;
    lastDisplayedMinute = -1;
    displayData();
  }
}

// 超音波モジュールの値から距離を計算する関数
float readDistance()
{
  digitalWrite(trigPin, LOW);  // トリガーピンを LOW にしてパルスをリセット
  delayMicroseconds(2);        // 2 マイクロ秒待つ
  digitalWrite(trigPin, HIGH); // 10 マイクロ秒のパルスを送信
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);                    // トリガーピンを LOW に戻す
  long duration = pulseIn(echoPin, HIGH, 30000); // 30msタイムアウト
  if (duration == 0)
    return -1;                                    // タイムアウト時は -1 を返す
  float distance = duration / SOUND_SPEED_FACTOR; // 計算式: (音速 340m/s → 1us あたりの距離) / 2
  return distance;
}

// ディスプレイに表示させる関数
void displayData()
{
  time_t rawtime = (time_t)timeClient.getEpochTime();
  if (rawtime < NTP_EPOCH_THRESHOLD)
  {
    return;
  }
  struct tm timeinfo;
  localtime_r(&rawtime, &timeinfo);
  struct tm *ti = &timeinfo;

  if (ti == NULL)
    return;

  // 時刻が変わっていなければ更新しない
  if (ti->tm_hour == lastDisplayedHour && ti->tm_min == lastDisplayedMinute)
  {
    return;
  }

  lastDisplayedHour = ti->tm_hour;
  lastDisplayedMinute = ti->tm_min;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE); // 白色で文字を描画
  qrdata();
  dateTimeData(ti);
  display.display();
}

// 日時を管理する関数
void dateTimeData(struct tm *ti)
{
  if (ti == NULL)
    return;

  int textX = 66;

  display.setTextSize(1);
  display.setCursor(textX, 10);

  display.print(ti->tm_year + 1900);

  display.setCursor(textX, 20);

  if (ti->tm_mon + 1 < 10)
    display.print("0");
  display.print(ti->tm_mon + 1);
  display.print("/");
  if (ti->tm_mday < 10)
    display.print("0");
  display.print(ti->tm_mday);

  display.print(" ");

  // 曜日を表示
  const char *daysOfTheWeek[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  display.print(daysOfTheWeek[ti->tm_wday]);

  display.setTextSize(2);
  display.setCursor(textX, 35);

  // 時を表示（10未満は0埋め）
  if (ti->tm_hour < 10)
    display.print("0");
  display.print(ti->tm_hour);

  display.print(":");

  // 分を表示（10未満は0埋め）
  if (ti->tm_min < 10)
    display.print("0");
  display.print(ti->tm_min);
}

// QRコードを作る関数
void qrdata()
{
  int scale = QR_CODE_SCALE; // 1セルをQR_CODE_SCALE×QR_CODE_SCALEpxで描画
  int qroffsetX = 0, qroffsetY = (SCREEN_HEIGHT - (qrcode.size * scale)) / 2;

  for (int y = 0; y < qrcode.size; y++)
  {
    for (int x = 0; x < qrcode.size; x++)
    {
      if (qrcode_getModule(&qrcode, x, y))
      {
        display.fillRect(
            qroffsetX + x * scale,
            qroffsetY + y * scale,
            scale, scale, SSD1306_WHITE);
      }
    }
  }
}
