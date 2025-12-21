#define BLYNK_TEMPLATE_ID "TMPL4VQ2tDL3T"
#define BLYNK_TEMPLATE_NAME "Esp32"
#define BLYNK_AUTH_TOKEN "jXpx3L5GRNQe48L_8S8BaRx-ajNrDQ9G"
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <Temperature_LM75_Derived.h>
#include "secret.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "Wire.h"
#include "gpio.h"
#include <BlynkSimpleEsp32.h>

// InfluxDB
#define INFLUXDB_URL "http://192.168.68.109:8086"
#define INFLUXDB_TOKEN "1K941SBKEARLoRWT_7ypQr2HPPywvkwIZOlyYXNapqyZDr4ooSVw3r6kDIPYrl-1rv3j30uBff7scztDyLEh5A=="
#define INFLUXDB_ORG "my-org"
#define INFLUXDB_BUCKET "Esp32plants_data"

InfluxDBClient client(
  INFLUXDB_URL,
  INFLUXDB_ORG,
  INFLUXDB_BUCKET,
  INFLUXDB_TOKEN
);
Point sensor("plant_data");
bool autoMode = true;  
bool tempAlertSent  = false;
bool waterAlertSent = false;
bool humidAlertSent = false;

void influxTask(void *parameter) {
  SensorData data;

  for (;;) {
    if (xQueueReceive(influxQueue, &data, portMAX_DELAY)) {

      sensor.clearFields();
      sensor.addField("temperature", data.temperature);
      sensor.addField("humidity", data.humidity);
      sensor.addField("waterlevel", data.waterlevel);

      if (!client.writePoint(sensor)) {
        Serial.print("Influx write failed: ");
        Serial.println(client.getLastErrorMessage());
      } else {
        Serial.println("Influx write OK");
      }

      yield();  // RTOS-friendly
    }
  }
}

BLYNK_WRITE(V0) {
  if (!autoMode) {
    int value = param.asInt();
    digitalWrite(DC_Pin, value ? HIGH : LOW);
    Serial.println(value ? "Pump ON (manual)" : "Pump OFF (manual)");
  }
}

BLYNK_WRITE(V4) {
  autoMode = param.asInt();
  Serial.println(autoMode ? "AUTO mode" : "MANUAL mode");
}

float readLM75Temperature() {
/*
  Wire.beginTransmission(LM75_ADDR);
  Wire.write(0x00);           // Temperature register
  Wire.endTransmission();

  Wire.requestFrom(LM75_ADDR, 2);  // LM75 gives 2 bytes

  if (Wire.available() == 2) {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();

    int16_t temp = (msb << 8) | lsb;
    temp >>= 5;                // LM75 uses 11-bit temperature
    return temp * 0.125f;      // each bit = 0.125 °C
  }

  return NAN;
  */
  return random(0,40);
}

float readWaterlevel(){

  //return map(analogRead(Anlog_Waterlevel_Pin), 0, 1023, 0,6);
   float raw = analogRead(Anlog_Waterlevel_Pin);

  // Map raw ADC to 0–4
  float level = map(raw, WATER_EMPTY, WATER_FULL, 0, 4);

  // Safety clamp
  level = constrain(level, 0, 4);

  return level;
}

float readH33PHumidity(){
  //return map(analogRead(Anlog_Pin), 0, 1023, 0, 100);;
  int humidity=map(analogRead(Anlog_Pin), DRY_SOIL, WET_SOIL, 0, 100);
  humidity = constrain(humidity, 0, 100);
  return  humidity;
}
void connectBlynk() {
  // Set WiFi to station mode (client me:\Ouyangjing\IOT24\examarbet\esp32s3\GPIO.hode, not access point)
 // WiFi.mode(WIFI_STA);
  //WiFi.begin(ssid, password);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  Blynk.syncVirtual(V0); 
  Blynk.syncVirtual(V4); 

  Serial.println("Connecting to Wi-Fi");
  // Optional: Print local IP
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  }

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(Anlog_Pin, INPUT);
  pinMode(Anlog_Waterlevel_Pin, INPUT);
  pinMode(DC_Pin, OUTPUT);
  pinMode(Fan_Pin, OUTPUT);
  pinMode(LED_Pin, OUTPUT);

  Wire.begin(SDA, SCL);
  // ---- Blynk ----
  connectBlynk();
  // ---- InfluxDB ----
  sensor.addTag("device", "esp32s3");

  if (client.validateConnection()) {
    Serial.println("InfluxDB connected");
  } else {
    Serial.println(client.getLastErrorMessage());
  }

  // ---- Queue ----
  influxQueue = xQueueCreate(5, sizeof(SensorData));
  if (influxQueue == NULL) {
    Serial.println("Queue creation failed!");
    while (1);
  }

  // ---- Task on Core 1 ----
  xTaskCreatePinnedToCore(
    influxTask,
    "InfluxTask",
    8192,
    NULL,
    1,
    &influxTaskHandle,
    1
  );

  Serial.println("System ready");
}

// Arduino main loop - runs continuously
void loop() {
  // Use static variable to track last send time (initialized to allow immediate first send)
   Blynk.run();
  
  if (millis() - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = millis();

    // ---- Read sensors ----
    temperature = readLM75Temperature();
    humidity = readH33PHumidity();
    waterlevel = readWaterlevel();


    // ---------- ALARMS (AUTO + MANUAL) ----------
   
    // Temperature alarm
    if (temperature > 28 && !tempAlertSent) {
      Blynk.logEvent("high_temperature_alert", "Temperature too high");
    
      tempAlertSent = true;
    }
    if (temperature <= 28) tempAlertSent = false;

    // Water level alarm
    if (waterlevel <= 1 && !waterAlertSent) {
      Blynk.logEvent("low_waterlevel_alert", "Water level too low");
    
      waterAlertSent = true;
    }
    if (waterlevel > 1) waterAlertSent = false;

    // Humidity alarm
    if (humidity < 40 && waterlevel > 1 && !humidAlertSent) {
      Blynk.logEvent("low_humidity_alert", "Soil is too dry");
     
      humidAlertSent = true;
    }
    if (humidity >= 40) humidAlertSent = false;
    bool alarmActive =
    (temperature > 28) ||
    (waterlevel <= 1) ||
    (humidity < 40 && waterlevel > 1);

    Blynk.virtualWrite(V5, alarmActive ? 255 : 0);

    // ---- Automatic control ----
    if (autoMode) {
       digitalWrite(Fan_Pin, temperature > 28 ? HIGH : LOW);

    if (humidity < 40 && waterlevel > 1) {
        
        digitalWrite(DC_Pin, HIGH);
        digitalWrite(LED_Pin, HIGH);
      } else {
        digitalWrite(DC_Pin, LOW);
        digitalWrite(LED_Pin, LOW);
      }
    }

    // ---- Send to Blynk ----
    Blynk.virtualWrite(V1, temperature);
    Blynk.virtualWrite(V2, humidity);
    Blynk.virtualWrite(V3, waterlevel);

    // ---- Send to Influx queue ----
    SensorData data = {temperature, humidity, waterlevel};
    xQueueSend(influxQueue, &data, 0);
  }
}
