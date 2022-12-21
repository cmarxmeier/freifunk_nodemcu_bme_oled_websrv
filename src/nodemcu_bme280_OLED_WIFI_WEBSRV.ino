/***************************************************************************
 * Project nodemcu_bme2809_OLED
 * Description: I2C OLED Display and BME280 on nodemcu 8266 with WIFI and Webserver
 * Author: alladin@routeme.de
 * Date: 2022/12/18
***************************************************************************/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
// Wifi
#include <ESP8266WiFi.h>
// Display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BMP_SCK 13
#define BMP_MISO 12
#define BMP_MOSI 11
#define BMP_CS 10

// define device I2C address: 0x76 or 0x77 (0x77 is library default address)
#define BMP280_I2C_ADDRESS 0x76

Adafruit_BMP280 bme;  // I2C
//Adafruit_BMP280 bme(BMP_CS); // hardware SPI
//Adafruit_BMP280 bme(BMP_CS, BMP_MOSI, BMP_MISO, BMP_SCK);

// Sensorwerte

float temp, humidity, altitude, pressure;

// Wifi Credentials
const char* ssid = "Freifunk";
const char* password = "";

// Start Webserver
WiFiServer server(80);
// Variable für den HTTP Request
String header;

void setup() {
  Serial.begin(9600);
  Serial.println(F("BMP280 test"));

  if (!bme.begin(BMP280_I2C_ADDRESS)) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1)
      ;
  }
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi verbunden");

  // Start des Servers
  server.begin();
  Serial.println("Server gestartet");

  // Print the IP address
  Serial.print("Diese URL zum Verbinden aufrufen: ");
  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.println("/");

  // Show Into
  introDisplay();
  delay(10000);
}

void loop() {
  // Get Sensor Values
  temp = bme.readTemperature();
  pressure = (bme.readPressure() / 100);
  altitude = bme.readAltitude(1021.10);  // this should be adjusted to your local forcase

  // Output webserver
 WiFiClient client = server.available();   // Auf Clients (Server-Aufrufe) warten

  if (client) {                             // Bei einem Aufruf des Servers
    Serial.println("Web-Client available");
    String currentLine = "";                // String definieren für die Anfrage des Clients

    while (client.connected()) { // Loop, solange Client verbunden ist

      if (client.available()) {
        char c = client.read();             // Ein (1) Zeichen der Anfrage des Clients lesen
        Serial.write(c);                    // und es im Seriellen Monitor ausgeben
        header += c;
        if (c == '\n') {                    // bis eine Neue Zeile ausgegeben wird

          // Wenn der Client eine Leerzeile sendet, ist das Ende des HTTP Request erreicht
          if (currentLine.length() == 0) {

            // Der Server sendet nun eine Antwort an den Client
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Die Webseite anzeigen
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\"></head>");
            client.println("<body><h1 align=\"center\">Netz.Werk.Stadt</h1>");
            client.println("<center>"); // Ausgabe zentrieren
            client.print("Temperatur = ");
            client.print(temp);
            client.println(" 'C");
            client.println("<br>");
            client.print("Luftdruck = ");
            client.print(pressure);
            client.println(" Pa");
            client.println("<br>");
            client.print("Hoehe uNN = ");
            client.print(altitude);
            client.println(" m");
            client.println("<br>");
            client.println("</center>"); // Ausgabe zentrieren Ende
            
            // Footer schreiben
            client.println("</body></html>");

            // Die Antwort mit einer Leerzeile beenden
            client.println();
            // Den Loop beenden
            break;
          } else { // Bei einer Neuen Zeile, die Variable leeren
            currentLine = "";
          }
        } else if (c != '\r') {  // alles andere als eine Leerzeile wird
          currentLine += c;      // der Variable hinzugefüht
        }
      }
    }
    // Variable für den Header leeren
    header = "";
    // Die Verbindung beenden
    client.stop();
    Serial.println("Web-Client disconnected");
    Serial.println("");
  }
  // Output Serial
  Serial.print("Temperatur = ");
  Serial.print(temp);
  Serial.println(" 'C");

  Serial.print("Luftdruck = ");
  Serial.print(pressure);
  Serial.println(" Pa");

  Serial.print("Ungefaehre Hoehe = ");
  Serial.print(altitude);
  Serial.println(" m");

  Serial.println();

  // Display Loop with delays
  updateDisplay();
  delay(5000);
  networkDisplay();
  delay(5000);
  introDisplay();
  delay(5000);
}

void introDisplay() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setFont(NULL);
  display.setCursor(10, 10);
  display.println("Netz.Werk.Stadt");
  display.display();
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setFont(NULL);
  display.setCursor(0, 0);
  display.print("Temperatur: ");
  display.print(temp);
  display.println(" 'C");
  display.println("Luftdruck: ");
  display.print(pressure);
  display.println(" hPa");
  display.print("Hoehe uNN: ");
  display.print(altitude);
  display.println(" m");
  display.display();
}

void networkDisplay() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setFont(NULL);
  display.setCursor(0, 0);
  display.println("Netzwerk-Status:");
  display.println("SSID: Freifunk");
  display.println("IP config: ");
  display.println(WiFi.localIP());
  display.display();
}
