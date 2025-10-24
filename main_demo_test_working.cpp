#include <Arduino.h>
#include "heltec-eink-modules.h"
#include "Code39Generator.h"
// Demo fonts
#include "Fonts/FreeSans24pt7b.h"
#include "Fonts/FreeSans12pt7b.h"
#include "Fonts/FreeSansBold12pt7b.h"

// Use pointer style so the constructor chosen by build_flags is used
EInkDisplay_VisionMasterE290 *display = nullptr;

const int LED_PIN = 45;

// Draw a simple filled heart using two circles + triangle
void drawHeart(int16_t cx, int16_t cy, int16_t r)
{
  int16_t rx = r;
  // lobes
  display->fillCircle(cx - rx / 2, cy - rx / 3, rx, 0x0000);
  display->fillCircle(cx + rx / 2, cy - rx / 3, rx, 0x0000);
  // bottom point
  display->fillTriangle(cx - rx, cy, cx + rx, cy, cx, cy + rx + rx / 2, 0x0000);
}

void setup()
{
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("Attempting to create display instance...");

  if (!display)
  {
    // Prefer the library-chosen constructor (selected via build_flags)
    display = new EInkDisplay_VisionMasterE290();

    // Fallback: if the build flags somehow differ or constructor not available,
    // use an explicit constructor (edit pins if your board uses different ones).
    if (!display)
    {
      Serial.println("Library constructor failed, trying explicit fallback constructor...");
      display = new DEPG0290BNS800(4 /*DC*/, 3 /*CS*/, 6 /*BUSY*/);
    }
  }

  Serial.println("Display instance created.");
  Serial.println("Preparing display (landscape, clear, text)...");

  display->landscape();
  display->clearMemory(); // clear internal memory / buffer

  // Create barcode generator instance
  Code39Generator barcodeGen(display);

  // Display some text above the barcode
  display->setFont(&FreeSans12pt7b);
  display->setTextSize(1);
  display->setCursor(10, 30);
  display->print("Parking Pass");

  // Draw sample barcode: encode `barcodeValue` but display `barcodeLabel` below
  // Decoded from permit image with zbarimg: CODE-39:6103268
  const char *barcodeValue = "6103268"; // value encoded into the barcode
  const char *barcodeLabel = "00435";   // human-visible label shown under barcode
  int barcodeX = 10;
  int barcodeY = 50;
  int barcodeHeight = 45;
  int narrowBarWidth = 1; // Adjust this for different barcode sizes

  // Draw the barcode (encodes barcodeValue)
  barcodeGen.drawBarcode(barcodeValue, barcodeX, barcodeY, barcodeHeight, narrowBarWidth);

  // Draw the human-readable label below the barcode (shows barcodeLabel) and center it
  int barcodePixelWidth = barcodeGen.getBarcodeWidth(barcodeValue, narrowBarWidth);
  int16_t x1, y1; uint16_t w, h;
  display->setFont(&FreeSans12pt7b);
  display->getTextBounds(barcodeLabel, 0, 0, &x1, &y1, &w, &h);
  int labelX = barcodeX + (barcodePixelWidth / 2) - (w / 2);
  display->setCursor(labelX, barcodeY + barcodeHeight + 25);
  display->print(barcodeLabel);

  // Update the display
  display->update();
}

void loop()
{
  // Serial.println("LED ON");
  // digitalWrite(LED_PIN, HIGH);
  // delay(500);
  // Serial.println("LED OFF");
  // digitalWrite(LED_PIN, LOW);
  // delay(500);
}
