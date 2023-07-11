#include <Arduino.h>
#include<SoftwareSerial.h>
#include <ArduinoJson.h>
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "addons/TokenHelper.h"

#include "addons/RTDBHelper.h"

#define DATABASE_URL "waterdetection-1440b-default-rtdb.firebaseio.com/"                     
#define API_KEY "AIzaSyAFnMzHSzW2dzWhpkxbKhY-i1955zD5woc"   //Your Firebase Database Secret goes here

#define WIFI_SSID "Mar0"
#define WIFI_PASSWORD "00000000"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "masahmagdi7@gmail.com"
#define USER_PASSWORD "123456"

SoftwareSerial SUART(4, 5); //SRX=Dpin-D2; STX-DPin-D1

#include <OneWire.h> 
#include <DallasTemperature.h>

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;

String pHPath = "/pH";
String tempPath = "/temperature";
String turbPath = "/turbidity";
String tdsPath= "/tds";
String timePath = "/timestamp";
String dayPath = "/Day";

// Parent Node (to be updated in every loop)
String parentPath;

FirebaseJson json;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Variable to save current  time
int timestamp;

String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};


float pH;
float temperature;
float turbidity;
float tds;

String currentDate;
String formattedTime;

unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 10000;

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

bool signupOK = false;

void setup() {
  Serial.begin(115200); //enable Serial Monitor
  Serial.println("Serial communication started\n\n");  
  SUART.begin(115200); //enable SUART Port

  initWiFi();
  timeClient.begin();

 // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/readings";

  timeClient.begin();
  timeClient.setTimeOffset(10800);
}

void loop() {
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, SUART);
  if (error) {
  Serial.print(F("deserializeJson() failed: "));
  Serial.println(error.c_str());
  return;
  }
  Serial.println("JSON received and parsed");
  serializeJsonPretty(doc,SUART);
  Serial.print("PH :");
  float data1=doc["pH"];
  Serial.print(data1);
  Serial.println("");
  Serial.print("Temperature:");
  float data2=doc["temperature"];
  Serial.print(data2);
  Serial.println("");
  Serial.print("Turbidity:");
  float data3=doc["turbidity"];
  Serial.print(data3);
  Serial.println("");
  Serial.print("TDS:");
  float data4=doc["tds"];
  Serial.print(data4);
  Serial.println("ppm");

if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();


  //Get current timestamp
    timestamp = getTime();
    realtime();
  String weekDay = weekDays[timeClient.getDay()]; 


    parentPath= databasePath + "/" + String(timestamp);

    json.set(pHPath.c_str(), String(data1));
    json.set(tempPath.c_str(), String(data2));
    json.set(turbPath.c_str(), String(data3));
    json.set(tdsPath.c_str(), String(data4));
    json.set(timePath, String(formattedTime));
    json.set(dayPath, String(weekDay));
    
  Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
   delay(5000);
  } 
}



void realtime()
{
  timeClient.update();

  time_t epochTime = timeClient.getEpochTime();
  
  formattedTime = timeClient.getFormattedTime();

  int currentHour = timeClient.getHours();

  int currentMinute = timeClient.getMinutes();
  
  int currentSecond = timeClient.getSeconds();

  String weekDay = weekDays[timeClient.getDay()]; 

  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  int monthDay = ptm->tm_mday;

  int currentMonth = ptm->tm_mon+1;

  String currentMonthName = months[currentMonth-1];

  int currentYear = ptm->tm_year+1900;

  currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
}