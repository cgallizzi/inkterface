#include <Adafruit_ThinkInk.h>

ThinkInk_583_Mono_AAAMFGN display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  delay(2000); // startup delay for serial recon
  Serial.println("EPD Test!");
  display.begin(THINKINK_MONO);
  display.clearBuffer();
  display.display();
}

void loop() {
  drawStatic();
  display.display();
  delay(5000);
}

void drawText(const char* text, const int16_t& x = -1, const int16_t& y = -1, const uint8_t& size = 1, const uint16_t& color = EPD_BLACK, const bool& wrap = false)
{
  if (x >= 0 && y >= 0) {
    display.setCursor(x, y);
  }
  display.setTextSize(size);
  display.setTextColor(color);
  display.setTextWrap(wrap);
  display.print(text);
}

void drawLogo(const int16_t& x = 0, const int16_t& y = 0)
{
  display.fillRect(x, y, 101, 101, EPD_BLACK);
  display.fillCircle(x + 50, y + 50, 33, EPD_WHITE);
}

void drawStatic() {
  // fremont logo in top left corner
  drawLogo(5, 5);

  drawText("Connecting...", 120, 15, 3);

  // version tag
  drawText("mango-frunk gDEADBEEF", 4, display.height() - 12);
}
