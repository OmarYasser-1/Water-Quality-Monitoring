#include<SoftwareSerial.h>
#include <ArduinoJson.h>  //for json

SoftwareSerial SUART(2, 3); //SRX=Dpin-2; STX-DPin-3

//for pH sensor
unsigned long int avgValue;  //Store the average value of the sensor feedback
float b;
int buf[10],temp;

//for ds18b20 temp probe
#include <OneWire.h> 
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 8
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

//for tds sensor
#define TdsSensorPin A5
#define VREF 5.0              // analog reference voltage(Volt) of the ADC
#define SCOUNT  30            // sum of sample point

// median filtering algorithm
int getMedianNum(int bArray[], int iFilterLen){
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0){
    bTemp = bTab[(iFilterLen - 1) / 2];
  }
  else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}

//for turbidity sensor
const int sensorPin = A3;           // Analog pin connected to the sensor
const float conversionFactor = 0.0175;  // Conversion factor to convert sensor value to NTU



void setup()
{
  Serial.begin(115200); //enable Serial Monitor
  SUART.begin(115200); //enable SUART Port
  pinMode(sensorPin,INPUT) ;
sensors.begin();
    //for tds
    pinMode(TdsSensorPin,INPUT);
}

void loop() {
StaticJsonDocument<256> doc;
  JsonObject data = doc.to<JsonObject>();

//  json test***

  data["pH"]=pHvalue();
  data["tds"]=tdsVal();
  data["temperature"]=tempValue();
  data["turbidity"]=turbidityVal();
  
serializeJson(doc, SUART);
  doc.clear();
  delay(5000);
 
}


float pHvalue()
{
for(int i=0;i<10;i++)       //Get 10 sample value from the sensor for smooth the value
  { 
    buf[i]=analogRead(A1);
    delay(10);
  }
  for(int i=0;i<9;i++)        //sort the analog from small to large
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        temp=buf[i];
        buf[i]=buf[j];
        buf[j]=temp;
      }
    }
  }
  avgValue=0;
  for(int i=2;i<8;i++)                      //take the average value of 6 center sample
    avgValue+=buf[i];
  float phValue=(float)avgValue*5.0/1024/6; //convert the analog into millivolt
  phValue=3.5*phValue;                      //convert the millivolt into pH value
  Serial.print("PH is:");  
  Serial.println(phValue,2);
  return phValue ;
  delay(5000);
}


float tempValue()
{
 sensors.requestTemperatures();
 Serial.print("Temperature is: "); 
 float temp=sensors.getTempCByIndex(0);
 Serial.println(temp);
 return temp;
 delay(5000); 
}


float tdsVal()
{

int analogBuffer[SCOUNT];     // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;

float averageVoltage = 0;
float tdsValue = 0;
float temperature = tempValue() ;       // current temperature for compensation

  static unsigned long analogSampleTimepoint = millis();
  if(millis()-analogSampleTimepoint > 40U){     //every 40 milliseconds,read the analog value from the ADC
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
    analogBufferIndex++;
    if(analogBufferIndex == SCOUNT){ 
      analogBufferIndex = 0;
    }
  }   
  
  static unsigned long printTimepoint = millis();
  if(millis()-printTimepoint > 800U){
    printTimepoint = millis();
    for(copyIndex=0; copyIndex<SCOUNT; copyIndex++){
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
      
      // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 1024.0;
      
      //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0)); 
      float compensationCoefficient = 1.0+0.02*(temperature-25.0);
      //temperature compensation
      float compensationVoltage=averageVoltage/compensationCoefficient;
      
      //convert voltage value to tds value
      tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;
      
      //Serial.print("voltage:");
      //Serial.print(averageVoltage,2);
      //Serial.print("V   ");
      Serial.print("TDS Value:");
      Serial.print(tdsValue,0);
      Serial.println("ppm");
            return tdsValue;
 delay(5000); 
    }
  }
}
    


float turbidityVal()
{
  int sensorValue = analogRead(sensorPin);           // Read the sensor value
  float turbidity = sensorValue * conversionFactor;  // Convert the sensor value to NTU

  Serial.print("Turbidity is: ");
  Serial.println(turbidity, 2);   // Print turbidity with 2 decimal places
  return turbidity; 
 delay(5000); 
}

float round_to_dp( float in_value, int decimal_place )
{
  float multiplier = powf( 10.0f, decimal_place );
  in_value = roundf( in_value * multiplier )/ multiplier;
  return in_value;
}
