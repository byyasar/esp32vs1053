#include "Arduino.h"
#include <WiFi.h>


const char *ssid = "ASUS";
const char *password = "uTF52pvs";

bool wifiStatus = false;

/*Wifi baslangıcı*/
void WifiBaslat()
{
  int maxRetries = 3;
  int attempt = 0;
  const int timeout = 2000; // her deneme için maksimum 5 saniye bekle
  while (attempt < maxRetries)
  {
    Serial.print("Wi-Fi'ye bağlanılıyor... Deneme ");
    Serial.println(attempt + 1);
    WiFi.begin(ssid, password);
    unsigned long startAttemptTime = millis();
    // 2 saniye boyunca bağlanmaya çalış
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startAttemptTime < timeout)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("Wi-Fi'ye başarıyla bağlanıldı!");
      Serial.print("IP adresi: ");
      Serial.println(WiFi.localIP());
      wifiStatus = true;
      break;
    }
    else
    {
      Serial.println("Bağlantı başarısız.");
      attempt++;
    }
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Wi-Fi bağlantısı kurulamadı. Devam edilmiyor.");
    wifiStatus = false;
    // İstersen burada cihazı resetleyebilir veya AP moduna geçirebilirsin
  }
}
/*Wifi bitis */


  // WiFi bağlantısını kapat (pil tasarrufu için)
  // Eğer sürekli internet bağlantısı gerekiyorsa bu satırları silebilirsiniz
  // WiFi.disconnect(true);
  // WiFi.mode(WIFI_OFF);
  // Serial.println("WiFi bağlantısı kapatıldı");