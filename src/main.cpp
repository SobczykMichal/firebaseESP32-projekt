#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "DHT.h"

// Dane Wi-Fi
#define WIFI_SSID "Redmi Note 11S"
#define WIFI_PASSWORD "b8ccf655f68a"

// Firebase
#define API_KEY "AIzaSyCz8Y9XXUki7fwfY_J7UDhMK0P8QeAdGEM"
#define DATABASE_URL "https://fir-esp32-3d326-default-rtdb.europe-west1.firebasedatabase.app/" 

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Definicje pinów
#define DHTPIN 26
#define DHTTYPE DHT11
#define RAIN_SENSOR_PIN 36   // analogowy
#define FLAME_SENSOR_PIN 39  // analogowy
#define PIR_SENSOR_PIN 25    // cyfrowy
#define LIGHT_SENSOR_PIN 34  // analogowy czujnik natężenia światła

DHT dht(DHTPIN, DHTTYPE);

// NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000); // UTC+1 dla Polski (CET, zima)

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(PIR_SENSOR_PIN, INPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  timeClient.begin();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase signup ok");
    signupOK = true;
  } else {
    Serial.printf("Firebase signup failed: %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 900000 || sendDataPrevMillis == 0)) { // 15 minut = 900000 ms
    sendDataPrevMillis = millis();

    timeClient.update();

    // Odczyt danych z czujników
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int rainValue = analogRead(RAIN_SENSOR_PIN);
    int flameValue = analogRead(FLAME_SENSOR_PIN);
    bool motionDetected = digitalRead(PIR_SENSOR_PIN);
    int lightIntensity = analogRead(LIGHT_SENSOR_PIN);

    // Pobierz timestamp i sformatowany czas
    String timestamp = String(timeClient.getEpochTime());
    String formattedTime = timeClient.getFormattedTime();

    // Wyświetl czas w Serial Monitor
    Serial.print("Data sent at: ");
    Serial.println(formattedTime);

    String basePath = "history/" + timestamp;

    // Wysyłanie danych historycznych na Firebase
    if (Firebase.RTDB.setFloat(&fbdo, basePath + "/temperature", temperature)) {
      Serial.println("Historical Temperature sent.");
    } else {
      Serial.println("Historical Temperature FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, basePath + "/humidity", humidity)) {
      Serial.println("Historical Humidity sent.");
    } else {
      Serial.println("Historical Humidity FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, basePath + "/rain_sensor", rainValue)) {
      Serial.println("Historical Rain sensor sent.");
    } else {
      Serial.println("Historical Rain sensor FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, basePath + "/flame_sensor", flameValue)) {
      Serial.println("Historical Flame sensor sent.");
    } else {
      Serial.println("Historical Flame sensor FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setBool(&fbdo, basePath + "/motion_detected", motionDetected)) {
      Serial.println("Historical Motion status sent.");
    } else {
      Serial.println("Historical Motion status FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, basePath + "/light_intensity", lightIntensity)) {
      Serial.println("Historical Light intensity sent.");
    } else {
      Serial.println("Historical Light intensity FAILED: " + fbdo.errorReason());
    }

    // Opcjonalnie: Aktualne dane do "sensors"
    if (Firebase.RTDB.setFloat(&fbdo, "sensors/temperature", temperature)) {
      Serial.println("Live Temperature sent.");
    }
    if (Firebase.RTDB.setFloat(&fbdo, "sensors/humidity", humidity)) {
      Serial.println("Live Humidity sent.");
    }
    if (Firebase.RTDB.setInt(&fbdo, "sensors/rain_sensor", rainValue)) {
      Serial.println("Live Rain sensor sent.");
    }
    if (Firebase.RTDB.setInt(&fbdo, "sensors/flame_sensor", flameValue)) {
      Serial.println("Live Flame sensor sent.");
    }
    if (Firebase.RTDB.setBool(&fbdo, "sensors/motion_detected", motionDetected)) {
      Serial.println("Live Motion status sent.");
    }
    if (Firebase.RTDB.setInt(&fbdo, "sensors/light_intensity", lightIntensity)) {
      Serial.println("Live Light intensity sent.");
    }

    Serial.println("All data sent to Firebase.\n");
  }
}