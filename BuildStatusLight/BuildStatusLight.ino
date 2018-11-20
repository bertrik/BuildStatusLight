#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

#ifdef AUDIO
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"
#endif

#include "cmdproc.h"
#include "editline.h"

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

#ifdef AUDIO
static AudioGeneratorMP3 *mp3;
static AudioOutputI2SNoDAC *dac;
static AudioFileSourceSPIFFS *file;
#endif

static char editline[120];

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
static void leds_init(void)
{
    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_YELLOW, OUTPUT);
    pinMode(PIN_GREEN, OUTPUT);
    mode = FLASH;
    leds_run(mode, 0);
}

static void leds_set(const char *str)
{
    int value = atoi(str);
    mode = (vri_mode_t)value;
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
        
        leds_set(str);

#ifdef AUDIO
        // start audio if the file exist
        char filename[16];
        sprintf(filename, "/%s.mp3", str);
        if (SPIFFS.exists(filename)) {
            if (!mp3->isRunning()) {
                Serial.println("Begin audio playback.");
                file->open(filename);
                mp3->begin(file, dac);
            }
        }
#endif
    }
}

static int do_set(int argc, char *argv[]) 
{
    if (argc < 2) {
        return -1;
    }
    leds_set(argv[1]);
    return 0;
}

static const cmd_t cmds[] = {
    { "set", do_set, "<value> Sets the build status to 'value'"},
    { "", NULL, ""} 
};

void setup(void)
{
    // initialize serial port
    Serial.begin(115200);
    Serial.print("\nBuildStatusLight\n");

    // init LEDs
    leds_init();
    
    // init command handler
    EditInit(editline, sizeof(editline));

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

#ifdef AUDIO
    // init audio
    SPIFFS.begin();
    mp3 = new AudioGeneratorMP3();
    dac = new AudioOutputI2SNoDAC();
    file = new AudioFileSourceSPIFFS();
#endif
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

    // handle commands
    bool haveLine = false;
    if (Serial.available()) {
        char cout;
        haveLine = EditLine(Serial.read(), &cout);
        Serial.print(cout);
    }
    if (haveLine) {
        int result = cmd_process(cmds, editline);
        switch (result) {
        case CMD_OK:
            Serial.println("OK");
            break;
        case CMD_NO_CMD:
            break;
        default:
            Serial.print("ERR ");
            Serial.println(result);
            break;
        }
        Serial.print(">");
    }

#ifdef AUDIO
    // run audio if applicable
    if (mp3->isRunning()) {
        if (!mp3->loop()) {
            Serial.println("Audio playback done.");
            mp3->stop();
            file->close();
        }
    }
#endif
}

