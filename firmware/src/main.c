// vim: foldmethod=marker:foldmarker={{{,}}}
#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <driver/spi_master.h>
#include <esp_chip_info.h>
#include <esp_event.h>
#include <esp_flash.h>
#include <esp_log.h>
#include <esp_pm.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <inttypes.h>
#include <nvs_flash.h>

#include <sys/stat.h>
#include <sys/unistd.h>

#define NOW_MS ((uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS))

static const char *TAG = "main";

static i2c_master_bus_handle_t BUS0_HANDLE = NULL;

void setup_chip(void)
{ // {{{
    /* load chip info, flash size, etc. */
    esp_chip_info_t chip_info;
    uint32_t flash_size = 0;
    esp_chip_info(&chip_info);
    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get flash size.");
        return;
    }

    /* print chip info */
    ESP_LOGI(
        TAG, "%s, %d core, WiFi%s%s%s, v%d.%d, %" PRIu32 "MB %s flash, %" PRIu32 "KB min free heap",
        CONFIG_IDF_TARGET, chip_info.cores, (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
        (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
        (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "",
        major_rev, minor_rev, flash_size / (uint32_t)(1024 * 1024),
        (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external",
        esp_get_minimum_free_heap_size() / (uint32_t)1024);

    /* initialize non-volatile storage, erasing if old */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* initialize the default event loop
     *   not necessary for a lot of basic applications, like arduino style stuff
     *   but things like the WiFi subsystem, etc. need this to exist
     */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

#ifdef QWIIC_ENABLE_PIN
    ESP_LOGI(TAG, "enabling qwiic port using pin %d", QWIIC_ENABLE_PIN);
    gpio_config_t qwiic_conf = {
        .pin_bit_mask = (1ULL << QWIIC_ENABLE_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&qwiic_conf));
    ESP_ERROR_CHECK(gpio_set_level(QWIIC_ENABLE_PIN, 1));
    ESP_ERROR_CHECK(gpio_hold_en(QWIIC_ENABLE_PIN));
#endif

#ifdef ENABLE_SLEEP
    esp_pm_config_t pm_config;
    ESP_ERROR_CHECK(esp_pm_get_configuration(&pm_config));
    pm_config.light_sleep_enable = true;
    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
#endif
} // }}}

void app_main(void)
{ // {{{
    /* add '-D START_DELAY_MS=(5000)' to a build_flags setting in platformio.ini
     * to delay startup for 5s (or however long you want), useful if you're
     * trying to see serial output when resetting.
     */
#ifdef START_DELAY_MS
    ESP_LOGI(TAG, "delaying startup for %d ms", START_DELAY_MS);
    vTaskDelay(START_DELAY_MS / portTICK_PERIOD_MS);
#endif

    setup_chip();

    /* setup the i2c busses here, subsystems will grab handles using
     *  esp_err_t i2c_master_get_bus_handle(i2c_port, i2c_master_bus_handle_t&)
     *
     *  bus 0 is the main telemetry bus, bus 1 is the charger and accessory bus
     */
    i2c_master_bus_config_t i2c_conf = {0};
    i2c_conf.clk_source = I2C_CLK_SRC_DEFAULT;    // may need changed if we enable sleep
    i2c_conf.glitch_ignore_cnt = 7;               // noise filtering, docs suggest 7
    i2c_conf.intr_priority = 0;                   // interrupt priority, 0 for auto-select
    i2c_conf.trans_queue_depth = 0;               // depth of queue for async requests
    i2c_conf.flags.enable_internal_pullup = true; // WARN: not recommended, should use external
    i2c_conf.flags.allow_pd = false;              // don't allow powerdown for sleep
    i2c_conf.i2c_port = 0;
    i2c_conf.sda_io_num = I2C_SDA0_PIN;
    i2c_conf.scl_io_num = I2C_SCL0_PIN;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_conf, &BUS0_HANDLE));
    ESP_LOGI(TAG, "i2c master bus 0 configured with SDA on %d and SCL on pin %d", I2C_SDA0_PIN,
             I2C_SCL0_PIN);
} // }}}
