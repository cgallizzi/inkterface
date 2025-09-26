#include <Adafruit_ThinkInk.h>

ThinkInk_583_Mono_AAAMFGN display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

struct Point {
  int16_t x;
  int16_t y;
};

void setup() {
  Serial.begin(115200);
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

void drawSparkbox(const int16_t& x, const int16_t& y, const char* title, const char* value)
{
  display.drawRoundRect(x, y, 200, 120, 5, EPD_BLACK);
  display.fillRect(x, y + 25, 200, 1, EPD_BLACK);
  drawText(title, x + 12, y + 6, 2);
  drawText(value, (x + 190) - (12 * strlen(value)), y + 6, 2);

  Point points[] = {
    {.x = 6, .y = 100},
    {.x = 46, .y = 96},
    {.x = 86, .y = 60},
    {.x = 100, .y = 45},
    {.x = 114, .y = 110},
    {.x = 156, .y = 30},
    {.x = 194, .y = 115},
  };
  for (int i = 0; i < 6 /* len - 1 */; ++i) {
    display.drawLine(x + points[i].x, y + points[i].y, x + points[i+1].x, y + points[i+1].y, EPD_BLACK);
  }
}

void drawStatic() {
  // fremont logo in top left corner
  drawLogo(5, 5);

  // show connected fremont hostname/serial or connecting status
  drawText("Connecting...", 120, 15, 3);

  drawSparkbox(5, 115, "Temp C", "69.23");
  drawSparkbox(215, 115, "FPS", "84.6");
  drawSparkbox(425, 115, "BIOS", "F7F0123");

  // version tag
  drawText("mango-frunk gDEADBEEF", 4, display.height() - 12);
}
