#ifndef MELODIES_H
#define MELODIES_H

#include "pitches.h"

// 楽譜のデータをひとまとめにする構造体
typedef struct {
    const int* notes;      // メロディ配列へのポインタ
    const int* durations;  // 音価配列へのポインタ
    const int* slur;       // スラー配列のポインタ
    int length;            // 音の数
    int tempo;             // テンポ
} ALARM;

// --- 曲1のデータ ---
const int THEME_1[] = { 
  NOTE_A3, NOTE_D4, NOTE_D4, NOTE_D4, NOTE_D4, NOTE_D4, NOTE_E4, NOTE_E4, REST, NOTE_A3, NOTE_D4, NOTE_D4,
  NOTE_D4, NOTE_D4, NOTE_D4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_AS4, NOTE_GS4, NOTE_D4, NOTE_D4, NOTE_D4,
  NOTE_AS4, NOTE_GS4, NOTE_GS4, REST, NOTE_D4, NOTE_AS4, NOTE_AS4, NOTE_GS4, NOTE_D4, NOTE_D4, NOTE_D4, NOTE_AS4,
  NOTE_GS4, NOTE_GS4 
};
const int DURATIONS_1[] = { 
  8, 4, 4, 4, 4, 4, 4, 4, 8, 8, 4, 4, 4, 4, 4, 8, 8, 3, 4, 4, 8, 8, 4, 4, 8, 8, 4, 8, 8, 4, 4, 8, 8, 4, 4, 8, 8,
  4
};
const int SLUR_1[] = { 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0 
};

// --- 曲2のデータ (新しく追加するもの) ---
const int THEME_2[] = { 
  NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_AS4, NOTE_AS4, REST, NOTE_AS4, NOTE_D5, NOTE_E5, NOTE_A4,
  NOTE_G4, NOTE_AS4, NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_AS4, NOTE_AS4, REST, NOTE_AS4, NOTE_C5,
  NOTE_AS4, NOTE_A4, NOTE_G4, NOTE_G4, NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_AS4, NOTE_AS4, REST,
  NOTE_AS4, NOTE_D5, NOTE_E5, NOTE_A4, NOTE_G4, NOTE_AS4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_AS4, NOTE_A4, NOTE_G4,
  NOTE_A4, NOTE_G4, NOTE_AS4, NOTE_D5, NOTE_E5
};
const int DURATIONS_2[] = { 
  8, 8, 3, 16, 8, 8, 4, 8, 8, 8, 8, 8, 8, 4, 8, 8, 3, 16, 8, 8, 4, 8, 8, 8, 8, 8, 8, 4, 8, 8, 3, 16, 8, 8, 4, 8,
  8, 8, 8, 8, 8, 4, 8, 8, 4, 4, 4, 4, 8, 8, 8, 8, 1
};
const int SLUR_2[] = {
  0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// 構造体として実体化
const ALARM SONG_1 = { THEME_1, DURATIONS_1, SLUR_1, sizeof(THEME_1) / sizeof(THEME_1[0]), 1400 };
const ALARM SONG_2 = { THEME_2, DURATIONS_2, SLUR_2, sizeof(THEME_2) / sizeof(THEME_2[0]), 1100 };

#endif  // MELODIES_H
