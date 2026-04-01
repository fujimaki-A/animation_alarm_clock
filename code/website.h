#ifndef WEBSITE_H
#define WEBSITE_H

const char index_html[] PROGMEM = R"raw(
  <!DOCTYPE html>
  <html lang="ja">

  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Arduino時計 - アラーム設定</title>
    <style>
      body {
        font-family: sans-serif;
        text-align: center;
        background: #f4f4ff;
      }

      .container {
        background: #fffadd;
        padding: 20px;
        border-radius: 10px;
        display: inline-block;
        margin-top: 30px;
        box-shadow: 0 4px 6px rgba(49, 49, 51, 0.2);
        min-width: 250px;
      }

      input[type='number'] {
        width: 60px;
        font-size: 1.2rem;
        padding: 5px;
      }

      .switch {
        position: relative;
        display: inline-block;
        width: 50px;
        height: 20px
      }

      .switch input {
        display: none;
      }

      .slider {
        position: absolute;
        cursor: pointer;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: #a0d8ef;
        border-radius: 26px;
        transition: .3s;
      }

      .slider:before {
        position: absolute;
        content: "";
        height: 20px;
        width: 20px;
        left: 3px;
        bottom: 3px;
        background-color: #fd4;
        border-radius: 50%;
        transition: .3s;
      }

      input:checked+.slider {
        background-color: #4a488e;
      }

      input:checked+.slider:before {
        transform: translateX(24px);
      }

      .repeat-label {
        color: #45d;
        margin-left: 8px;
        font-size: 1.2rem;
      }

      .weekday-box {
        margin-top: 10px;
        display: flex;
        justify-content: center;
        gap: 8px;
        flex-wrap: wrap;
      }

      .weekday-box label {
        background: #fffadd;
        padding: 6px 10px;
        border-radius: 6px;
        cursor: pointer;
        font-size: 0.9rem;
        color: #45d;
        user-select: none;
      }

      .weekday-box input {
        margin-right: 4px;
      }

      input[type='submit'] {
        background: #45d;
        color: #ffffea;
        border: none;
        padding: 10px 20px;
        border-radius: 5px;
        cursor: pointer;
      }

      .outline-text {
        text-shadow:
        1px 1px 0 #f8f8f8,
        -1px 1px 0 #f8f8f8,
        1px -1px 0 #f8f8f8,
        -1px -1px 0 #f8f8f8;
      }
      </style>
  </head>

  <body>
    <div class="container">
      <h1 style="color: #45d; margin-bottom: 5px;">アラーム1</h1>
      <div class="outline-text" style="font-size: 1.2rem; color: #44617b; margin-bottom: 15px;"><b>♪回る空うさぎ</b></div>
      <form action="/save" method="get">
        <input type="number" name="h1" min="0" max="23" placeholder="時" value="VAL_H1" required>
        <input type="number" name="m1" min="0" max="59" placeholder="分" value="VAL_M1"required>
        <br><br>
        <label class="switch">
          <input type="checkbox" id="repeat" name="repeat1" CHK_R1>
          <span class="slider"></span>
        </label>
        <label for="repeat" class="repeat-label">繰り返す</label>
        <br>
        <div class="weekday-box">
          <label><input type="checkbox" name="mon1" CHK_MON1> 月</label>
          <label><input type="checkbox" name="tue1" CHK_TUE1> 火</label>
          <label><input type="checkbox" name="wed1" CHK_WED1> 水</label>
          <label><input type="checkbox" name="thu1" CHK_THU1> 木</label><br>
          <label><input type="checkbox" name="fri1" CHK_FRI1> 金</label>
          <label><input type="checkbox" name="sat1" CHK_SAT1> 土</label>
          <label><input type="checkbox" name="sun1" CHK_SUN1> 日</label>
        </div>
        <br>
        <input type="submit" value="設定を保存">
      </form>
    </div>
    <br>
    <div class="container">
      <h1 style="color: #45d; margin-bottom: 5px;">アラーム2</h1>
      <div class="outline-text" style="font-size: 1.2rem; color: #fc97ad; margin-bottom: 15px;"><b>♪千本桜</b></div>
      <form action="/save" method="get">
        <input type="number" name="h2" min="0" max="23" placeholder="時" value="VAL_H2" required>
        <input type="number" name="m2" min="0" max="59" placeholder="分" value="VAL_M2" required>
        <br><br>
        <label class="switch">
          <input type="checkbox" id="repeat" name="repeat2" CHK_R2>
          <span class="slider"></span>
        </label>
        <label for="repeat" class="repeat-label">繰り返す</label>
        <br>
        <div class="weekday-box">
          <label><input type="checkbox" name="mon2" CHK_MON2> 月</label>
          <label><input type="checkbox" name="tue2" CHK_TUE2> 火</label>
          <label><input type="checkbox" name="wed2" CHK_WED2> 水</label>
          <label><input type="checkbox" name="thu2" CHK_THU2> 木</label><br>
          <label><input type="checkbox" name="fri2" CHK_FRI2> 金</label>
          <label><input type="checkbox" name="sat2" CHK_SAT2> 土</label>
          <label><input type="checkbox" name="sun2" CHK_SUN2> 日</label>
        </div>
        <br>
        <input type="submit" value="設定を保存">
      </form>
    </div>
  </body>

  </html>
)raw";

#endif  //WEBSITE_H
