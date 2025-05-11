
#include "Arduino.h"
#include "LedControl.h"
#include "Delay.h"
#include <Wire.h>

// Set the origin in the middle of the display
const int xOrigin = 64;
const int yOrigin = 32;

float x, y;

#define MATRIX_A 0
#define MATRIX_B 1

#define ACC_THRESHOLD_LOW 0
#define ACC_THRESHOLD_HIGH 0

// Matrix
// Matrix
#define PIN_DATAIN 23  // SCK
#define PIN_CLK 18     // MOSI      
#define PIN_LOAD 5        // SS

#define BUTTON_PIN 25 // Buton bağlı olduğu pini belirt
unsigned long lastDebounce = 0;
bool calis = false;

// This takes into account how the matrixes are mounted
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

/**
 * Traverse matrix and check if particles need to be moved
 */
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

/**
 * Let a particle go from one matrix to the other
 */
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

/**
 * Setup
 */
void setup()
{
  Serial.begin(9600);
  Serial.println("Welcome to the Serial Monitor!");
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Buton aktif düşük
  // init displays
  for (byte i = 0; i < 2; i++)
  {
    lc.shutdown(i, false);
    lc.setIntensity(i, 2);
  }
  resetTime();
}

/**
 * Main loop
 */
void loop()
{
  if (digitalRead(BUTTON_PIN) == LOW)
  {
    Serial.println("Baslat!");
    if (millis() - lastDebounce > DEBOUNCE_THRESHOLD)
    {
      calis=!calis;
      Serial.println(calis);
      /*resetTime();
      resetCounter = 0;
       // Yeni debounce zamanı ayarla*/
       lastDebounce = millis();
    }
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
}