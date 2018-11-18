#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

#define MQTT_HOST   "test.mosquitto.org"
#define MQTT_PORT   1883

#define MQTT_TOPIC  "bertrik/buildstatus"

#define PIN_RED     D2
#define PIN_YELLOW  D3
#define PIN_GREEN   D4

typedef enum {
    OFF = 0,
    RED,
    YELLOW,
    GREEN,
    FLASH
} vri_mode_t;

static char esp_id[16];
static WiFiManager wifiManager;
static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static vri_mode_t mode = FLASH;

static AudioGeneratorMP3 *mp3;
static AudioOutputI2SNoDAC *dac;
static AudioFileSourceSPIFFS *file;

// updates all three LEDs
static void leds_write(int red, int yellow, int green)
{
    digitalWrite(PIN_RED, red);
    digitalWrite(PIN_YELLOW, yellow);
    digitalWrite(PIN_GREEN, green);
}

// runs the LEDs including flashing
static void leds_run(vri_mode_t mode, unsigned long ms)
{
    int blink = (ms / 500) % 2;

    switch (mode) {
    case RED:       leds_write(1, 0, 0);        break;
    case YELLOW:    leds_write(0, 1, 0);        break;
    case GREEN:     leds_write(0, 0, 1);        break;
    case FLASH:     leds_write(0, blink, 0);    break;

    default:
    case OFF:       leds_write(0, 0, 0);        break;
        break;
    }
}

// initializes the LEDs
static void leds_init(vri_mode_t newmode)
{
    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_YELLOW, OUTPUT);
    pinMode(PIN_GREEN, OUTPUT);
    mode = newmode;
    leds_run(mode, 0);
}

static void mqtt_callback(const char *topic, byte* payload, unsigned int length)
{
    char str[100];

    if (length < sizeof(str)) {
        memcpy(str, payload, length);
        str[length] = '\0';

        Serial.print("Received '");
        Serial.print(str);
        Serial.println("'");
        
        int val = atoi(str);
        leds_init((vri_mode_t)val);

        // start audio if the file exist
        char filename[16];
        sprintf(filename, "/%d.mp3", val);
        if (SPIFFS.exists(filename)) {
            if (!mp3->isRunning()) {
                Serial.println("Begin audio playback.");
                file->open(filename);
                mp3->begin(file, dac);
            }
        }
    }
}

void setup(void)
{
    // initialize serial port
    Serial.begin(115200);
    Serial.print("\nBuildStatusLight\n");
    
    // init file system
    SPIFFS.begin();

    // init LEDs
    leds_init(FLASH);

    // get ESP id
    sprintf(esp_id, "%08X", ESP.getChipId());
    Serial.print("ESP ID: ");
    Serial.println(esp_id);

    // connect to wifi
    Serial.println("Starting WIFI manager ...");
    wifiManager.autoConnect("ESP-BuildStatusLight");
    
    // connect to topic
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCallback(mqtt_callback);

    // init audio
    mp3 = new AudioGeneratorMP3();
    dac = new AudioOutputI2SNoDAC();
    file = new AudioFileSourceSPIFFS();
}

void loop()
{
    // run the LEDs
    unsigned long t = millis();
    leds_run(mode, t);
    
    // stay subscribed
    if (!mqttClient.connected()) {
        mqttClient.connect(esp_id);
        mqttClient.subscribe(MQTT_TOPIC);
    }
    mqttClient.loop();

    // run audio if applicable
    if (mp3->isRunning()) {
        if (!mp3->loop()) {
            Serial.println("Audio playback done.");
            mp3->stop();
            file->close();
        }
    }
}

