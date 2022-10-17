#include <M5Core2.h>
#include <AXP192.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secret.hpp"

#include <SHT31.h>
// #include <EEPROM.h> //CO2
// #include <sensirion_common.h>
// #include <sgp30.h>
// #include <M5_BH1750FVI.h> //照度

AXP192 power; //電源管理

SHT31 sht31 = SHT31(); //温湿度

uint16_t text_color = M5.Lcd.color565(228, 121, 17); // Amazonの色（M5の外側に合わせる）

// 照度
// M5_BH1750FVI sensor;
// uint16_t lux;

int interval = 0;

static WiFiClientSecure https_client;
static PubSubClient mqtt_client(https_client);

void setupWifi()
{
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  // 最初に一度接続をリセットさせる
  WiFi.disconnect(true);
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    M5.Lcd.print(".");
    Serial.print(".");
    delay(1000);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  M5.Lcd.println(WiFi.localIP());
}

// WiFiアクセスポイントへの再接続
void reconnectWifi()
{
  if (WiFi.status() != WL_CONNECTED) //接続が切れていたら
  {
    setupWifi(); //再接続
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Received. topic=");
  Serial.println(topic);
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.print("\n");
}

// MQTT接続する
void setupMqtt()
{
  https_client.setCACert(ROOT_CA);
  https_client.setCertificate(CERTIFICATE);
  https_client.setPrivateKey(PRIVATE_KEY);
  mqtt_client.setServer(CLOUD_ENDPOINT, CLOUD_PORT);
  // mqtt_client.setCallback(mqttCallback);
  reconnectWifi();
  while (!mqtt_client.connected())
  {
    if (!(mqtt_client.connect(DEVICE_NAME)))
    {
      M5.Lcd.printf("failed, error state =%d", mqtt_client.state());
      M5.Lcd.println(" try again in 5 seconds");
      Serial.printf("failed, error state =%d", mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// void array_to_u32(u32 *value, u8 *array)
// {
//   (*value) = (*value) | (u32)array[0] << 24;
//   (*value) = (*value) | (u32)array[1] << 16;
//   (*value) = (*value) | (u32)array[2] << 8;
//   (*value) = (*value) | (u32)array[3];
// }
// void u32_to_array(u32 value, u8 *array)
// {
//   if (!array)
//     return;
//   array[0] = value >> 24;
//   array[1] = value >> 16;
//   array[2] = value >> 8;
//   array[3] = value;
// }
// /*
//  * Reset baseline per hour,store it in EEPROM;
//  */
// void store_baseline(void)
// {
//   static u32 i = 0;
//   u32 j = 0;
//   u32 iaq_baseline = 0;
//   u8 value_array[4] = {0};
//   i++;
//   // Serial.println(i);
//   if (i == 3600)
//   {
//     i = 0;
//     if (sgp_get_iaq_baseline(&iaq_baseline) != STATUS_OK)
//     {
//       Serial.println("get baseline failed!");
//     }
//     else
//     {
//       Serial.println(iaq_baseline, HEX);
//       Serial.println("get baseline");
//       u32_to_array(iaq_baseline, value_array);
//       for (j = 0; j < 4; j++)
//       {
//         EEPROM.write(j, value_array[j]);
//         Serial.print(value_array[j]);
//         Serial.println("...");
//       }
//       EEPROM.write(j, BASELINE_IS_STORED_FLAG);
//     }
//   }
//   delay(LOOP_TIME_INTERVAL_MS);
// }
// /*Read baseline from EEPROM and set it.If there is no value in EEPROM,retrun .
//  * Another situation: When the baseline record in EEPROM is older than seven days,Discard it and return!!
//  *
//  */
// void set_baseline(void)
// {
//   u32 i = 0;
//   u8 baseline[5] = {0};
//   u32 baseline_value = 0;
//   for (i = 0; i < 5; i++)
//   {
//     baseline[i] = EEPROM.read(i);
//     Serial.print(baseline[i], HEX);
//     Serial.print("..");
//   }
//   Serial.println("!!!");
//   if (baseline[4] != BASELINE_IS_STORED_FLAG)
//   {
//     Serial.println("There is no baseline value in EEPROM");
//     return;
//   }
//   /*
//   if(baseline record in EEPROM is older than seven days)
//   {
//    return;
//    }
//    */
//   array_to_u32(&baseline_value, baseline);
//   sgp_set_iaq_baseline(baseline_value);
//   Serial.println(baseline_value, HEX);
// }

// 検知状態を送信する
void send_sensordata(float bat, float temp, float hum, float moisture)
{
  M5.Lcd.setTextSize(2); // 一行で描画できるよう文字を小さくする

  // 検知状態をJSON化する
  // ここで内容を作る
  char message[128] = {0};
  int restart_count = 0; // 再起動用カウンタ

  // JSON形式にする。完成イメージは下の通り
  //  {
  //  "temp": 23.875,
  //  "brightness": 62,
  //  "moisture": 0
  //  }
  // e.g. sprintf(json, "\"humid\":%d,\"temp_weak\":%d,",(int)DHT11.humidity,(int)DHT11.temperature);
  sprintf(message, "{\"Bat\": %.2f, \"temp\": %.2f, \"hum\": %.2f, \"moisture\": %.2f}", bat, temp, hum, moisture);

  // JSONを送信する CloudWatchには必ず送信
  while (!(mqtt_client.publish(CLOUD_TOPIC1, message)))
  {
    // 送信失敗した場合は再接続してリトライする
    mqtt_client.disconnect();
    while (!mqtt_client.connected())
    {
      if (!(mqtt_client.connect(DEVICE_NAME)))
      {
        M5.Lcd.print("#");
        Serial.println("#");
        delay(1000);
        restart_count++;
        // 30回送信を試みてダメな場合
        if (restart_count >= 30)
        {
          restart_count = 0;
          esp_restart(); // 再起動
        }
      }
    }
  }
  M5.Lcd.println("Uploaded to CloudWatch"); //送信完了
  interval++;

  // 5分おき * 12 = 60分に一回lambdaに送信
  if (interval >= 12)
  {
    // JSONを送信する
    while (!(mqtt_client.publish(CLOUD_TOPIC2, message)))
    {
      // 送信失敗した場合は再接続してリトライする
      mqtt_client.disconnect();
      while (!mqtt_client.connected())
      {
        if (!(mqtt_client.connect(DEVICE_NAME)))
        {
          M5.Lcd.print("#");
          Serial.println("#");
          delay(1000);
        }
      }
    }
    interval = 0;
    M5.Lcd.println("Uploaded to lambda"); // 送信完了
  }
}

void setup()
{
  // Serial.begin(115200);
  M5.begin();
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(text_color);

  // Get MAC address for WiFi station
  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  char baseMacChr[18] = {0};
  sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
  Serial.print("MAC: ");
  Serial.println(baseMacChr);

  setupWifi();
  setupMqtt(); // AWS IoTも止める

  //各種センサの初期化
  sht31.begin();
  pinMode(26, INPUT); // Soil moisture : Set pin 26 to input mode.
  dacWrite(25, 0);    //指定したピンにアナログDAC(Digital Analog Converter)出力

  // // CO2
  // s16 err;
  // u16 scaled_ethanol_signal, scaled_h2_signal;
  // Serial.begin(115200);
  // Serial.println("serial start!!");
  // /*Init module,Reset all baseline,The initialization takes up to around 15 seconds, during which
  // all APIs measuring IAQ(Indoor air quality ) output will not change.Default value is 400(ppm) for co2,0(ppb) for tvoc*/
  // while (sgp_probe() != STATUS_OK)
  // {
  //   Serial.println("SGP failed");
  //   while (1)
  //     ;
  // }
  // /*Read H2 and Ethanol signal in the way of blocking*/
  // err = sgp_measure_signals_blocking_read(&scaled_ethanol_signal, &scaled_h2_signal);
  // if (err == STATUS_OK)
  // {
  //   Serial.println("get ram signal!");
  // }
  // else
  // {
  //   Serial.println("error reading signals");
  // }
  // // err = sgp_iaq_init();
  // set_baseline();

  // 照度センサ
  // sensor.begin(&Wire, 21, 22);
  // sensor.setMode(CONTINUOUSLY_H_RESOLUTION_MODE);
}

void loop()
{
  M5.Lcd.clear(); //ディスプレイ表示の更新
  M5.Lcd.setTextSize(3);

  M5.Lcd.setCursor(0, 0);
  float Bat = power.GetBatteryLevel();
  M5.Lcd.printf("\nBattery:%.2f %%\n\n", Bat); //バッテリー電圧の表示

  //温湿度
  float temp = sht31.getTemperature();
  float hum = sht31.getHumidity();
  M5.Lcd.printf("Temp:%.2f C\n", temp);
  Serial.print(temp);
  Serial.println(" C");                // The unit for  Celsius because original arduino don't support speical symbols
  M5.Lcd.printf("Hum:%.2f %%\n", hum); //%%で"%"を文字列として出力
  Serial.print("Hum : ");
  Serial.print(hum);
  Serial.println("%");

  //土壌水分
  const int AirValue = 3000;   // 乾いている時の値 you need to replace this value with Value_1. default:3650
  const int WaterValue = 1400; // you need to replace this value with Value_2. default:1600
  float soilmoisture_percent;
  soilmoisture_percent = map(analogRead(36), AirValue, WaterValue, 0, 100);
  M5.Lcd.printf("Moisture:%.2f %%\n\n", soilmoisture_percent); // 0-4095. 乾いてて4095
  // M5.Lcd.printf("DigitalRead:%d\n", digitalRead(26));
  Serial.printf("AnalogRead:%d\n", analogRead(36));
  // Serial.printf("DigitalRead:%d\n", digitalRead(26));

  // // CO2
  // s16 err = 0;
  // u16 tvoc_ppb, co2_eq_ppm;
  // err = sgp_measure_iaq_blocking_read(&tvoc_ppb, &co2_eq_ppm);
  // if (err == STATUS_OK)
  // {
  //   // Serial.print("tVOC  Concentration:");
  //   // Serial.print(tvoc_ppb);
  //   // Serial.println("ppb");

  //   Serial.print("CO2eq Concentration:");
  //   Serial.print(co2_eq_ppm);
  //   Serial.println("ppm");
  //   M5.Lcd.printf("CO2: %d ppm\n", co2_eq_ppm);
  // }
  // else
  // {
  //   Serial.println("error reading IAQ values\n");
  // }
  // store_baseline();

  // 照度
  // lux = sensor.getLUX();
  // Serial.println(lux);
  // M5.Lcd.printf("lux : %d lx\n", lux);

  //データ送信．条件分岐はlambda側でやる
  // if(moisture < 20){
  send_sensordata(Bat, temp, hum, soilmoisture_percent); // コメントアウトでCloud Watchとlambdaを止める
  delay(60000 * 5);                                      // 5分おきに送信
}
