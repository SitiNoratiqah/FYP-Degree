#define BLYNK_PRINT Serial
#include <Adafruit_MCP3008.h>
#include <ESP8266HTTPClient.h>
#include "ThingSpeak.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <WiFiClientSecure.h>


/* Fill-in your Template ID (only if using Blynk.Cloud) */
#define BLYNK_TEMPLATE_ID "TMPLw_ZVyzMT"
#define BLYNK_TEMPLATE_NAME "Soil Monitor"
char auth[] = "mkBM41ffehfuO5x8_2QtJr2pERPGi7x0";

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

char ssid[] = "Kaa";   // your network SSID (name) 
char pass[] = "kkkkkkkk";   // your network password
const char* host = "script.google.com";        
String SCRIPT_ID = "AKfycbzQdK48BupOn7KU7v0hVJZyG4sOu7QDTM1kWKbvEMozmspABde4Txs-oGOS5OW2IysJOA"; //tukar
bool connectToBlynk = false;
unsigned long startTime = 0;

//MCP3008
#define CLK_PIN 14
#define MISO_PIN 12
#define MOSI_PIN 13
#define CS_PIN 15

//temperature
#define ONE_WIRE_BUS D2
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
// Pass the oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);

//pushButton
#define BUTTON_PIN D1

//LED code
#define ledR 10
#define ledG D3
#define ledY D4


//pH probe Sensor
float calibration_value = 21.34;
int phval = 0; 
unsigned long int avgval; 
int buffer_arr[10],temp;

Adafruit_MCP3008 adc;

// Define sensor thresholds
const int moistureThreshold = 30;
const int phLowerThreshold = 2;
const int phUpperThreshold = 4;
const int gasSensorThreshold = 20;
const int temperatureThreshold = 26;



//For Google
WiFiClientSecure client;

//For ThingSpeak
WiFiClient regularClient;

unsigned long myChannelNumber = 2357291;
const char * myWriteAPIKey = "65647AQ8XO0TR2I7";

void setup()
{
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Set the button pin as INPUT with internal pull-up resistor
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledY, OUTPUT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  ThingSpeak.begin(regularClient);
  //Blynk.begin(auth,ssid,pass);
  sensors.begin();
  // Setup MCP3008
  if (!adc.begin()) {
    Serial.println("Failed to initialize MCP3008!");
    while (1);
  }


}

int soilM() {
  
  //SOIL MOISTURE
  uint16_t adcValue = adc.readADC(7); // Read from CH0
  Serial.print("Raw ADC Value: ");
  Serial.println(adcValue);
  int soilMoisture = map(adcValue, 0, 1023, 0, 100); //(Declaration name, Channel, AnalogValue, Min, Max)
  Serial.print("Soil Moisture: ");
  Serial.print(soilMoisture);
  Serial.println("%");
  ThingSpeak.setField(3, soilMoisture);
  Blynk.virtualWrite(V0, soilMoisture);

  return soilMoisture;
}

int gasS() {
  
  uint16_t analogValue = adc.readADC(0); // Read from CH1
  int gasSensor = map(analogValue, 0, 1023, 0, 100);
  Serial.println(gasSensor);
  ThingSpeak.setField(1, gasSensor);
  Blynk.virtualWrite(V2, gasSensor);

  return gasSensor;
   
}

float soilTemp() {
  sensors.requestTemperatures();
  float temperatureCelsius = sensors.getTempCByIndex(0);
  
  Serial.print("Temperature: ");
  Serial.print(temperatureCelsius);
  Serial.println(" °C");
  
  ThingSpeak.setField(4, temperatureCelsius);
  Blynk.virtualWrite(V3, temperatureCelsius);

  return temperatureCelsius;
}

float phprobe()
{
  const int numReadings = 10;
  int buffer_arr[numReadings];
  int avgval = 0;

  for (int i = 0; i < numReadings; i++) {
    buffer_arr[i] = analogRead(A0);
    delay(30);
  }

  // Sort the readings and discard the highest and lowest values
  std::sort(buffer_arr, buffer_arr + numReadings);
  for (int i = 2; i < numReadings - 2; i++) {
    avgval += buffer_arr[i];
  }

  float volt = (float)avgval * (5.0 / 1024) / (numReadings - 4);
  float ph_act = -5.70 * volt + calibration_value;
  float phtotal = abs(ph_act);
  
  Serial.print("pH Value: ");
  Serial.println(phtotal);

  ThingSpeak.setField(2, phtotal);
  Blynk.virtualWrite(V1, phtotal);
  return phtotal;
}

void loop()
{  
  int soilMoisture = soilM();
  float pHValue = phprobe();
  int gasSensorReading = gasS();
  float temperatureValue = soilTemp();
  
  int buttonState = digitalRead(BUTTON_PIN); // Read the button state

// Assuming phprobe() is a function that returns the pH value

if (soilMoisture < moistureThreshold) {
  if (pHValue >= 9.7 && pHValue <= 12.8) {
    // Bauxite condition
    Serial.println("Toxic soil: Bauxite");
    ThingSpeak.setField(5, "Toxic soil: Bauxite");
    digitalWrite(ledR, HIGH);
    digitalWrite(ledG, LOW);
    digitalWrite(ledY, LOW);
  } else {
    // Suspicious contamination condition
    if (gasSensorReading > gasSensorThreshold) {
      Serial.println("Toxic soil: Suspicious Gas contamination");
      ThingSpeak.setField(5, "Toxic soil: Suspicious Gas contamination");
      digitalWrite(ledR, HIGH);
      digitalWrite(ledG, LOW);
      digitalWrite(ledY, LOW);
    } else {
      // Additional contamination condition
      if (soilMoisture < moistureThreshold && temperatureValue > temperatureThreshold) {
        Serial.println("Toxic soil: Suspicious contamination");
        ThingSpeak.setField(5, "Toxic soil: Suspicious contamination");
        digitalWrite(ledR, HIGH);
        digitalWrite(ledG, LOW);
        digitalWrite(ledY, LOW);
      } else {
        // Not contaminated
        Serial.println("Not contaminated");
        ThingSpeak.setField(5, "Not contaminated");
        digitalWrite(ledR, LOW);
        digitalWrite(ledG, HIGH);
        digitalWrite(ledY, LOW);
      }
    }
  }
} else {
  // Check for soilMoisture >= 100 condition
  if (pHValue == 7.16) {  // Fix: Add parentheses around phprobe() == 7.16
    Serial.println("Data not Valid: pH 7.16");
    ThingSpeak.setField(5, "Data not Valid: pH 7.16");
    digitalWrite(ledR, LOW);
    digitalWrite(ledG, LOW);
    digitalWrite(ledY, HIGH);
  } 
  // Check for pHValue == 7.16 condition
  else if (soilMoisture >= 90) {
    Serial.println("Data not Valid: soilMoisture too high");
    ThingSpeak.setField(5, "Data not Valid: soilMoisture too high");
    digitalWrite(ledR, LOW);
    digitalWrite(ledG, LOW);
    digitalWrite(ledY, HIGH);
  } else {
    // Not contaminated
    Serial.println("Not contaminated");
    ThingSpeak.setField(5, "Not contaminated");
    digitalWrite(ledR, LOW);
    digitalWrite(ledG, HIGH);
    digitalWrite(ledY, LOW);
  }
}

  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

    if (buttonState == LOW) {
    Serial.println("Button pressed!");
    sendToGoogleSheets();
    }
    else{
      Serial.println("Button not detected");
    }

  unsigned long currentTime = millis();
  if (currentTime - startTime < 60000) { // If less than 60 seconds have passed
    if (WiFi.status() == WL_CONNECTED && !connectToBlynk) {
      connectToBlynk = true; // Set to true after successful Wi-Fi connection within 60 seconds
      Blynk.begin(auth, ssid, pass);
    }
  }
  
  if (connectToBlynk && Blynk.connected()) {
    Blynk.run();
    Serial.println("Blynk update successful.");
  }
  else{
    Serial.println("Blynk not update successful.");
  }
  
}

void sendToGoogleSheets(){

  int read2SoilM = soilM();
  int read2Gas = gasS();
  float read2SoilT = soilTemp(); 
  float read2pH = phprobe();
  //SET CONNECTION TO INSECURE HTTPS
  client.setInsecure();

  //CONNECT TO HOST SERVER
  Serial.print("Connecting to ");
  Serial.println(host);
  
  if (!client.connect(host, 443)) {      // HTTPS Connection pada Port 443
    Serial.println("Connection failed");
    return;                              //Stop Process jika connection failed
  }
  delay(10);

  String data1 = String(read2SoilM); 
  String data2 = String(read2pH); 
  String data3 = String(read2SoilT); //convert temperature kepada format data String
  String data4 = String(read2Gas);
  
  int soilMoisture = soilM(); 
  int gasSensorReading = gasS(); 
  float pHValue = phprobe(); //convert temperature kepada format data String
  float temperatureValue = soilTemp();

  
  if (soilMoisture < moistureThreshold) {
    if (pHValue >= 9.7 && pHValue <= 12.8) {
      // Bauxite condition
      Serial.println("Toxic soil: Bauxite");
    String data = "soilMoisture="+data1+"&soil_pHSensor="+data2+"&soilTemperature="+data3+"&soil_GasValue="+data4+"&status="+1;

          String url = "/macros/s/" + SCRIPT_ID + "/exec?" + data;
          Serial.print("Requesting URL: ");
          Serial.println(url);
          requestgoogle(url);
          sendmessage();
      
    } else {
      // Suspicious contamination condition
      if (gasSensorReading > gasSensorThreshold) {
        Serial.println("Toxic soil: Suspicious Gas contamination");
    String data = "soilMoisture="+data1+"&soil_pHSensor="+data2+"&soilTemperature="+data3+"&soil_GasValue="+data4+"&status="+1;

          String url = "/macros/s/" + SCRIPT_ID + "/exec?" + data;
          Serial.print("Requesting URL: ");
          Serial.println(url);
          requestgoogle(url);
          sendmessage();
      } else {
        // Additional contamination condition
        if (soilMoisture < moistureThreshold && temperatureValue > temperatureThreshold) {
          Serial.println("Toxic soil: Suspicious contamination");
      String data = "soilMoisture="+data1+"&soil_pHSensor="+data2+"&soilTemperature="+data3+"&soil_GasValue="+data4+"&status="+1;

          String url = "/macros/s/" + SCRIPT_ID + "/exec?" + data;
          Serial.print("Requesting URL: ");
          Serial.println(url);
          requestgoogle(url);
          sendmessage();
        } else {
          // Not contaminated
          Serial.println("Not contaminated");
      String data = "soilMoisture="+data1+"&soil_pHSensor="+data2+"&soilTemperature="+data3+"&soil_GasValue="+data4+"&status="+0;

          String url = "/macros/s/" + SCRIPT_ID + "/exec?" + data;
          Serial.print("Requesting URL: ");
          Serial.println(url);
          requestgoogle(url);
          sendmessage();
        }
      }
    }
  } else {
    // Not contaminated
    Serial.println("Not contaminated");
  String data = "soilMoisture="+data1+"&soil_pHSensor="+data2+"&soilTemperature="+data3+"&soil_GasValue="+data4+"&status="+0;

          String url = "/macros/s/" + SCRIPT_ID + "/exec?" + data;
          Serial.print("Requesting URL: ");
          Serial.println(url);
          requestgoogle(url);
          sendmessage();
  }delay(1000);

}

void sendmessage(){
  
  //VARIABLE MENYIMPAN SERVER REPLY
  String fullResponse;
  String htmlResponse;
  String responseHeader;
  String responseBody;
  
  //WHILE/TUNGGU REPLY CONTENT DARI SERVER
  while (client.connected()) { //Response Code
    htmlResponse = client.readStringUntil('\r'); //Simpan Reply dalam String "line"
    break;
    //fullResponse = client.readString(); //Uncomment untuk lihat semua HTTP Response
  }//end While Client
  
  while (client.connected()) { //Response Header
    String responseHeader = client.readStringUntil('\n');
    if(responseHeader == "\r"){
      //Serial.println("Header diTerima");
      break;
    }
  }
  responseBody = client.readStringUntil('<'); //Response Body

  //CHECK/SEMAK RESPONSE CODE
  if(htmlResponse == "HTTP/1.1 200 OK"){
    Serial.println("Request diTerima Server");
    digitalWrite(ledY, HIGH);
    delay(3000);
    digitalWrite(ledY, LOW);
  }
  else{
    Serial.println("Request ERROR");
  }

  client.stop(); //DISCONNECT HTTPS connection (Wajib Disconnect)
}

void requestgoogle(String url)
{

client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266-Saya\r\n" +
               "Connection: close\r\n\r\n");

          Serial.println("Request sent");
          delay(300);

          // Properly close the connection

}
