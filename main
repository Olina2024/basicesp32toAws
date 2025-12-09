// Include required libraries for WiFi, secure connections, MQTT, and JSON parsing
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "Wire.h"
#include "GPIO.h"
// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC "/telemetry"      // Topic for publishing sensor data
#define AWS_IOT_SUBSCRIBE_TOPIC "/downlink"     // Topic for receiving commands from AWS

long sendInterval = 60000;  // Default interval (in milliseconds) at which to send telemetry to AWS

String THINGNAME = "";  // Will store the device's unique name (derived from MAC address)

// Initialize secure WiFi client and MQTT client with 1024 byte buffer for messagesmqttrulraws iotlamcognitomes
WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(1024);
    
float readLM75Temperature() {
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
}

float readH33PHumidity(){
  
  return map(analogRead(Anlog_Pin), 0, 1023, 0, 100);;
}
// Establishes connection to AWS IoT Core via WiFi and MQTT
void connectAWS() {
  // Set WiFi to station mode (client mode, not access point)
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Get the MAC address to use as unique device identifier
  THINGNAME = WiFi.macAddress();
  // Remove colons from the MAC address string (AWS IoT thing names cannot contain colons)
  for (int i = 0; i < THINGNAME.length(); i++) {
    if (THINGNAME.charAt(i) == ':') {
      THINGNAME.remove(i, 1);
      i--;
    }
  }

  Serial.println();
  Serial.print("MAC Address: ");
  Serial.println(THINGNAME);

  Serial.println("Connecting to Wi-Fi");

  // Wait until WiFi connection is established
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials for mutual TLS authentication
  net.setCACert(AWS_CERT_CA);           // Root CA certificate
  net.setCertificate(AWS_CERT_CRT);     // Device certificate
  net.setPrivateKey(AWS_CERT_PRIVATE);  // Device private key

  // Connect to the MQTT broker on the AWS endpoint (port 8883 is the standard MQTT over TLS port)
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Register callback function to handle incoming MQTT messages
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  // Attempt to connect to AWS IoT using the thing name as client ID
  while (!client.connect(THINGNAME.c_str())) {
    Serial.print(".");
    delay(100);
  }

  // Verify connection was successful
  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to the downlink topic to receive commands from AWS
  client.subscribe(THINGNAME + AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}
void publishShadowState(float temperature, float humidity) {
  JsonDocument doc;
   
  bool temperatureAlarm = temperature > temperature_threshold;
  bool humidityAlarm = humidity > humidity_threshold;
  doc["state"]["reported"]["temperature"] = temperature;
  doc["state"]["reported"]["humidity"] = humidity;
    doc["state"]["reported"]["temperatureAlarm"] = temperatureAlarm;
  doc["state"]["reported"]["humidityAlarm"] = humidityAlarm;

  char buffer[256];
  serializeJson(doc, buffer);

  Serial.print("Updating shadow: ");
  Serial.println(buffer);

  client.publish("$aws/things/" + THINGNAME + "/shadow/update", buffer);
}



// Configures AWS IoT Device Shadow subscriptions and requests current shadow state
void setupShadow() {
  // Subscribe to shadow topics to receive shadow updates
  client.subscribe("$aws/things/" + THINGNAME + "/shadow/get/accepted");  // Receive accepted shadow get response
  client.subscribe("$aws/things/" + THINGNAME + "/shadow/get/rejected");  // Receive rejected shadow get responsedev
  //client.subscribe("$aws/things/" + THINGNAME + "/shadow/update/accepted");  // Optional: receive update confirmations
  client.subscribe("$aws/things/" + THINGNAME + "/shadow/update/delta");   // Receive delta updates (when desired state changes)

  // Request the current device shadow state from AWS
  client.publish("$aws/things/" + THINGNAME + "/shadow/get");
}

// Publishes telemetry data to AWS IoT Core
// Returns true if publish was successful, false otherwise
bool publishTelemetry(String payload) {
  Serial.print("Publishing: ");
  Serial.println(payload);
  return client.publish(THINGNAME + AWS_IOT_PUBLISH_TOPIC, payload);
}

// Callback function that handles incoming MQTT messages
void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  // Parse the JSON payload
  JsonDocument doc;
  deserializeJson(doc, payload);

  // Handle device shadow messages
  if (topic.endsWith("/shadow/get/accepted")) {
    // Received accepted shadow get response - extract desired state
    updateSettings(doc["state"]["desired"]);
  } else if (topic.endsWith("/shadow/update/delta")) {
    // Received shadow delta update - extract the changed state
    updateSettings(doc["state"]);
  }
}

// Updates device settings based on shadow document and reports back to AWS
void updateSettings(JsonDocument settingsObj) {

  Serial.println("Received DeviceShadow document: ");
  serializeJson(settingsObj, Serial);
  Serial.println();

  // Update send interval if present in shadow document (convert seconds to milliseconds)
  if (settingsObj.containsKey("sendIntervalSeconds")) {
    sendInterval = settingsObj["sendIntervalSeconds"].as<int>() * 1000;
    Serial.println("Send interval updated to: " + String(sendInterval / 1000) + " seconds");
  }

    if (settingsObj.containsKey("temperature_threshold")) {
        temperature_threshold = settingsObj["temperature_threshold"].as<float>();
    }
    if (settingsObj.containsKey("humidity_threshold")) {
        humidity_threshold = settingsObj["humidity_threshold"].as<float>();
    }
  // Create response document to report current state back to shadow
  JsonDocument docResponse;
  docResponse["state"]["reported"] = settingsObj;  // Report the received settings as current state
  char jsonBuffer[512];
  serializeJson(docResponse, jsonBuffer);

  // Report back to device shadow to acknowledge the settings were applied
  Serial.print("Sending reported state to AWS: ");
  serializeJson(docResponse, Serial);
  Serial.println();

  client.publish("$aws/things/" + THINGNAME + "/shadow/update", jsonBuffer);
}

// Arduino setup function - runs once at startup
void setup() {
  pinMode(Anlog_Pin,INPUT_PULLUP);
  Serial.begin(115200);  // Initialize serial communication for debugging
  delay(2000);            // Wait for serial monitor to connect

  Wire.begin(SDA,SCL);
  connectAWS();           // Establish connection to AWS IoT Core
  setupShadow();          // Configure and request device shadow
}

// Arduino main loop - runs continuously
void loop() {
  // Use static variable to track last send time (initialized to allow immediate first send)
  static unsigned long previousMillis = -sendInterval;
  temperature= readLM75Temperature();
  humidity=readH33PHumidity();
 
  // Check if it's time to send telemetry data
  if (millis() - previousMillis >= sendInterval) {
    // Update the last send time
    previousMillis = millis();

    // Publish simulated sensor data (temperature and humidity) as JSON
     bool sendResult = publishTelemetry("{\"temperature\":" + String(temperature, 2) + ",\"humidity\":" + String(humidity,2) + "}");
      publishShadowState(temperature, humidity);
    // Restart ESP32 if publish failed (indicates connection issue)
    if (sendResult == 0)
      ESP.restart();
  }

  // Process incoming MQTT messages and maintain connection
  client.loop();
}
