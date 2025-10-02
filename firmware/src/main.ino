#include <iomanip>
#include <limits>
#include <sstream>

#include <Adafruit_ThinkInk.h>
#include <NimBLEDevice.h>

#define SERVICE_UUID NimBLEUUID{"95c7b479-8e84-4ce7-a121-faf74bf48c84"}
#define TOPLINE_UUID NimBLEUUID{"d6f4c07e-4a21-4c69-bd15-43a38a871900"}
#define MIDLINE_UUID NimBLEUUID{"d6f4c07e-4a21-4c69-bd15-43a38a871901"}
#define BOTLINE_UUID NimBLEUUID{"d6f4c07e-4a21-4c69-bd15-43a38a871902"}
#define KEYVAL_UUID NimBLEUUID{"d6f4c07e-4a21-4c69-bd15-43a38a871903"}
#define VECTOR_UUID NimBLEUUID{"d6f4c07e-4a21-4c69-bd15-43a38a871904"}
#define FLUSH_UUID NimBLEUUID{"d6f4c07e-4a21-4c69-bd15-43a38a8719FF"}

#define RSSI_LIMIT -40

NimBLEServer *BLE_SERVER = nullptr;

ThinkInk_583_Mono_AAAMFGN MF_DISPLAY(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

static unsigned long DISP_DEBOUNCE = 0;

struct RssiWindow {
    float avg = 0;
    float collection[5] = {0};
    int pos = 0;

    void push(const float &rssi)
    {
        // ignore outliers
        if (rssi < -90 || rssi > -5) {
            return;
        }
        auto size = sizeof(collection) / sizeof(collection[0]);
        collection[pos] = rssi;
        pos = (pos + 1) % size;
        avg = 0;
        for (int i = 0; i < size; ++i) {
            avg += collection[i];
        }
        avg /= size;
    }
} RSSI_WINDOW;

struct Point {
    float x;
    float y;

    Point()
        : x(0)
        , y(0)
    {
    }
    Point(float _x, float _y)
        : x(_x)
        , y(_y)
    {
    }
};
typedef std::vector<Point> Points;

struct KeyVal {
    std::string key;
    std::string val;

    KeyVal()
        : key{""}
        , val{""}
    {
    }
};
typedef std::vector<KeyVal> KeyVals;

struct State {
    bool connected = false;
    std::string topLine{"Starting up..."};
    std::string midLine{"No User"};
    std::string botLine{"No Activity"};

    KeyVals keyvals{9};
    std::vector<Points> sparks{6};

    void reset()
    {
        keyvals.clear();
        keyvals.resize(9);
        sparks.clear();
        sparks.resize(6);

        connected = false;
        topLine = "Waiting on connection...";
        midLine = "";
        botLine = "";
        keyvals[0].key = "OS";
        keyvals[0].val = "--";
        keyvals[1].key = "BIOS";
        keyvals[1].val = "--";
        keyvals[2].key = "STEAM";
        keyvals[2].val = "--";
        keyvals[3].key = "CPU";
        keyvals[3].val = "-- dC";
        keyvals[4].key = "GPU";
        keyvals[4].val = "-- dC";
        keyvals[5].key = "FAN";
        keyvals[5].val = "-- RPM";
        keyvals[6].key = "CPU";
        keyvals[6].val = "--%";
        keyvals[7].key = "GPU";
        keyvals[7].val = "--%";
        keyvals[8].key = "FPS";
        keyvals[8].val = "--";
    }
} STATE;

void drawStatic();

class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *server, NimBLEConnInfo &conn) override
    {
        // we don't want any other devices to see us once we are connected
        // to a host
        NimBLEDevice::stopAdvertising();
        Serial.println("got connection");
        auto rssi = server->getClient(conn)->getRssi();
        if (rssi < RSSI_LIMIT) {
            Serial.print("rssi out of bounds, rejecting: ");
            Serial.println(rssi);
            server->disconnect(conn);
        } else {
            STATE.connected = true;
        }
    }

    void onDisconnect(NimBLEServer *server, NimBLEConnInfo &conn, int reason) override
    {
        Serial.print("got disconnect event, connected count: ");
        Serial.println(server->getConnectedCount());
        // connected count appears to be updated after this callback is
        // triggered, so the count will be at least 1 higher than reality
        if (server->getConnectedCount() <= 1) {
            if (STATE.connected) {
                DISP_DEBOUNCE = 100;
            }
            STATE.reset();
        }
        NimBLEDevice::startAdvertising();
    }
} SERVER_CALLBACKS;

class StatusLineCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *characteristic, NimBLEConnInfo &conn) override
    {
        std::string value = characteristic->getValue();
        auto uuid = characteristic->getUUID();
        if (uuid == TOPLINE_UUID && STATE.topLine != value) {
            STATE.topLine = value;
        } else if (uuid == MIDLINE_UUID && STATE.midLine != value) {
            STATE.midLine = value;
        } else if (uuid == BOTLINE_UUID && STATE.botLine != value) {
            STATE.botLine = value;
        } else if (uuid != TOPLINE_UUID && uuid != MIDLINE_UUID && uuid != BOTLINE_UUID) {
            Serial.print("Got value (");
            Serial.print(value.c_str());
            Serial.print(") for unknown UUID (");
            Serial.print(uuid.toString().c_str());
            Serial.println("), ignoring.");
            return;
        }
    }
} STATUS_CALLBACKS;

class KeyValCallbacks : public NimBLECharacteristicCallbacks
{
    typedef struct __attribute__((packed)) {
        uint8_t index;
        char key[32];
        char val[32];
    } Msg;

    void onWrite(NimBLECharacteristic *characteristic, NimBLEConnInfo &conn) override
    {
        std::string value = characteristic->getValue();
        Msg msg;
        if (value.length() == sizeof(Msg)) {
            memcpy(&msg, value.data(), sizeof(Msg));
            STATE.keyvals[msg.index].key = msg.key;
            STATE.keyvals[msg.index].val = msg.val;
        } else {
            Serial.print("got bad keyval write, size: ");
            Serial.println(value.length());
        }
    }
} KEYVAL_CALLBACKS;

class VectorCallbacks : public NimBLECharacteristicCallbacks
{
    typedef struct __attribute__((packed)) {
        uint8_t index;
        uint8_t count;
        uint16_t values[20];
    } Msg;

    void onWrite(NimBLECharacteristic *characteristic, NimBLEConnInfo &conn) override
    {
        std::string value = characteristic->getValue();
        Msg msg;
        if (value.length() >= 2) {
            memcpy(&msg, value.data(), sizeof(Msg));
            Serial.print("got vector for index (");
            Serial.print(msg.index);
            Serial.print(") with ");
            Serial.print(msg.count);
            Serial.println(" values");
            STATE.sparks[msg.index].clear();
            for (int i = 0; i < msg.count; i += 2) {
                STATE.sparks[msg.index].emplace_back(msg.values[i] / 65535.0,
                                                     msg.values[i + 1] / 65535.0);
            }
        } else {
            Serial.print("got bad vectors write, size: ");
            Serial.println(value.length());
        }
    }
} VECTOR_CALLBACKS;

class FlushCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *characteristic, NimBLEConnInfo &conn) override
    {
        DISP_DEBOUNCE = 100;
    }
} FLUSH_CALLBACKS;

void setup()
{
    STATE.reset();

    Serial.begin(115200);

    Serial.println("setting up ble device and service");
    NimBLEDevice::init("");
    NimBLEDevice::setPower(21);
    BLE_SERVER = NimBLEDevice::createServer();
    BLE_SERVER->setCallbacks(&SERVER_CALLBACKS);
    BLEService *service = BLE_SERVER->createService(SERVICE_UUID);
    BLECharacteristic *characteristic = nullptr;

    // status line characteristics, they share callbacks
    characteristic =
        service->createCharacteristic(TOPLINE_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
    characteristic->setValue(STATE.topLine.c_str());
    characteristic->setCallbacks(&STATUS_CALLBACKS);
    characteristic =
        service->createCharacteristic(MIDLINE_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
    characteristic->setValue(STATE.midLine.c_str());
    characteristic->setCallbacks(&STATUS_CALLBACKS);
    characteristic =
        service->createCharacteristic(BOTLINE_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
    characteristic->setValue(STATE.botLine.c_str());
    characteristic->setCallbacks(&STATUS_CALLBACKS);

    characteristic = service->createCharacteristic(KEYVAL_UUID, NIMBLE_PROPERTY::WRITE);
    characteristic->setCallbacks(&KEYVAL_CALLBACKS);
    characteristic = service->createCharacteristic(VECTOR_UUID, NIMBLE_PROPERTY::WRITE);
    characteristic->setCallbacks(&VECTOR_CALLBACKS);

    characteristic = service->createCharacteristic(FLUSH_UUID, NIMBLE_PROPERTY::WRITE);
    characteristic->setCallbacks(&FLUSH_CALLBACKS);

    service->start();

    Serial.println("starting ble advert");
    std::stringstream name;
    name << "MANGOFRUNK-";
    name << std::uppercase << std::hex << std::setfill('0') << std::setw(12)
         << NimBLEDevice::getAddress();
    BLEAdvertising *advert = NimBLEDevice::getAdvertising();
    BLEAdvertisementData ad_data{};
    ad_data.setName(name.str());
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
    drawStatic();
    MF_DISPLAY.display();
}

void loop()
{
    static unsigned long LAST_MS = 0;
    static unsigned long RSSI_DEBOUNCE = 200;

    auto now = millis();
    auto delta = now - LAST_MS;
    if (now < LAST_MS) {
        // handling rollover
        Serial.println("handling time rollover");
        delta = (std::numeric_limits<unsigned long>::max() - LAST_MS) + now;
    }

    if (RSSI_DEBOUNCE > 0 && RSSI_DEBOUNCE > delta) {
        RSSI_DEBOUNCE -= delta;
    } else if (RSSI_DEBOUNCE > 0) {
        RSSI_DEBOUNCE = 200;
        auto peers = BLE_SERVER->getPeerDevices();
        for (auto peer : peers) {
            auto rssi = BLE_SERVER->getClient(peer)->getRssi();
            RSSI_WINDOW.push(rssi);
            if (RSSI_WINDOW.avg < RSSI_LIMIT) {
                Serial.print("rssi out of bounds, disconnecting: ");
                Serial.println(RSSI_WINDOW.avg);
                BLE_SERVER->disconnect(peer);
            }
            break;
        }
    }

    if (DISP_DEBOUNCE > 0 && DISP_DEBOUNCE > delta) {
        DISP_DEBOUNCE -= delta;
    } else if (DISP_DEBOUNCE > 0) {
        DISP_DEBOUNCE = 0;
        MF_DISPLAY.clearBuffer();
        drawStatic();
        MF_DISPLAY.display();
    }

    LAST_MS = now;
    delay(10);
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

void drawLogo(int16_t &x, const int16_t &y = 0)
{
    MF_DISPLAY.fillRoundRect(x, y, 101, 101, 4, EPD_BLACK);
    MF_DISPLAY.fillCircle(x + 50, y + 50, 33, EPD_WHITE);
    x += 101;
}

void drawSparkbox(int16_t &x, const int16_t &y, std::string &title, const std::string &value,
                  const Points &points)
{
    const int16_t w = 209;
    const int16_t h = 120;
    const int16_t hpad = 8;
    const int16_t vpad = 6;
    const int16_t title_h = 26;
    const int16_t graph_h = (h - title_h) - 20;
    const int16_t graph_w = w - 20;
    const int16_t graph_x = x + 10;
    const int16_t graph_y = (y + h) - 10;

    if (!title.empty()) {
        MF_DISPLAY.drawRoundRect(x, y, w, h, 4, EPD_BLACK);
        MF_DISPLAY.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 4, EPD_BLACK);
        MF_DISPLAY.fillRect(x, y + title_h, w, 1, EPD_BLACK);
        drawText(title.c_str(), x + hpad, y + vpad, 2);
        drawText(value.c_str(), (x + (w - hpad)) - (12 * strlen(value.c_str())), y + vpad, 2);

        if (points.size() >= 2) {
            int16_t s_x = 0.0, s_y = 0.0, e_x = 0.0, e_y = 0.0;
            for (auto p = points.cbegin(); p != points.cend() - 1; ++p) {
                s_x = graph_x + (p->x * graph_w);
                e_x = graph_x + ((p + 1)->x * graph_w);
                s_y = graph_y + (p->y * graph_h * -1.0);
                e_y = graph_y + ((p + 1)->y * graph_h * -1.0);
                MF_DISPLAY.drawLine(s_x, s_y, e_x, e_y, EPD_BLACK);
                MF_DISPLAY.drawLine(s_x, s_y - 1, e_x, e_y - 1, EPD_BLACK);
                MF_DISPLAY.drawLine(s_x, s_y + 1, e_x, e_y + 1, EPD_BLACK);
                MF_DISPLAY.drawLine(s_x - 1, s_y, e_x - 1, e_y, EPD_BLACK);
                MF_DISPLAY.drawLine(s_x + 1, s_y, e_x + 1, e_y, EPD_BLACK);
            }
        }
    }

    x += w;
}

void drawDiscreteBox(int16_t &x, const int16_t &y, const std::string &title,
                     const std::string &value)
{
    const int16_t w = 209;
    const int16_t h = 26;
    const int16_t hpad = 8;
    const int16_t vpad = 6;

    if (!title.empty()) {
        MF_DISPLAY.drawRoundRect(x, y, w, h, 4, EPD_BLACK);
        MF_DISPLAY.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 4, EPD_BLACK);
        drawText(title.c_str(), x + hpad, y + vpad, 2);
        drawText(value.c_str(), (x + (w - hpad)) - (12 * strlen(value.c_str())), y + vpad, 2);
    }

    x += w;
}

void drawStatic()
{
    int16_t x = 0;
    int16_t y = 0;

    // fremont logo in top left corner
    x = 5;
    y = 5;
    drawLogo(x, y);

    // show connected fremont hostname/serial or connecting status
    x = 120;
    y = 15;
    drawText(STATE.topLine.c_str(), x, y, 3);
    y += 35;
    drawText(STATE.midLine.c_str(), x, y, 2);
    y += 30;
    drawText(STATE.botLine.c_str(), x, y, 2);

    // first row of boxes with no sparklines
    x = 5;
    y = 115;
    drawDiscreteBox(x, y, STATE.keyvals[0].key, STATE.keyvals[0].val);
    x += 5;
    drawDiscreteBox(x, y, STATE.keyvals[1].key, STATE.keyvals[1].val);
    x += 5;
    drawDiscreteBox(x, y, STATE.keyvals[2].key, STATE.keyvals[2].val);

    // second row
    x = 5;
    y += 26 + 5;
    drawSparkbox(x, y, STATE.keyvals[3].key, STATE.keyvals[3].val, STATE.sparks[0]);
    x += 5;
    drawSparkbox(x, y, STATE.keyvals[4].key, STATE.keyvals[4].val, STATE.sparks[1]);
    x += 5;
    drawSparkbox(x, y, STATE.keyvals[5].key, STATE.keyvals[5].val, STATE.sparks[2]);

    // third row
    x = 5;
    y += 120 + 5;
    drawSparkbox(x, y, STATE.keyvals[6].key, STATE.keyvals[6].val, STATE.sparks[3]);
    x += 5;
    drawSparkbox(x, y, STATE.keyvals[7].key, STATE.keyvals[7].val, STATE.sparks[4]);
    x += 5;
    drawSparkbox(x, y, STATE.keyvals[8].key, STATE.keyvals[8].val, STATE.sparks[5]);

    // version tag
    std::stringstream tag;
    tag << "mango-frunk " << GIT_REVISION << ", @nakylew";
    x = 4;
    y = MF_DISPLAY.height() - 12;
    drawText(tag.str().c_str(), x, y);
}
