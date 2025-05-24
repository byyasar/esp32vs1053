#include<WifiIslemleri.h>
#include<LedDisplayIslemleri.h>
#include<MuzikIslemleri.h>
#include<FirebaseIslemleri.h>

#define BUTTON1_PIN 25
#define BUTTON2_PIN 26
#define BUTTON3_PIN 27
#define FIRCA1_PIN 12
#define FIRCA2_PIN 34


String klasor = "";
String cihazId = "";

void setup()
{
  Serial.begin(115200);
  // Wi-Fi'ye bağlanılıyor
  WifiBaslat();

  // Türkiye saat dilimini ayarla
  setTurkeyTimezone();

  // NTP sunucusundan saat bilgisini al ve yapılandır
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Alınan saat bilgisini yazdır
  Serial.println("Türkiye saati alındı:");
  printLocalTime();



  // Firebase yapılandırması
  config.api_key = API_KEY;
  config.database_url = FIREBASE_HOST;

  // Gerekirse kimlik doğrulama
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.
  // Oturum yenilemeyi etkinleştir
  Firebase.reconnectNetwork(true);
  Serial.println("1");
  // Firebase başlat
  Firebase.begin(&config, &auth);
  Serial.println("2");

  pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);
  pinMode(BUTTON3_PIN, INPUT);
  pinMode(FIRCA1_PIN, INPUT);
  pinMode(FIRCA2_PIN, INPUT);

  /*Kum saati baslangıcı*/
  for (byte i = 0; i < 2; i++)
  {
    lc.shutdown(i, false);
    lc.setIntensity(i, 2);
  }
  resetTime();

  /*Kum saati bitis*/

  SPI.begin(VS1053_SCK, VS1053_MISO, VS1053_MOSI);
  if (!SD.begin(SD_CS, SPI, 1000000))
  { // 1 MHz SPI hızı
    Serial.println("SD Card initialization failed!");
    while (!SD.begin(SD_CS))
      ;
  }

  delay(500);
  mp3.begin();
  mp3.setVolume(volume);
  // randomSeed(analogRead(0)); // Rastgelelik için
  delay(500);
  Serial.println("ESP32 Dual-Core FreeRTOS başlıyor...");
  lc.clearDisplay(MATRIX_A);
  lc.clearDisplay(MATRIX_B);

  // Core 0’a görev atama
  xTaskCreatePinnedToCore(
      TaskCore0,        // Görev fonksiyonu
      "Task on Core 0", // Görev adı
      8192,             // Stack büyüklüğü (byte)
      NULL,             // Parametre
      1,                // Öncelik
      NULL,             // Görev tanıtıcısı (handle)
      0                 // Core numarası (0)
  );

  // Core 1’e görev atama
  xTaskCreatePinnedToCore(
      TaskCore1,
      "Task on Core 1",
      8192,
      NULL,
      1,
      NULL,
      1 // Core numarası (1)
  );
}




// ---------- Görev Fonksiyonları ----------
void TaskCore0(void *pvParameters)
{
  while (true)
  {
    // Serial.println("Core 0: LED veya sensör görevi...");
    if (digitalRead(FIRCA1_PIN) == HIGH && isPlaying == false)
    { // Buton1'e basıldıysa
      Serial.println("Firca 1 cikarildi");
      cihazId = "Firca1";
      klasor = "/1";
      delay(100);
    }
    else if (digitalRead(FIRCA2_PIN) == HIGH && isPlaying == false)
    {
      Serial.println("FIRCA 2 cikarildi");
      cihazId = "Firca2";
      klasor = "/2";
      delay(100);
    }
    else if (digitalRead(FIRCA1_PIN) == LOW && digitalRead(FIRCA2_PIN) == LOW && isPlaying == true)
    {
      Serial.println("Firca 1 yerinde");
      Serial.println("Firca 2 yerinde");
      if (calis)
      {
        Serial.println("Resetle!");
        calis = false;
        resetTime();
        delay(100);
        lc.clearDisplay(MATRIX_A);
        lc.clearDisplay(MATRIX_B);
      }
      cihazId = "";
      klasor = "";
      MuzikDurdurFnc();
    }
    // TODO: Veriyi gönder sonra müzik çal

    if (digitalRead(BUTTON1_PIN) == HIGH)
    {
      Serial.println("Muzik Durduruldu");
      MuzikDurdurFnc();
    }
    if (mp3.isRunning())
    {
      mp3.loop();
    }
    else
    {
      if (isPlaying)
      {
        Serial.println("Muzik yeniden oynatılıyor");
        const char *charUrl = klasor.c_str();
        if (klasor != "" && isPlaying == true)
        {
          playRandomSongFromFolder(charUrl);
          
          
        }
      }

      delay(100);
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void TaskCore1(void *pvParameters)
{
  while (true)
  {
    // Serial.println("Core 1: Ağ, ses, vs. görevi...");
    /*Kum saati baslangıcı*/
    if (digitalRead(BUTTON2_PIN) == HIGH)
    {
      Serial.println("Baslat!");
      for (byte i = 0; i < 2; i++)
      {
        lc.shutdown(i, false);
        lc.setIntensity(i, 2);
      }
      resetTime();
      if (cihazId != "" && wifiStatus == true)
      {
        while (!DatayiGonder(cihazId))
        {
          Serial.println("Veri gonderiliyor...");
        }
        isPlaying = true;
      }

      if (millis() - lastDebounce > DEBOUNCE_THRESHOLD)
      {
        calis = !calis;
        Serial.println(calis);
        lastDebounce = millis();
      }
    }
    if (digitalRead(BUTTON3_PIN) == HIGH)
    {
      Serial.println("Resetle!");
      for (byte i = 0; i < 2; i++)
      {
        lc.shutdown(i, false);
        lc.setIntensity(i, 2);
      }
      calis = false;
      resetTime();
    }
    if (calis)
    {
      delay(DELAY_FRAME);
      gravity = getGravity();
      lc.setRotation((ROTATION_OFFSET + gravity) % 360);
      // Serial.println(gravity);
      bool moved = updateMatrix();
      bool dropped = dropParticle();
    }
    /*Kum saati bitis*/
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// ---------- Loop ----------
void loop()
{

  // Genellikle boş bırakılır, çünkü FreeRTOS görevleri kullanılıyor
}
