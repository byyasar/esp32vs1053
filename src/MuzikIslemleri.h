#include "Arduino.h"
#include <vs1053_ext.h>
#include <vs1053b-patches-flac.h>
#include "FS.h"
#include "SPI.h"

#define SD_CS 5
#define VS1053_MOSI 23
#define VS1053_MISO 19
#define VS1053_SCK 18
#define VS1053_CS 32
#define VS1053_DCS 33
#define VS1053_DREQ 35

int volume = 100;
bool isPlaying = false;
String path = "";

VS1053 mp3(VS1053_CS, VS1053_DCS, VS1053_DREQ, VSPI, VS1053_MOSI, VS1053_MISO, VS1053_SCK);

void MuzikDurdurFnc()
{
  if (mp3.isRunning())
  {
    mp3.pauseResume();
  }
  isPlaying = false;
  delay(1000);
}

void playRandomSongFromFolder(const char *folderPath)
{
  Serial.print("[INFO] Opening folder: ");
  Serial.println(folderPath);

  File root = SD.open(folderPath);
  if (!root || !root.isDirectory())
  {
    Serial.println("[ERROR] Folder open failed!");
    return;
  }

  // Tüm dosyaları say
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
    file.close(); // <-- Eklendi
    file = root.openNextFile();
  }

  root.rewindDirectory(); // Klasörü sıfırla

  if (fileCount == 0)
  {
    Serial.println("[ERROR] No files in folder!");
    return;
  }

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
        path = String(folderPath);
        path.concat("/");
        path.concat(file.name());
        Serial.print("[INFO] Now playing: ");
        Serial.println(path);

        bool durum = mp3.connecttoFS(SD, path.c_str());
        if (durum)
        {
          Serial.println("[INFO] Playback started");
        }
        else
        {
          Serial.println("[ERROR] Playback failed");
        }

        file.close(); // <-- Eklendi
        break;
      }
      currentIndex++;
    }
    file.close(); // <-- Eklendi
    file = root.openNextFile();
  }

  root.close(); // <-- Kök klasörü de kapatalım
}
