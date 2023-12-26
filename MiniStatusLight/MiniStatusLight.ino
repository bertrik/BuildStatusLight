#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "ArduinoJson.h"
#include "MiniShell.h"

#include "tlc.h"

#define CONFIG_MAGIC   0xCAFEF00D

typedef struct {
    char gitlab_url[120];
    char gitlab_token[60];
    uint32_t magic;
} config_t;

static MiniShell shell(&Serial);
static config_t config;
static char user_agent[128];
static WiFiClientSecure wifiClient;
static DynamicJsonDocument doc(2048);
static int last_period = -1;
static int interval = 600000;

// see https://docs.gitlab.com/ee/api/pipelines.html#list-project-pipelines
typedef struct {
    const char *status;
    tlc_mode_t mode;
} mapping_t;

// gitlab docs don't explain exactly what these mean, we map intermediate states to flashing
static const mapping_t mappings[] = {
    { "created", TLC_MODE_FLASH},
    { "waiting_for_resource", TLC_MODE_FLASH},
    { "preparing", TLC_MODE_FLASH},
    { "pending", TLC_MODE_FLASH},
    { "running", TLC_MODE_FLASH},
    { "success", TLC_MODE_GREEN},
    { "failed", TLC_MODE_RED},
    { "canceled", TLC_MODE_YELLOW},
    { "skipped", TLC_MODE_YELLOW},
    { "manual", TLC_MODE_FLASH},
    { "scheduled", TLC_MODE_FLASH},
    { nullptr, TLC_MODE_OFF}
};

static void config_init(void)
{
    EEPROM.begin(sizeof(config));
    EEPROM.get(0, config);
    if (config.magic != CONFIG_MAGIC) {
        // use sensible defaults, but don't validate yet
        memset(&config, 0, sizeof(config));
        strcpy(config.gitlab_url, "https://gitlab.technolution.nl/api/v4/projects/197/pipelines/latest?ref=master");
        strcpy(config.gitlab_token, "glpat-secret");
    }
}

static void config_save(void)
{
    config.magic = CONFIG_MAGIC;
    EEPROM.put(0, config);
    EEPROM.commit();
}

static bool fetch_url(const char *url, String & response)
{
    HTTPClient httpClient;
    httpClient.begin(wifiClient, url);
    httpClient.setTimeout(20000);
    httpClient.setUserAgent(user_agent);
    httpClient.addHeader("PRIVATE-TOKEN", config.gitlab_token);

    Serial.printf("> GET %s\n", url);
    int res = httpClient.GET();

    // evaluate result
    bool result = (res == HTTP_CODE_OK);
    response = result ? httpClient.getString() : httpClient.errorToString(res);
    httpClient.end();
    Serial.printf("< %d: %s\n", res, response.c_str());
    return result;
}

static int do_get(int argc, char *argv[])
{
    String response;
    if (fetch_url(config.gitlab_url, response)) {
        // decode JSON
        if (deserializeJson(doc, response) == DeserializationError::Ok) {
            // find TLC mode associated with the build status
            const char *status = doc["status"];
            Serial.printf("status = '%s'\n", status);
            tlc_mode_t mode = TLC_MODE_OFF;
            for (const mapping_t *mapping = mappings; mapping->status != nullptr; mapping++) {
                if (strcmp(mapping->status, status) == 0) {
                    mode = mapping->mode;
                    break;
                }
            }
            Serial.printf("Status '%s' -> mode %d\n", status, mode);
            tlc_set_mode(mode);
            return 0;
        }
        return -2;
    }
    return -1;
}

static int do_reboot(int argc, char *argv[])
{
    ESP.restart();
    return 0;
}

static int do_tlc(int argc, char *argv[])
{
    if (argc < 2) {
        return -1;
    }
    tlc_mode_t mode = (tlc_mode_t) atoi(argv[1]);
    tlc_set_mode(mode);
    return 0;
}

static int do_wifi(int argc, char *argv[])
{
    char *ssid;
    char *pass;

    switch (argc) {
    case 1:
        Serial.println(WiFi.localIP());
        break;
    case 2:
        ssid = argv[1];
        WiFi.persistent(true);
        WiFi.begin(ssid);
        break;
    case 3:
        ssid = argv[1];
        pass = argv[2];
        WiFi.persistent(true);
        WiFi.begin(ssid, pass);
        break;
    default:
        break;
    }
    return WiFi.status();
}

static int do_gitlab(int argc, char *argv[])
{
    if (argc > 2) {
        char *url = argv[1];
        char *token = argv[2];
        strcpy(config.gitlab_url, url);
        strcpy(config.gitlab_token, token);
        config_save();
    }
    Serial.printf("gitlab URL:   %s\n", config.gitlab_url);
    Serial.printf("gitlab token: %s\n", config.gitlab_token);
    return 0;
}

static const cmd_t commands[] = {
    { "reboot", do_reboot, "Reboot" },
    { "tlc", do_tlc, "<value> Sets the traffic light controller status" },
    { "wifi", do_wifi, "[ssid] [pass] Configures WiFi" },
    { "gitlab", do_gitlab, "[<url> <token>] Configures gitlab url and token" },
    { "get", do_get, "Fetches the build status and updates the light" },
    { "", nullptr, "" }
};

void setup(void)
{
    Serial.begin(115200);
    Serial.printf("\nBuildStatusLight\n");

    sprintf(user_agent, "esp-buildlight-%06x", ESP.getChipId());

    // we can't really verify the signature
    wifiClient.setInsecure();

    config_init();
    tlc_init();
    WiFi.begin();
}

void loop()
{
    // handle commands
    shell.process(">", commands);

    // automatic fetching
    int period = millis() / interval;
    if ((WiFi.status() == WL_CONNECTED) && (period != last_period)) {
        last_period = period;
        do_get(0, nullptr);
    }

    // run the LEDs
    tlc_run(millis());
}
