#include <vs1053_ext.h>
#include <vs1053b-patches-flac.h>
#include "Arduino.h"
#include "SPI.h"
#include "FS.h"
#include <Wire.h>
#include "LedControl.h"
#include "Delay.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>

const char *ssid = "ASUS";
const char *password = "uTF52pvs";
String Web_App_URL = "https://script.google.com/macros/s/AKfycbwyWqEulpnu6J-dKeLH5wqyw2gWI_kCMJkm4w4r_b-S_Bf0YmvofwO5XNXL0_ISHrtC3A/exec";

#define SD_CS 5
#define VS1053_MOSI 23
#define VS1053_MISO 19
#define VS1053_SCK 18
#define VS1053_CS 32
#define VS1053_DCS 33
#define VS1053_DREQ 35

#define BUTTON1_PIN 25
#define BUTTON2_PIN 26
#define BUTTON3_PIN 27
#define FIRCA1_PIN 12
#define FIRCA2_PIN 34

/*1- Kum saati baslangıcı*/
#define PIN_DATAIN 14 // SCK-din
#define PIN_CLK 13    // MOSI-clk
#define PIN_LOAD 15   // SS-cs
/*1- Kum saati bitis*/

VS1053 mp3(VS1053_CS, VS1053_DCS, VS1053_DREQ, VSPI, VS1053_MOSI, VS1053_MISO, VS1053_SCK);

int volume = 100;
bool isPlaying = false;
String klasor="";

/*2- Kum saati baslangıcı*/
const int xOrigin = 64;
const int yOrigin = 32;
float x, y;
#define MATRIX_A 0
#define MATRIX_B 1
#define ACC_THRESHOLD_LOW 0
#define ACC_THRESHOLD_HIGH 0
unsigned long lastDebounce = 0;
bool calis = false;
#define ROTATION_OFFSET 90

int getGravity()
{
  return 0;
}
// in milliseconds
#define DEBOUNCE_THRESHOLD 500
#define DELAY_FRAME 80
byte delayHours = 0;
byte delayMinutes = 2;
int gravity;
LedControl lc = LedControl(PIN_DATAIN, PIN_CLK, PIN_LOAD, 2);
NonBlockDelay d;
int resetCounter = 0;

long getDelayDrop()
{
  // since we have exactly 60 particles we don't have to multiply by 60 and then divide by the number of particles again :)
  return delayMinutes + delayHours * 60;
}
coord getDown(int x, int y)
{
  coord xy;
  xy.x = x - 1;
  xy.y = y + 1;
  return xy;
}
coord getLeft(int x, int y)
{
  coord xy;
  xy.x = x - 1;
  xy.y = y;
  return xy;
}
coord getRight(int x, int y)
{
  coord xy;
  xy.x = x;
  xy.y = y + 1;
  return xy;
}
bool canGoLeft(int addr, int x, int y)
{
  if (x == 0)
    return false;                        // not available
  return !lc.getXY(addr, getLeft(x, y)); // you can go there if this is empty
}
bool canGoRight(int addr, int x, int y)
{
  if (y == 7)
    return false;                         // not available
  return !lc.getXY(addr, getRight(x, y)); // you can go there if this is empty
}
bool canGoDown(int addr, int x, int y)
{
  if (y == 7)
    return false; // not available
  if (x == 0)
    return false; // not available
  if (!canGoLeft(addr, x, y))
    return false;
  if (!canGoRight(addr, x, y))
    return false;
  return !lc.getXY(addr, getDown(x, y)); // you can go there if this is empty
}
void goDown(int addr, int x, int y)
{
  lc.setXY(addr, x, y, false);
  lc.setXY(addr, getDown(x, y), true);
}
void goLeft(int addr, int x, int y)
{
  lc.setXY(addr, x, y, false);
  lc.setXY(addr, getLeft(x, y), true);
}
void goRight(int addr, int x, int y)
{
  lc.setXY(addr, x, y, false);
  lc.setXY(addr, getRight(x, y), true);
}
int countParticles(int addr)
{
  int c = 0;
  for (byte y = 0; y < 8; y++)
  {
    for (byte x = 0; x < 8; x++)
    {
      if (lc.getXY(addr, x, y))
      {
        c++;
      }
    }
  }
  return c;
}
bool moveParticle(int addr, int x, int y)
{
  if (!lc.getXY(addr, x, y))
  {
    return false;
  }

  bool can_GoLeft = canGoLeft(addr, x, y);
  bool can_GoRight = canGoRight(addr, x, y);

  if (!can_GoLeft && !can_GoRight)
  {
    return false; // we're stuck
  }

  bool can_GoDown = canGoDown(addr, x, y);

  if (can_GoDown)
  {
    goDown(addr, x, y);
  }
  else if (can_GoLeft && !can_GoRight)
  {
    goLeft(addr, x, y);
  }
  else if (can_GoRight && !can_GoLeft)
  {
    goRight(addr, x, y);
  }
  else if (random(2) == 1)
  { // we can go left and right, but not down
    goLeft(addr, x, y);
  }
  else
  {
    goRight(addr, x, y);
  }
  return true;
}
void fill(int addr, int maxcount)
{
  int n = 8;
  byte x, y;
  int count = 0;
  for (byte slice = 0; slice < 2 * n - 1; ++slice)
  {
    byte z = slice < n ? 0 : slice - n + 1;
    for (byte j = z; j <= slice - z; ++j)
    {
      y = 7 - j;
      x = (slice - j);
      lc.setXY(addr, x, y, (++count <= maxcount));
    }
  }
}
int getTopMatrix()
{
  return (getGravity() == 0) ? MATRIX_A : MATRIX_B;
}
int getBottomMatrix()
{
  return (getGravity() != 0) ? MATRIX_A : MATRIX_B;
}
void resetTime()
{
  for (byte i = 0; i < 2; i++)
  {
    lc.clearDisplay(i);
  }
  fill(getTopMatrix(), 60);

  d.Delay(getDelayDrop() * 1000);
}
bool updateMatrix()
{
  int n = 8;
  bool somethingMoved = false;
  byte x, y;
  bool direction;
  for (byte slice = 0; slice < 2 * n - 1; ++slice)
  {
    direction = (random(2) == 1); // randomize if we scan from left to right or from right to left, so the grain doesn't always fall the same direction
    byte z = slice < n ? 0 : slice - n + 1;
    for (byte j = z; j <= slice - z; ++j)
    {
      y = direction ? (7 - j) : (7 - (slice - j));
      x = direction ? (slice - j) : j;
      // for (byte d=0; d<2; d++) { lc.invertXY(0, x, y); delay(50); }
      if (moveParticle(MATRIX_B, x, y))
      {
        somethingMoved = true;
      };
      if (moveParticle(MATRIX_A, x, y))
      {
        somethingMoved = true;
      }
    }
  }
  return somethingMoved;
}
boolean dropParticle()
{
  if (d.Timeout())
  {
    d.Delay(getDelayDrop() * 1000);
    if (gravity == 0 || gravity == 180)
    {
      if ((lc.getRawXY(MATRIX_A, 0, 0) && !lc.getRawXY(MATRIX_B, 7, 7)) ||
          (!lc.getRawXY(MATRIX_A, 0, 0) && lc.getRawXY(MATRIX_B, 7, 7)))
      {
        // for (byte d=0; d<8; d++) { lc.invertXY(0, 0, 7); delay(50); }
        lc.invertRawXY(MATRIX_A, 0, 0);
        lc.invertRawXY(MATRIX_B, 7, 7);
        return true;
      }
    }
  }
  return false;
}
void resetCheck()
{
  /*int z = analogRead(A3);
  if (z > ACC_THRESHOLD_HIGH || z < ACC_THRESHOLD_LOW) {
    resetCounter++;
    Serial.println(resetCounter);
  } else {
    resetCounter = 0;
  }*/
  if (resetCounter > 20)
  {
    Serial.println("RESET!");
    resetTime();
    resetCounter = 0;
  }
}

/*2- Kum saati bitis*/

void setup()
{
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
      4096,
      NULL,
      1,
      NULL,
      1 // Core numarası (1)
  );
}

bool DatayiGonder(String deviceId)
{
  bool isSendingData = false;

  Serial.println("buton tiklandi");
  String Send_Data_URL = Web_App_URL + "?sts=write";
  Send_Data_URL += "&id=" + deviceId;

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
    isSendingData = true;
  }
  http.end();
  return isSendingData;
}

void playRandomSongFromFolder(const char *folderPath)
{
  if (mp3.isRunning())
  {
    mp3.stop_mp3client();
  }

  Serial.print("[INFO] Opening folder: ");
  Serial.println(folderPath);

  File root = SD.open(folderPath);
  if (!root || !root.isDirectory())
  {
    Serial.println("[ERROR] Folder open failed!");
    return;
  }

  int fileCount = 0;
  File file = root.openNextFile();
  while (file)
  {
    if (!file.isDirectory())
    {
      fileCount++;
      Serial.print("[INFO] Found file: ");
      Serial.println(file.name());
    }
    file = root.openNextFile();
  }
  root.rewindDirectory();

  if (fileCount == 0)
  {
    Serial.println("[ERROR] No files in folder!");
    return;
  }

  Serial.print("[INFO] Total files: ");
  Serial.println(fileCount);

  int targetIndex = random(fileCount);
  Serial.print("[INFO] Randomly selected file index: ");
  Serial.println(targetIndex);

  int currentIndex = 0;
  file = root.openNextFile();
  while (file)
  {
    if (!file.isDirectory())
    {
      if (currentIndex == targetIndex)
      {
        String path = String(folderPath) + "/" + file.name();
        Serial.print("[INFO] Now playing: ");
        Serial.println(path);
        bool durum = mp3.connecttoFS(SD, path.c_str());
        Serial.print("Durum :");
        Serial.println(durum);

        break;
      }
      currentIndex++;
    }
    file = root.openNextFile();
  }
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
      /* while (!DatayiGonder("Firca1"))
      {
      } */
      isPlaying = true;
      playRandomSongFromFolder("/1");
      klasor="/1";
      delay(100);
    }
    else if (digitalRead(FIRCA2_PIN) == HIGH && isPlaying == false)
    {
      Serial.println("FIRCA 2 cikarildi");
      /*while (!DatayiGonder("Firca2"))
       {
       }*/
      isPlaying = true;
      playRandomSongFromFolder("/2");
      klasor="/2";
      delay(100);

    }
    else if (digitalRead(FIRCA1_PIN) == LOW && digitalRead(FIRCA2_PIN) == LOW && isPlaying == true)
    { // Buton1'e basıldıysa
      Serial.println("Firca 1 yerinde");
      Serial.println("Firca 2 yerinde");
      klasor = "";
      isPlaying = false;
      mp3.stop_mp3client();
      delay(100); // Buton debouncing ve birden fazla tetiklemeyi önlemek için
    }
    // TODO: Buton1'e basıldıysa FIRÇALAR DIŞARDA İKEN YENİDEN VERİ GÖNDERİYOR DÜZELTİLECEK

    if (digitalRead(BUTTON1_PIN) == HIGH)
    { // Buton1'e basıldıysa
      Serial.println("Muzik Durduruldu");
      if (mp3.isRunning())
      {
        mp3.stop_mp3client();
      }
      delay(100);
      isPlaying = false;
      delay(100);
    }
    if (mp3.isRunning())
    {
      mp3.loop();
    }
    else
    {
      Serial.println("Muzik Durduruldu");
      const char* charUrl = klasor.c_str();
      if (klasor != "")
      {
         playRandomSongFromFolder(charUrl);
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
