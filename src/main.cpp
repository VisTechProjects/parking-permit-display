// demo_font.cpp
#include <Arduino.h>
#include "heltec-eink-modules.h"

// https://rop.nl/truetype2gfx/ extracts fonts to GFX format
// Fonts in src/Fonts/ -- adjust names to match the files you have
#include "Fonts/FreeSansBold8pt7b.h"
#include "Fonts/Org_01.h"
#include "Fonts/FreeSans12pt7b.h"

#include "Code39Generator.h"

// Create display pointer locally (not extern)
EInkDisplay_VisionMasterE290 *display = nullptr;

const int LED_PIN = 45;

// demo strings (compact)
const char *valid_from = "Sep 05, 2025: 01:08";
const char *valid_to = "Sep 12, 2025: 01:08";

const int SCREEN_W = 296;
const int SCREEN_H = 128;

void showFontDemo(const GFXfont *font, uint8_t textSize, const char *fontName)
{
  // clear display memory / framebuffer
  display->clearMemory();

  // choose font/size
  display->setFont((GFXfont *)font);
  display->setTextSize(textSize);

  // Measure both lines
  int16_t bx, by;
  uint16_t w1, h1, w2, h2;
  display->getTextBounds((char *)valid_from, 0, 0, &bx, &by, &w1, &h1);
  display->getTextBounds((char *)valid_to, 0, 0, &bx, &by, &w2, &h2);

  Serial.print(" valid_from -> w=");
  Serial.print(w1);
  Serial.print(" h=");
  Serial.println(h1);
  Serial.print(" valid_to   -> w=");
  Serial.print(w2);
  Serial.print(" h=");
  Serial.println(h2);


  

// ========== EASILY ADJUSTABLE TEXT POSITIONS ==========
const int VALID_FROM_X = 150;              // X position for "Valid from"
const int VALID_FROM_Y = 12;               // Y position (baseline) for "Valid from"

const int VALID_TO_X = VALID_FROM_X;       // X position for "Valid to"
const int VALID_TO_Y = VALID_FROM_Y + 15;  // Y position (baseline) for "Valid to"

const int PERMIT_X = VALID_FROM_X;         // X position for "Permit #"
const int PERMIT_Y = VALID_TO_Y + 20;      // Y position (baseline) for "Permit #" (extra space)

const int PLATE_X = VALID_FROM_X;          // X position for "Plate #"
const int PLATE_Y = PERMIT_Y + 15;         // Y position (baseline) for "Plate #"
// ======================================================

// Permit data
const char *permit_no = "Permit #: T6103268";
const char *plate_no = "Plate #: CSEB187";

// Draw the lines using chosen font
display->setFont((GFXfont *)font);
display->setTextSize(textSize);

display->setCursor(VALID_FROM_X, VALID_FROM_Y);
display->print(valid_from);

display->setCursor(VALID_TO_X, VALID_TO_Y);
display->print(valid_to);

display->setCursor(PERMIT_X, PERMIT_Y);
display->print(permit_no);

display->setCursor(PLATE_X, PLATE_Y);
display->print(plate_no);





  // ========== BARCODE SETTINGS ==========
  const char *barcodeValue = "6103268"; // value encoded into the barcode
  const char *barcodeLabel = "00435";   // human-visible label shown under barcode
  int barcodeX = 0;
  int barcodeY = 0;
  int barcodeHeight = 45;
  int narrowBarWidth = 1; // Adjust this for different barcode sizes
  // ======================================

  Code39Generator barcodeGen(display);

  // Draw the barcode (encodes barcodeValue)
  barcodeGen.drawBarcode(barcodeValue, barcodeX, barcodeY, barcodeHeight, narrowBarWidth);

  // Draw the human-readable label below the barcode (shows barcodeLabel) and center it
  int barcodePixelWidth = barcodeGen.getBarcodeWidth(barcodeValue, narrowBarWidth);
  int16_t x3, y3;
  uint16_t w, h;
  display->setFont(&FreeSans12pt7b);
  display->getTextBounds(barcodeLabel, 0, 0, &x3, &y3, &w, &h);
  int labelX = barcodeX + (barcodePixelWidth / 2) - (w / 2);
  display->setCursor(labelX, barcodeY + barcodeHeight + 25);
  display->print(barcodeLabel);

  // push to e-ink
  display->update();
}

void setup()
{
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("Attempting to create display instance...");

  if (!display)
  {
    display = new EInkDisplay_VisionMasterE290();

    if (!display)
    {
      Serial.println("Library constructor failed, trying explicit fallback...");
      display = new DEPG0290BNS800(4, 3, 6);
    }
  }

  Serial.println("Display instance created.");

  display->landscape();
  display->clearMemory();

  Serial.println("Font demo starting...");

  // Try the font
  showFontDemo(&FreeSansBold8pt7b, 1, "FreeSansBold8pt7b size=1");

  Serial.println("Font demo finished.");
}

void loop()
{
  // nothing - demo ran in setup
}