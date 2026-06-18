/*
 * Create at 2026.06.18
 * By Halin
 * nerdminer v2 簡單的樂透礦機狀況顯示器
 * 連線pool.nerdminers.org後登入
 * BTC account 後取回資料顯示
 */

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <ArduinoJson.h>

// 引入您自訂的中文字型檔
#include "u8g2_font_.h" 
const char* ssid     = "D-Link_DIR-612";
const char* password = "27687425";

// ==================== 修改區 ====================
const char* btcAddress = "1LHoxBwmZzrfESxhF47Kthsh9pKfZ3i1Qe"; // 填入 pool.nerdminers.org 查詢用的同一個地址
// ===============================================

// 初始化 U8g2 (使用 HW I2C, 128x64 結構)
// ESP32-C3 的預設硬體 I2C：SDA 為 8, SCL 為 9
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 9, /* data=*/ 8);

const char* apiEndpoint = "https://pool.nerdminers.org/users/";
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 30000; // 每 30 秒向礦池更新一次數據

// 全域變數用於暫存礦池數據
String globalHashrate = "0 H/s";
int globalWorkers = 0;
float globalBestLuck = 0.0;
bool isDataLoaded = false;
int animStep = 0;

void setup() {
  delay(500);
  Serial.begin(115200);
  
  // 初始化 U8g2 顯示器
  u8g2.begin();
  u8g2.enableUTF8Print(); // 啟用 UTF8 支援以顯示中文
  
  // 1. 開始 WiFi 連線
  WiFi.mode(WIFI_AP_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.disconnect();
  vTaskDelay(pdMS_TO_TICKS(100));
  
if(WiFi.SSID() == ""){
  Serial.printf("connectWIFI with default:%s (%s)\n",ssid,password);
  WiFi.begin(ssid,password);  
  } else {
  Serial.printf("SSID:%s PWD:%s \n",WiFi.SSID().c_str(),WiFi.psk().c_str());
  WiFi.begin();
  }
  // 等待 5 秒確認是否能自動連上
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    vTaskDelay(pdMS_TO_TICKS(500));
    retry++;
    }
//如果沒連上網路進行自動配網設定,手機須要安裝ESPtouch App
  if (WiFi.status() != WL_CONNECTED) {

  
  // 畫面顯示引導提示
  while (WiFi.status() != WL_CONNECTED) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_custom_fonts); // 使用您的中文字型
    
    u8g2.setCursor(0, 18);
    u8g2.print("請打開手機 App");
    u8g2.setCursor(0, 38);
    u8g2.print("進行 SmartConfig");
    u8g2.setCursor(0, 58);
    u8g2.print("等待連線中");
    
    // 動態小點點
    for(int i = 0; i < (animStep % 4); i++) {
      u8g2.print(".");
    }
    u8g2.sendBuffer();
    
    animStep++;
    delay(500);
  }
  WiFi.stopSmartConfig(); // 成功連線後關閉 SmartConfig
  
 }  
  Serial.println("Wi-Fi 連線成功！");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // 定時向礦池抓取數據
  if (currentMillis - lastUpdate >= updateInterval || !isDataLoaded) {
    if (WiFi.status() == WL_CONNECTED) {
      fetchMiningData();
      lastUpdate = currentMillis;
    }
  }
  
  // 繪製即時中文監控畫面
  drawMiningScreen();
  
  animStep++;
  delay(200); // 每 0.2 秒刷新一次畫面（用於更新滾動燈）
}

// 負責抓取與解析 JSON
void fetchMiningData() {
  WiFiClientSecure client;
  client.setInsecure(); // 跳過 SSL 憑證檢查，允許直接連線 https
  
  HTTPClient http;
  String url = String(apiEndpoint) + btcAddress;
  
  // 使用包含安全客戶端 (client) 的連線方式
  http.begin(client, url); 

  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      // 讀取礦池數值
      const char* hashrate5m = doc["hashrate5m"];
      globalHashrate = String(hashrate5m) + " H/s";
      globalWorkers = doc["workers"];
      globalBestLuck = doc["bestever"];
      isDataLoaded = true;
    } else {
      Serial.println("JSON 解析失敗");
    }
  } else {
    Serial.printf("HTTP 請求失敗，錯誤碼: %d\n", httpCode);
  }
  http.end();
}

// 負責繪製中文圖形化介面
void drawMiningScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_custom_fonts); // 載入自訂的中文字型

  // 1. 頂部標題區與分隔線
  u8g2.setCursor(0, 15);
  u8g2.print("樂透礦機即時狀態");
  u8g2.drawHLine(0, 16, 128); // 畫一條橫線

  if (!isDataLoaded) {
    u8g2.setCursor(0, 31);
    u8g2.print("正在讀取礦池數據...");
  } else {
    // 2. 顯示算力
    u8g2.setCursor(0, 31);
    u8g2.print("目前算力: ");
    u8g2.print(globalHashrate);

    // 3. 顯示在線礦機數量
    u8g2.setCursor(0, 47);
    u8g2.print("在線礦機: ");
    u8g2.print(globalWorkers);
    u8g2.print(" 台");

    // 4. 顯示歷史最高幸運值
    u8g2.setCursor(0, 63);
    u8g2.print("最高幸運: ");
    u8g2.print(globalBestLuck, 5); // 顯示 5 位小數
  }

  // 5. 底部右下角加入一個閃爍跳動的即時運作小正方形（提示系統正常運作）
  if (animStep % 2 == 0) {
    u8g2.drawBox(120, 40, 5, 5);
  } else {
    u8g2.drawFrame(120, 40, 5, 5);
  }

  u8g2.sendBuffer();
}
