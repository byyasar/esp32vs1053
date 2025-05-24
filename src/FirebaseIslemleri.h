#include "Arduino.h"
#include "time.h"
#include <FirebaseESP32.h> 
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>


#define FIREBASE_HOST "https://braishiq-default-rtdb.firebaseio.com/" // "/" ile bitmeli
#define API_KEY "AIzaSyC2aP4Q-1g9tPN4d7yOYRDFMTGp5n6jUTY"
#define USER_EMAIL "byyasar@gmail.com" // Firebase Auth email (gerekirse)
#define USER_PASSWORD "098765"         // Firebase Auth password (gerekirse)

// Firebase nesneleri
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String tarih = "";
String saat = "";

// NTP sunucu ayarları
const char *ntpServer = "pool.ntp.org";
// Türkiye saat dilimi ayarları (GMT+3)
const long gmtOffset_sec = 10800; // 3 saat * 60 dakika * 60 saniye = 10800 saniye
// Türkiye artık yaz saati uygulaması kullanmadığı için 0 olarak ayarlanmıştır
const int daylightOffset_sec = 0;

/*Tarih saat işlemleri baslangıcı*/
// Tarih ve saat bilgisini yazdıran fonksiyon
void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Saat bilgisi alınamadı!");
    return;
  }

  // Tarih ve saat bilgisini Türkçe formatla yazdır
  Serial.print(timeinfo.tm_mday);
  Serial.print("/");
  Serial.print(timeinfo.tm_mon + 1); // Ay değeri 0-11 arası olduğu için +1 ekliyoruz
  Serial.print("/");
  Serial.print(timeinfo.tm_year + 1900); // Yıl değerine 1900 eklememiz gerekiyor

  Serial.print(" ");

  Serial.print(timeinfo.tm_hour);
  Serial.print(":");
  Serial.print(timeinfo.tm_min);
  Serial.print(":");
  Serial.println(timeinfo.tm_sec);

  // Haftanın günü
  char gunler[7][12] = {"Pazar", "Pazartesi", "Salı", "Çarşamba", "Perşembe", "Cuma", "Cumartesi"};
  Serial.println(gunler[timeinfo.tm_wday]);
  // tarih = String(timeinfo.tm_year + 1900) + "-" + String(timeinfo.tm_mon + 1) + "-" + String(timeinfo.tm_mday);
  tarih = String(timeinfo.tm_mday);
  tarih.concat("/");
  tarih.concat(String(timeinfo.tm_mon + 1));
  tarih.concat("/");
  tarih.concat(String(timeinfo.tm_year + 1900));

  saat = String(timeinfo.tm_hour);
  saat.concat(":");
  saat.concat(timeinfo.tm_min);
  saat.concat(":");
  saat.concat(timeinfo.tm_sec);
}

// Türkiye saat dilimini ayarlayan fonksiyon
void setTurkeyTimezone()
{
  // Türkiye için timezone string: "EET-3"
  // EET: Eastern European Time, -3: GMT+3 (eksi işareti burada offset'i belirtir)
  setenv("TZ", "EET-3", 1);
  tzset();
}
/*Tarih saat işlemleri bitis*/


bool DatayiGonder(String deviceId)
{
  if (wifiStatus)
  {
    bool isSendingData = false;

    Serial.println("buton tiklandi");

    // ---------- FIRÇALAMA KAYDI EKLEME ----------

    printLocalTime();
    FirebaseJson json;
    json.set("tarih", tarih);
    json.set("saat", saat);
    json.set("cihaz_id", deviceId);
    String base_path = "/fircalama_kayitlari/";
    Firebase.pushJSON(fbdo, base_path, json);
    Serial.println(base_path);

    // Verileri gönder
    bool success = true;
    Serial.println("Fırçalama kaydı gönderiliyor!");

    if (success)
    {
      Serial.println("Fırçalama kaydı başarıyla gönderildi!");
      isSendingData = true;
    }
    else
    {
      Serial.println("Hata oluştu:");
      Serial.println(fbdo.errorReason());
      isSendingData = false;
    }
    return isSendingData;
  }
  else
  {
    return false;
  }
}

