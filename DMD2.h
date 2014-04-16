/*--------------------------------------------------------------------------------------

 DMD2.h   - Arduino library for the Freetronics DMD, a 512 LED matrix display
           panel arranged in a 32 x 16 layout.

 This is a non-compatible replacement for the original DMD library.
*/
#ifndef DMD2_H
#define DMD2_H

#include "Print.h"
#include "SPI.h"
#include "DMD2.h"

// Dimensions of a single display
const unsigned int WIDTH_PIXELS = 32;
const unsigned int HEIGHT_PIXELS = 16;

// Clamp a value between two limits
template<typename T> static inline void clamp(T &value, T lower, T upper) {
  if(value < lower)
    value = lower;
  else if(value > upper)
    value = upper;
}

// Swap A & B "in place" (well, with a temp variable!)
template<typename T> static inline void swap(T &a, T &b)
{
  T tmp(a); a=b; b=tmp;
}

// Check a<=b, and swap them otherwise
template<typename T> static inline void ensureOrder(T &a, T &b)
{
  if(b<a) swap(a,b);
}


enum DMDTestPattern {
  PATTERN_ALT_0,
  PATTERN_ALT_1,
  PATTERN_STRIPE_0,
  PATTERN_STRIPE_1
};

class BaseDMD
{
protected:
  BaseDMD(byte panelsWide, byte panelsHigh, byte pin_noe, byte pin_a, byte pin_b, byte pin_sck);

  virtual void writeSPIData(volatile uint8_t *rows[4], const int rowsize) = 0;
public:
  virtual void initialize();

  /* Refresh the display by manually scanning out current array of
     pixels. Call often, or use begin()/end() to automatically scan
     the display (see below.)
  */
  virtual void scanDisplay();

  /* Automatically start/stop scanning of the display output at
     flicker-free speed.  Setting this option will use Timer2 on
     AVR-based Arduinos or Timer3 on Arduino Due.
  */
  void begin();
  void end();

  // Set a single LED on or off
  void setPixel(unsigned int x, unsigned int y, const bool on);

  // Fill the screen on or off
  void fillScreen(bool on);
  inline void clearScreen() { fillScreen(false); };

  // Drawing primitives
  void drawLine(int x1, int y1, int x2, int y2, bool on = true);
  void drawCircle(unsigned int xCenter, unsigned int yCenter, int radius, bool on = true);
  void drawBox(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, bool on = true);
  void drawFilledBox(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, bool on = true);
  void drawTestPattern(DMDTestPattern pattern);

  // Text primitives
  void selectFont(const uint8_t* font);
  const inline uint8_t *getFont(void) { return font; }
  int drawChar(const int x, const int y, const char letter, bool inverse = false, const uint8_t *font = NULL);
#ifdef __AVR__
  void drawString_P(int x, int y, const char *flashStr, bool inverse = false, const uint8_t *font = NULL);

#endif
  void drawString(int x, int y, const char *bChars, bool inverse = false, const uint8_t *font = NULL);
  void drawString(int x, int y, const String &str, bool inverse = false, const uint8_t *font = NULL);

  //Find the width of a character
  int charWidth(const char letter, const uint8_t *font = NULL);

  //Find the width of a string (width of all characters plus 1 pixel "kerning" between each character)
#ifdef __AVR__
  unsigned int stringWidth_P(const char *flashStr, const uint8_t *font = NULL);
  inline unsigned int stringWidth(const __FlashStringHelper *flashStr, const uint8_t *font = NULL) {
    return stringWidth_P((const char*)flashStr, font);
  }
#endif
  unsigned int stringWidth(const char *bChars, const uint8_t *font = NULL);
  unsigned int stringWidth(const String &str, const uint8_t *font = NULL);


protected:
  volatile uint8_t *bitmap;
  volatile byte scan_row;
  byte width; // in displays not pixels
  byte height; // in displays not pixels

  byte pin_noe;
  byte pin_a;
  byte pin_b;
  byte pin_sck;

  bool default_pins; // shortcut for default pin behaviour, can use macro writes
  byte pin_other_cs; // CS pin to check before SPI behaviour, only makes sense for SPIDMD

  uint8_t *font;

  inline unsigned int total_panels() { return width * height; };
  inline size_t bitmap_bytes() { return total_panels() * WIDTH_PIXELS * HEIGHT_PIXELS / 8; };
  // Pixel dimensions:
  inline unsigned int total_width() { return width * WIDTH_PIXELS; };
  inline unsigned int total_height() { return height * HEIGHT_PIXELS; };
  inline unsigned int unified_width() { return total_panels() * WIDTH_PIXELS; }; // width of all displays as seen by controller

  template<typename T> inline void clamp_xy(T &x, T&y) {
    clamp(x, (T)0, total_width()-1);
    clamp(y, (T)0, total_height()-1);
  }
};

class SPIDMD : public BaseDMD
{
public:
  /* Create a single DMD display */
  SPIDMD();
  /* Create a DMD display using the default pinout, panelsWide x panelsHigh panels in size */
  SPIDMD(byte panelsWide, byte panelsHigh);
  /* Create a DMD display using a custom pinout for all the non-SPI pins (SPI pins set by hardware) */
  SPIDMD(byte panelsWide, byte panelsHigh, byte pin_noe, byte pin_a, byte pin_b, byte pin_sck);

  void initialize();

  /* Set the "other CS" pin that is checked for in use before scanning the DMD */
  void setOtherCS(byte pin_other_cs) { this->pin_other_cs = pin_other_cs; }

protected:
  void writeSPIData(volatile uint8_t *rows[4], const int rowsize);
};

class SoftDMD : public BaseDMD
{
public:
  SoftDMD(byte panelsWide, byte panelsHigh);
  SoftDMD(byte panelsWide, byte panelsHigh, byte pin_noe, byte pin_a, byte pin_b, byte pin_sck,
          byte pin_clk, byte pin_r_data);

  void initialize();

protected:
  void writeSPIData(volatile uint8_t *rows[4], const int rowsize);
private:
  inline void softSPITransfer(uint8_t byte) __attribute__((always_inline));
  byte pin_clk;
  byte pin_r_data;
};

#endif