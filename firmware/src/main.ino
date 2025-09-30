#include <Adafruit_ThinkInk.h>
#include <NimBLEDevice.h>

#define SERVICE_UUID "95c7b479-8e84-4ce7-a121-faf74bf48c84"
#define CHARACTERISTIC_UUID "d6f4c07e-4a21-4c69-bd15-43a38a8719e6"

NimBLEServer *BLE_SERVER = nullptr;

ThinkInk_583_Mono_AAAMFGN MF_DISPLAY(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

struct Point {
    float x;
    float y;
};
typedef std::vector<Point> Points;

void drawStatic(const char *status);

class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *server, NimBLEConnInfo &conn) override
    {
        // we don't want any other devices to see us once we are connected
        // to a host
        NimBLEDevice::stopAdvertising();
        Serial.println("got connection");
    }

    void onDisconnect(NimBLEServer *server, NimBLEConnInfo &conn, int reason) override
    {
        Serial.print("got disconnect event, connected count: ");
        Serial.println(server->getConnectedCount());
        // connected count appears to be updated after this callback is
        // triggered, so the count will be at least 1 higher than reality
        if (server->getConnectedCount() <= 1) {
            MF_DISPLAY.clearBuffer();
            drawStatic("Waiting for connection...");
            MF_DISPLAY.display();
        }
        NimBLEDevice::startAdvertising();
    }
} SERVER_CALLBACKS;

class StatusLineCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *characteristic)
    {
        std::string value = characteristic->getValue();
        Serial.print("got new status line: ");
        Serial.println(value.c_str());
        MF_DISPLAY.clearBuffer();
        drawStatic(value.c_str());
        MF_DISPLAY.display();
    }
} STATUS_CALLBACKS;

void setup()
{
    Serial.begin(115200);
    // delay(2000); // startup delay for serial recon

    Serial.println("setting up ble device and service");
    NimBLEDevice::init("MANGOFRUNK-001");
    NimBLEDevice::setPower(0);
    BLE_SERVER = NimBLEDevice::createServer();
    BLE_SERVER->setCallbacks(&SERVER_CALLBACKS);
    BLEService *service = BLE_SERVER->createService(SERVICE_UUID);
    BLECharacteristic *characteristic = service->createCharacteristic(
        CHARACTERISTIC_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
    characteristic->setValue("SOME BYTES");
    characteristic->setCallbacks(&STATUS_CALLBACKS);
    service->start();

    Serial.println("starting ble advert");
    BLEAdvertising *advert = NimBLEDevice::getAdvertising();
    BLEAdvertisementData ad_data{};
    ad_data.setName("MANGOFRUNK");
    ad_data.setManufacturerData("\x5d\x05MFv001");
    advert->setAdvertisementData(ad_data);
    advert->addServiceUUID(SERVICE_UUID);
    advert->enableScanResponse(false); // what does this do?
    // advert->setMinPreferred(0x06); // huh??
    // advert->setMinPreferred(0x12); // whuuu???
    NimBLEDevice::startAdvertising();

    Serial.println("initializing display");
    MF_DISPLAY.begin(THINKINK_MONO);
    MF_DISPLAY.clearBuffer();
    drawStatic("Starting up...");
    MF_DISPLAY.display();
}

void loop()
{
    delay(2000);
    // Serial.print("current power: ");
    // Serial.println(NimBLEDevice::getPower());

    // Serial.println("Checking on peers...");
    auto peers = BLE_SERVER->getPeerDevices();
    for (auto peer : peers) {
        auto rssi = BLE_SERVER->getClient(peer)->getRssi();
        Serial.print("rssi: ");
        Serial.println(rssi);
    }
}

void drawText(const char *text, const int16_t &x = -1, const int16_t &y = -1,
              const uint8_t &size = 1, const uint16_t &color = EPD_BLACK, const bool &wrap = false)
{
    if (x >= 0 && y >= 0) {
        MF_DISPLAY.setCursor(x, y);
    }
    MF_DISPLAY.setTextSize(size);
    MF_DISPLAY.setTextColor(color);
    MF_DISPLAY.setTextWrap(wrap);
    MF_DISPLAY.print(text);
}

void drawLogo(const int16_t &x = 0, const int16_t &y = 0)
{
    MF_DISPLAY.fillRect(x, y, 101, 101, EPD_BLACK);
    MF_DISPLAY.fillCircle(x + 50, y + 50, 33, EPD_WHITE);
}

void drawSparkbox(const int16_t &x, const int16_t &y, const char *title, const char *value,
                  const Points &points)
{
    const int16_t w = 209;
    const int16_t h = 120;
    const int16_t hpad = 8;
    const int16_t vpad = 6;
    const int16_t title_h = 26;
    const int16_t graph_h = (h - title_h) - 10;
    const int16_t graph_w = w - 10;
    const int16_t graph_x = x + 5;
    const int16_t graph_y = (y + h) - 5;

    MF_DISPLAY.drawRoundRect(x, y, w, h, 4, EPD_BLACK);
    MF_DISPLAY.fillRect(x, y + title_h, w, 1, EPD_BLACK);
    drawText(title, x + hpad, y + vpad, 2);
    drawText(value, (x + (w - hpad)) - (12 * strlen(value)), y + vpad, 2);

    int16_t s_x = 0.0, s_y = 0.0, e_x = 0.0, e_y = 0.0;
    for (auto p = points.cbegin(); p != points.cend() - 1; ++p) {
        s_x = graph_x + (p->x * graph_w);
        e_x = graph_x + ((p + 1)->x * graph_w);
        s_y = graph_y + (p->y * graph_h * -1.0);
        e_y = graph_y + ((p + 1)->y * graph_h * -1.0);
        MF_DISPLAY.drawLine(s_x, s_y, e_x, e_y, EPD_BLACK);
    }
}

void drawDiscreteBox(const int16_t &x, const int16_t &y, const char *title, const char *value)
{
    const int16_t w = 209;
    const int16_t h = 26;
    const int16_t hpad = 8;
    const int16_t vpad = 6;

    MF_DISPLAY.drawRoundRect(x, y, w, h, 4, EPD_BLACK);
    drawText(title, x + hpad, y + vpad, 2);
    drawText(value, (x + (w - hpad)) - (12 * strlen(value)), y + vpad, 2);
}

void drawStatic(const char *status)
{
    // fremont logo in top left corner
    drawLogo(5, 5);

    // show connected fremont hostname/serial or connecting status
    drawText(status, 120, 15, 3);

    int16_t x = 5;
    int16_t y = 115;
    drawSparkbox(x, y, "Temp C", "69.23",
                 {
                     {.x = 0.0, .y = 0.0},
                     {.x = 1.0, .y = 1.0},
                 });
    x = x + 209 + 5;
    drawSparkbox(x, y, "FPS", "84.6",
                 {
                     {.x = 0.0, .y = 1.0},
                     {.x = 1.0, .y = 1.0},
                 });
    x = x + 209 + 5;
    drawDiscreteBox(x, y, "BIOS", "F7F0123");
    y = y + 120 + 5;
    x = 5;
    drawDiscreteBox(x, y, "OS", "20250928.1000");
    x = x + 209 + 5;
    drawSparkbox(x, y, "FAN", "1500 RPM",
                 {
                     {.x = 0.0, .y = 0.0},
                     {.x = 1.0, .y = 0.0},
                 });
    x = x + 209 + 5;
    drawSparkbox(x, y, "Watts", "999 W",
                 {
                     {.x = 0.0, .y = 0.0},
                     {.x = 1.0, .y = 1.0},
                 });

    // version tag
    drawText("mango-frunk gDEADBEEF", 4, MF_DISPLAY.height() - 12);
}
