#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>

const char *ssid = "Wokwi-GUEST";
const char *password = "";
// Google script Web_App_URL.
String Web_App_URL = "REPLACE_WITH_YOUR_WEB_APP_URL";

// LED pin
const int ledPin = 27; // D27 pinine bağlı LED
const int btnPin = 34; // D27 pinine bağlı LED

// LED durumunu tutacak değişken
bool ledState = false;

// CSS dosyasının içeriği
const char *css = R"rawliteral(
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin: 0;
      padding: 0;
      background-color: #f4f4f4;
    }
    h1 {
      color: #333;
    }
    p {
      font-size: 18px;
    }
    a {
      display: inline-block;
      background-color: #4CAF50;
      color: white;
      padding: 10px 20px;
      text-decoration: none;
      border-radius: 5px;
      margin: 20px;
    }
    a:hover {
      background-color: #45a049;
    }
    #ledStatus {
      font-weight: bold;
      color: #d9534f;
    }
    button {
      font-size: 18px;
      padding: 10px 20px;
      border: none;
      border-radius: 5px;
      background-color: #4CAF50;
      color: white;
      cursor: pointer;
    }
    button.off {
      background-color: #f44336;
    }
    button:hover {
      background-color: #45a049;
    }
  )rawliteral";

// Dil seçenekleri
String currentLanguage = "tr"; // Başlangıç dili Türkçe

// Dil içeriği
String getPageContent(String language)
{
  if (language == "en")
  {
    return "<html><head><meta charset=\"UTF-8\"><link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\"></head><body>"
           "<h1>ESP32 LED Toggle</h1>"
           "<p><span id=\"ledStatus\">" +
           String(ledState ? "On" : "Off") + "</span></p>"
                                             "<p><button class=\"" +
           String(ledState ? "" : "off") + "\" onclick=\"toggleLED();\">" + String(ledState ? "Turn Off" : "Turn On") + "</button></p>"
                                                                                                                        "<p><a href=\"/setLanguage?lang=tr\">Türkçe'ye Geç</a></p>"
                                                                                                                        "<script>"
                                                                                                                        "function toggleLED() {"
                                                                                                                        "  fetch('/toggle').then(response => response.text()).then(data => {"
                                                                                                                        "    document.getElementById('ledStatus').innerText = (data == 'LED On') ? 'On' : 'Off';"
                                                                                                                        "    var button = document.querySelector('button');"
                                                                                                                        "    button.className = (data == 'LED On') ? '' : 'off';"
                                                                                                                        "    button.innerText = (data == 'LED On') ? 'Turn Off' : 'Turn On';"
                                                                                                                        "  });"
                                                                                                                        "}"
                                                                                                                        "</script>"
                                                                                                                        "</body></html>";
  }
  else
  {
    return "<html><head><meta charset=\"UTF-8\"><link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\"></head><body>"
           "<h1>ESP32 LED Toggle</h1>"
           "<p><span id=\"ledStatus\">" +
           String(ledState ? "Yandı" : "Söndü") + "</span></p>"
                                                  "<p><button class=\"" +
           String(ledState ? "" : "off") + "\" onclick=\"toggleLED();\">" + String(ledState ? "Kapat" : "Aç") + "</button></p>"
                                                                                                                "<p><a href=\"/setLanguage?lang=en\">Switch to English</a></p>"
                                                                                                                "<script>"
                                                                                                                "function toggleLED() {"
                                                                                                                "  fetch('/toggle').then(response => response.text()).then(data => {"
                                                                                                                "    document.getElementById('ledStatus').innerText = (data == 'LED Yandı') ? 'Yandı' : 'Söndü';"
                                                                                                                "    var button = document.querySelector('button');"
                                                                                                                "    button.className = (data == 'LED Yandı') ? '' : 'off';"
                                                                                                                "    button.innerText = (data == 'LED Yandı') ? 'Kapat' : 'Aç';"
                                                                                                                "  });"
                                                                                                                "}"
                                                                                                                "</script>"
                                                                                                                "</body></html>";
  }
}

// Web sunucu başlatılıyor
AsyncWebServer server(80);

void setup()
{
  // Seri haberleşme başlatılıyor
  Serial.begin(115200);

  // Wi-Fi'ye bağlanılıyor
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Bağlanıyor...");
  }
  Serial.println("Bağlantı başarılı!");
  Serial.print("IP Adresi: ");
  Serial.println(WiFi.localIP());

  // LED pini çıkış olarak ayarlanıyor
  pinMode(ledPin, OUTPUT);
  pinMode(btnPin, INPUT);

  digitalWrite(ledPin, LOW); // Başlangıçta LED kapalı

  // CSS dosyasını sağlayan endpoint
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              request->send(200, "text/css", css); // CSS dosyasını gönder
            });

  // /toggle yolu için işlem tanımlanıyor
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request)
            {
      // LED'in mevcut durumu tersine çevriliyor
      ledState = !ledState;
      digitalWrite(ledPin, ledState ? HIGH : LOW);  // LED durumunu güncelliyoruz
  
      // JSON formatında yanıt gönderiliyor
      String response = ledState ? "LED Yandı" : "LED Söndü";
      request->send(200, "text/plain", response); });

  // Dil değiştirme endpoint'i
  server.on("/setLanguage", HTTP_GET, [](AsyncWebServerRequest *request)
            {
      if (request->hasParam("lang")) {
        String lang = request->getParam("lang")->value();
        currentLanguage = lang;
      }
      request->redirect("/"); });

  // Ana sayfa isteği için HTML sayfası gönderiliyor
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
      String html = getPageContent(currentLanguage);
      request->send(200, "text/html", html); });

  // Web sunucusu başlatılıyor
  server.begin();
}

void loop()
{
  // Serial.println(digitalRead(btnPin));
  if (digitalRead(btnPin) == HIGH)
  {
    Serial.println("buton tiklandi");
    String Web_App_URL = "https://script.google.com/macros/s/AKfycbwyWqEulpnu6J-dKeLH5wqyw2gWI_kCMJkm4w4r_b-S_Bf0YmvofwO5XNXL0_ISHrtC3A/exec";
    String Send_Data_URL = Web_App_URL + "?sts=write";
    Send_Data_URL += "&id=" + String("DEVICE_02");

    Serial.println();
    Serial.println("-------------");
    Serial.println("Google Spreadsheet'e Veri gonderiliyor...");
    Serial.print("URL : ");
    Serial.println(Send_Data_URL);

    // Initialize HTTPClient as "http".
    HTTPClient http;
    // HTTP GET Request.
    http.begin(Send_Data_URL.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    // Gets the HTTP status code.
    int httpCode = http.GET();
    Serial.print("HTTP Status Code : ");
    Serial.println(httpCode);
    // Getting response from google sheets.
    String payload;
    if (httpCode > 0)
    {
      payload = http.getString();
      Serial.println("Payload : " + payload);
    }
    http.end();
  }
  
}