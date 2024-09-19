#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define DHTPIN 5     
#define DHTTYPE DHT11   
#define SSID_AP "AP_ESP_defe"
#define PASSWORD_AP "appasworddefe"
#define SSID_STA "IoTB" 
#define PASSWORD_STA "inventaronelVAR" 

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);
HTTPClient http;

float localTemp = 0.0;
float remoteTemp = 0.0;

const char* apiEndpoint = "http://api.weatherapi.com/v1/current.json?key=a64a38502ff64afe800142623241209&q=Buenos%20Aires&aqi=no";

void handleRoot() {
  String html = "<html><head><title>Temperature Monitor</title></head><body>";
  html += "<h1>Local Temperature: " + String(localTemp) + " &deg;C</h1>";
  html += "<h1>Remote Temperature: " + String(remoteTemp) + " &deg;C</h1>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Connect to external WiFi (STA mode)
  WiFi.begin(SSID_STA, PASSWORD_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to WiFi with IP: ");
  Serial.println(WiFi.localIP());

  // Create Access Point (AP mode)
  WiFi.softAP(SSID_AP, PASSWORD_AP);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(apIP);

  // Start web server
  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient(); 

  float h = dht.readHumidity();
  localTemp = dht.readTemperature(); 
  if (isnan(h) || isnan(localTemp)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Fetch remote temperature
  http.begin(apiEndpoint);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload); 

    if (httpCode == HTTP_CODE_OK) { 
      DynamicJsonDocument doc(1024); 
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
      } else {
        remoteTemp = doc["current"]["temp_c"].as<float>(); 
        Serial.println("Remote temperature: " + String(remoteTemp));
      }
    } else {
      Serial.print("Error fetching remote temperature data (HTTP code ");
      Serial.print(httpCode);
      Serial.print("): ");
      Serial.println(http.errorToString(httpCode)); 
    }
  } else {
    Serial.println("Error connecting to the API server");
  }

  http.end();

  delay(2000); 
}
