#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <ArduinoJson.h>

// Firebase credentials
const char* FIREBASE_HOST = "waterdetection-1440b-default-rtdb.firebaseio.com/";
const char* FIREBASE_AUTH = "AIzaSyAFnMzHSzW2dzWhpkxbKhY-i1955zD5woc";

// Wi-Fi credentials
const char* WIFI_SSID = "Mar0";
const char* WIFI_PASSWORD = "00000000";

const int triggerPin = 32;
const int echoPin = 33;

// Database main path (to be updated in setup with the user UID)
String databasePath;

String WLPath = "/WL";

// Parent Node (to be updated in every loop)
String parentPath;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Variable to save current  time
int timestamp;

// Function that gets current epoch time
unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

String formattedTime;

void setup() {
  Serial.begin(115200);
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to Wi-Fi");

    // Update database path
  databasePath = "/readings";

  timeClient.begin();  
  timeClient.setTimeOffset(10800);
}

void loop() {
digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);

  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  unsigned long duration = pulseIn(echoPin, HIGH);
  unsigned long distance = duration * 0.034 / 2;

  if (duration == 0) {
    Serial.println("Out of range");
  } else {
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  }

timestamp = getTime();
//formattedTime = timeClient.getFormattedTime();

 // Create JSON payload
  DynamicJsonDocument jsonPayload(256);
  jsonPayload["WL"] = distance;
  jsonPayload["timestamp"] =timeClient.getFormattedTime();;
 // Serialize JSON to string
  String jsonString;
  serializeJson(jsonPayload, jsonString);

  // Write data to Firebase
  String newData = String(distance);
  
  parentPath= databasePath + "/" + String(timestamp);

  // Send data to Firebase
  String url = "https://" + String(FIREBASE_HOST) + String(parentPath) + ".json?auth=" + String(FIREBASE_AUTH);

  HTTPClient http;
  http.begin(url);

  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.PUT(jsonString);
  if (httpResponseCode == HTTP_CODE_OK) {
    Serial.println("Data sent to Firebase");
  } else {
    Serial.print("Failed to send data to Firebase. Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();

  delay(13000);
}

void realtime()
{
  timeClient.update();

  time_t epochTime = timeClient.getEpochTime();
  
  formattedTime = timeClient.getFormattedTime();

  int currentHour = timeClient.getHours();

  int currentMinute = timeClient.getMinutes();
  
  int currentSecond = timeClient.getSeconds();

  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  int monthDay = ptm->tm_mday;

  int currentMonth = ptm->tm_mon+1;

  int currentYear = ptm->tm_year+1900;
}
