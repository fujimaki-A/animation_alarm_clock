# コードリファクター計画

## 対象ファイル

- `code/Clock.ino` (メイン)
- 必要に応じてヘッダーファイルの拡張

## リファクター目標

- コードの保守性・信頼性の向上
- 重複コードの解消
- エラー処理の強化
- リソース管理の改善

## 具体的な変更内容

### 1. アラーム管理の構造化

**問題**: アラーム1とアラーム2の処理が重複
**解決策**:

- `AlarmConfig` 構造体を定義
- アラーム設定、状態、曜日配列を一元管理
- アラーム設定パース関数を共通化

```cpp
struct AlarmConfig {
  int hour;
  int minute;
  bool days[7];
  bool repeatEnabled;
  bool active;
};
```

### 2. アニメーション管理の改善

**問題**: `currentFrame` がグローバルで共有され、アニメーションが混ざる
**解決策**:

- アニメーションごとにフレームカウンタを分離
- `enum AnimationType { COMMON, RABBIT, SAKURA };`
- 各アニメーション関数が独自のフレームカウンタを使用

### 3. マジックナンバーの定数化

**問題**: コード中にハードコードされた数値が多い
**解決策**: トップに定数定義を追加

```cpp
const unsigned long CHECK_INTERVAL_MS = 500;
const unsigned long NTP_TIMEOUT_MS = 30000;
const unsigned long ANIMATION_INTERVAL_MS = 500;
const int LED_BRIGHTNESS = 32;
const int NUM_LEDS = 8;
```

### 4. NTP同期のタイムアウト実装

**問題**: 無限ループの可能性
**解決策**:

```cpp
unsigned long ntpStartTime = millis();
while (timeClient.getEpochTime() < 1700000000 && 
       millis() - ntpStartTime < NTP_TIMEOUT_MS) {
  timeClient.begin();
  delay(500);
  timeClient.forceUpdate();
  delay(2000);
}
if (timeClient.getEpochTime() < 1700000000) {
  // エラー処理: デフォルト時間 or 再試行
}
```

### 5. 入力検証の追加

**問題**: アラーム設定の範囲チェックなし
**解決策**: `parseAlarmSetUp` 関数内で検証

```cpp
if (h1Pos > 2 && m1Pos > 2) {
  int h = line.substring(h1Pos, h1End).toInt();
  int m = line.substring(m1Pos, m1End).toInt();
  if (h >= 0 && h <= 23 && m >= 0 && m <= 59) {
    alarm1_h = h;
    alarm1_m = m;
  }
}
```

### 6. デバッグ出力の改善

**問題**: デバッグ出力がコメントアウト or 未実装
**解決策**: `DEBUG` マクロを導入

```cpp
#define DEBUG 1
#if DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif
```

### 7. 変数名の英語化（推奨）

**問題**: 日本語と英語が混在
**解決策**: 主要変数を英語に変更（オプション）

- `lastLEDTime` → `lastLEDUpdateTime`
- `lastNoteTime` → `lastNotePlayTime`
- `lastAnimTime` → `lastAnimUpdateTime`
- `isAlarmPlaying` → `isAlarmActive` (既存の`isAlarm1Active`と統一)

### 8. arduino_secrets.h のREADMEへの記載

**問題**: 必要なヘッダーファイルが存在しない
**解決策**: README.md に作成方法を追加

## 実装順序

1. 定数定義の追加
2. アラーム構造体の導入と共通関数の作成
3. NTPタイムアウト実装
4. アニメーション管理の分離
5. 入力検証の追加
6. デバッグマクロの導入
7. README.md の更新

## テスト項目

- [ ] WiFi接続失敗時の動作確認
- [ ] NTP同期失敗時の動作確認
- [ ] アラーム設定の範囲外値のテスト
- [ ] アニメーションの正常切り替え
- [ ] Webインターフェースからの設定反映

## 注意事項

- 既存の機能を維持すること
- ハードウェア依存部分は変更しない
- メモリ使用量を増やさない（PROGMEMの活用は継続）
