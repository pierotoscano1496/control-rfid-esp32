#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

/* Put your SSID & Password */
const char *ssid = "MOVISTAR_090D";            // Enter SSID here
const char *password = "gnZCsnm4LBpVoAMh7KF8"; //Enter Password here
const char *serverBackend = "http://192.168.1.4:3001/api/check-access";

/* Pins */
const int pinSDA = 5;
const int pinRST = 4;

MFRC522 rfid(SS, pinRST);

/* Put IP Address details */
IPAddress local_ip(192, 168, 1, 12);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

void handle_OnConnect()
{
  DynamicJsonDocument doc(1024);
  String json;

  //Esperando recibir el número del tag
  int count = 0;
  while (count < 10)
  {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
    {
      count = 10;
      //Obtener todos los caracteres del tag
      String tagNumber;
      for (byte i = 0; i < rfid.uid.size; i++)
      {
        tagNumber += rfid.uid.uidByte[i];
      }
      Serial.println(tagNumber);
      doc["tag"] = tagNumber;
      doc["status"] = "OK";
    }
    else
    {
      count++;
      doc["status"] = "No tag";
    }
    delay(1000);
    count++;
  }

  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handle_checkAccess()
{
  DynamicJsonDocument doc(1024);
  String json;
  int httpCode = 0;

  //Esperando recibir el número del tag
  int count = 0;
  while (count < 10)
  {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
    {
      count = 10;

      if (WiFi.status() == WL_CONNECTED)
      {
        WiFiClient client;
        HTTPClient http;
        String tagNumber;
        String jsonBody;

        http.begin(client, serverBackend);
        http.addHeader("Content-Type", "application/json");

        //Obtener todos los caracteres del tag

        for (byte i = 0; i < rfid.uid.size; i++)
        {
          tagNumber += rfid.uid.uidByte[i];
        }
        Serial.println(tagNumber);

        doc["number"] = tagNumber;

        //Send number to server
        serializeJson(doc, jsonBody);
        httpCode = http.POST(jsonBody);
        doc.clear();

        if (httpCode == 200)
        {
          String payload = http.getString();
          Serial.println(payload);
          deserializeJson(doc, payload);
          doc["status"] = "OK";
        }
        else
        {
          doc["status"] = "Error";
        }
      }
    }
    else
    {
      count++;
      doc["status"] = "No tag";
      httpCode = 408;
    }
    delay(1000);
    count++;
  }

  serializeJson(doc, json);
  server.send(httpCode, "application/json", json);
}

void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}

void setup()
{
  Serial.begin(115200);
  SPI.begin();
  // pinMode(pinRST, OUTPUT);
  // digitalWrite(pinRST, LOW);

  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  rfid.PCD_Init();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado a la red");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.enableCORS();
  server.on("/leer-tag", handle_OnConnect);
  server.on("/check-access", handle_checkAccess);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  server.handleClient();
}