#define BLYNK_TEMPLATE_ID "-----------"
#define BLYNK_TEMPLATE_NAME "------"
#define BLYNK_AUTH_TOKEN "......."


// ==== INCLUDES ====
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <IotWebConf.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>

// ==== BLYNK AUTH TOKEN ====
char blynkAuth[] = "DEIN_BLYNK_TOKEN_HIER";

// ==== DISPLAY ====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ==== SENSOR ====
#define SDA_PIN 21
#define SCL_PIN 22
#define AOUT_PIN 36
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;

// ==== IOTWEBCONF ====
const char thingName[] = "DeinSensor";
const char wifiInitialApPassword[] = "12345678";

DNSServer dnsServer;
WebServer server(80);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);

unsigned long lastBlynkAttempt = 0;
bool blynkConnected = false;

BlynkTimer timer;

// ==== SENSORWERTE ====
int soil_moist = 0;
int luftdruck = 0;
int altitude = 0;
int temperatur = 0;
int air_moist = 0;
int alert_1;

// Symbol for Wifi Connection
const unsigned char wifi_bitmap [] PROGMEM = {
0b00000000, 0b00000000, //
    0b00000111, 0b11100000, //      ######
    0b00011111, 0b11111000, //    ##########
    0b00111111, 0b11111100, //   ############
    0b01110000, 0b00001110, //  ###        ###
    0b01100111, 0b11100110, //  ##  ######  ##
    0b00001111, 0b11110000, //     ########
    0b00011000, 0b00011000, //    ##      ##
    0b00000011, 0b11000000, //       ####
    0b00000111, 0b11100000, //      ######
    0b00000100, 0b00100000, //      #    #
    0b00000001, 0b10000000, //        ##
    0b00000001, 0b10000000, //        ##
    0b00000000, 0b00000000, //
    0b00000000, 0b00000000, //
    0b00000000, 0b00000000, //
};


// ==== HILFSFUNKTIONEN ====
float calc_percent(float val)
{
  val = val - 1800;
  val = val / 2296 * 100;
  val = 100 - val;
  return val;
}

void sendSensorData()
{
  if (blynkConnected)
  {
    Blynk.virtualWrite(V0, soil_moist);
    Blynk.virtualWrite(V1, air_moist);
    Blynk.virtualWrite(V2, temperatur);
    Blynk.virtualWrite(V3, altitude);
    Blynk.virtualWrite(V4, luftdruck);
     if (alert_1== 1) // zu Trocken
      {
        Blynk.logEvent("zu_trocken"); 
      };
  }
}

// ==== SETUP ====
void setup()
{
  Serial.begin(115200);

  // Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("⚠️ SSD1306 konnte nicht gestartet werden"));
    for (;;)
      ;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  display.println("Felix sein Sensor");
  display.display();

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!bme.begin(0x77))
  {
    Serial.println("⚠️ BME280 nicht gefunden");
  }

  // WLAN Provisioning
  iotWebConf.init();
  server.on("/", []() {
    server.send(200, "text/plain", "Sensor läuft. Konfiguriere unter /config");
  });
  server.on("/config", []() { iotWebConf.handleConfig(); });
  server.onNotFound([]() { iotWebConf.handleNotFound(); });

  timer.setInterval(600000L, sendSensorData);
}

// ==== LOOP ====
void loop()
{
  iotWebConf.doLoop();
  timer.run();
  

  // Sensorwerte erfassen
  soil_moist = round(analogRead(AOUT_PIN));
  if(soil_moist >3330)
  {
    alert_1 = 1;
  }else{
    alert_1=0;
  };
  soil_moist = round(calc_percent(soil_moist));
  luftdruck = bme.readPressure() / 100.0F;
  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  temperatur = bme.readTemperature();
  air_moist = bme.readHumidity();

// Alle Display Actions

  display.clearDisplay();
  display.setCursor(0, 10);
  display.print("Soil Moisture: ");
  display.print(soil_moist);
  display.print("%");

  display.setCursor(0, 30);

  display.print("Air Humidity: ");
  display.print(air_moist);
  display.print("%");

  display.setCursor(0, 50);
  display.print("Temperatur: ");
  display.print(temperatur);
  display.print("C");


  if (WiFi.status() == WL_CONNECTED)
  {
    
    display.drawBitmap(112, 0, wifi_bitmap, 16, 16, WHITE);

    // Blynk nur einmal starten
    if (!blynkConnected && millis() - lastBlynkAttempt > 5000)
    {
      lastBlynkAttempt = millis();
      Blynk.config(BLYNK_AUTH_TOKEN);
      blynkConnected = Blynk.connect(1000); // Timeout 1 Sekunde
      if (blynkConnected)
      {
        Serial.println("✅ Blynk verbunden");
      }
      else
      {
        Serial.println("⛔ Blynk Verbindung fehlgeschlagen");
      }
    }

    Blynk.run(); // Nur wenn verbunden
  }
  else
  {
    display.drawBitmap(112, 0, wifi_bitmap, 16, 16, BLACK);
    blynkConnected = false;
  }

  display.display();
  delay(100);
}
