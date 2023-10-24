// 完成品 wifi時間取得＋音声警告
#include <WiFi.h>
#include <WiFiUdp.h>
#include <time.h>  
#include "Arduino.h"
#include "NTPClient.h"
#include <HTTPClient.h>

#define buzzerPin 13
#define ledPin 26
#define on HIGH
#define off LOW

// 音階
#define BEAT 500
#define DO 261.6
#define _DO 277.18
#define RE 293.665
#define _RE 311.127
#define MI 329.63
#define FA 349.228
#define _FA 369.994
#define SO 391.995
#define _SO 415.305
#define RA 440
#define _RA 466.164
#define SI 493.883
#define octDO 523.251
#define octRE 587.330
#define octMI 659.255

// 距離センサー設定
#define echoPin 33 // INPUTA:33 INPUTB:32
#define trigPin 17 // INPUTA:17 INPUTB:19

#ifndef WIFI_SSID
#define WIFI_SSID "AP01-01" // WiFi SSID (2.4GHz only)
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "1qaz2wsx" // WiFiパスワード
#endif

// IFTTT
const String endpoint = "https://maker.ifttt.com/trigger/";
const String eventName = "SchooMyIoT";
const String conn = "/with/key/";
const String Id = "bWiJMpwaLEAe7zIE9eY42RSttG6BJFoQKbSYG6poO_-";

// NTPClient時間取得設定
WiFiUDP udp;
NTPClient ntp(udp, "ntp.nict.jp", 32400, 60000); // udp, ServerName, timeOffset, updateInterval

// Distance
double Duration = 0; // 受信した間隔
double Distance = 0; // 距離

// 音楽の内容
void playmusic() {
  ledcWriteTone(1, MI);
  delay(2000);
  ledcWriteTone(1, 0);
  delay(2000);
}

bool flag = true;

void setup() {
  // 音楽設定
  ledcSetup(1, 1200, 8);
  ledcAttachPin(buzzerPin, 1);

  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT);
  WiFi.mode(WIFI_STA);
  delay(500);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  ntp.begin();
}

void loop() {
  ntp.update();
  digitalWrite(ledPin, HIGH); // HIGHへ変更

  // 距離センサー作動
  digitalWrite(trigPin, LOW); // 超音波を停止
  delayMicroseconds(2); // 2マイクロ秒
  digitalWrite(trigPin, HIGH); // 超音波を出力
  delayMicroseconds(10);
  Duration = pulseIn(echoPin, HIGH); // センサからの入力

  if (Duration > 0) {
    Duration = Duration / 2; // 往復距離を半分
    Distance = Duration * 340 * 100 / 100000000; // 音速を340m/sに設定。時間×音速で距離が出る
    Serial.print("Distance:");
    Serial.print(Distance);
    Serial.println("m");
  }

  unsigned long epochTime = ntp.getEpochTime();
  String formattedTime = ntp.getFormattedTime(); // hh:mm:ss

  // 曜日を計算するためのtm構造体を作成し、エポック時刻から設定します
  struct tm timeinfo;
  time_t epochTime_t = static_cast<time_t>(epochTime); // エポック時刻をtime_t型にキャスト
  gmtime_r(&epochTime_t, &timeinfo);

  // 曜日の数値を取得します（0=日曜、1=月曜、...、6=土曜）
  int weekday = timeinfo.tm_wday;

  // 時刻の表示設定
  Serial.print("エポック時刻: ");
  Serial.print(epochTime);
  Serial.print("  ");
  Serial.print("時刻: ");
  Serial.print(formattedTime);
  Serial.print("  ");
  Serial.print("曜日 (0=日曜): ");
  Serial.println(weekday);

  // LINE通知
  // 特定の曜日・時刻であるか、かつtrueの場合に条件が成立。音楽が鳴りIFTTTでLINE通知
  if (Distance < 0.15 && timeinfo.tm_hour == 7 && weekday == 1 && flag) {
    Serial.print("ゴミ検知、通知します\n");
    Serial.print("Go\n");
    playmusic();
    digitalWrite(ledPin, LOW);
    delay(8000);
    digitalWrite(ledPin, HIGH); // HIGHへ変更
    delay(1000);
    flag = false; // フラグをfalseに切り替え

    // wifi経由IFTTTでLINE送信
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(endpoint + eventName + conn + Id + "?value1=trushbox_MAX");
      int httpCode = http.GET(); // GETリクエストを送信
      if (httpCode == 200) { // 返答がある場合
        Serial.println("200.wifiOK");
      } else {
        Serial.println("Error on HTTP request");
      }
      http.end(); // Free the resources
    }
  } else if (weekday != 1) {
    Serial.print("Stop\n");
    digitalWrite(ledPin, HIGH);
    flag = true; // フラグをtrueに切り替え
  }

  delay(5000);
}
