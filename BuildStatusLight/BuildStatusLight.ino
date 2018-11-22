#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "cmdproc.h"
#include "editline.h"
#include "print.h"

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

static vri_mode_t mode = FLASH;

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

static void show_help(const cmd_t *cmds)
{
    for (const cmd_t *cmd = cmds; cmd->cmd != NULL; cmd++) {
        print("%10s: %s\n", cmd->name, cmd->help);
    }
}

static int do_help(int argc, char *argv[]);

static int do_set(int argc, char *argv[]) 
{
    if (argc < 2) {
        return -1;
    }
    leds_set(argv[1]);
    return 0;
}

static const cmd_t commands[] = {
    { "set",    do_set,     "<value> Sets the build status to 'value'"},
    { "help",   do_help,    "Show available commands"},
    { "", NULL, ""} 
};

static int do_help(int argc, char *argv[])
{
    show_help(commands);
    return 0;
}

void setup(void)
{
    // initialize serial port
    PrintInit(115200);
    print("\nBuildStatusLight\n");

    // init LEDs
    leds_init();
    
    // init command handler
    EditInit(editline, sizeof(editline));
}

void loop()
{
    // run the LEDs
    unsigned long t = millis();
    leds_run(mode, t);
    
    // handle commands
    bool haveLine = false;
    if (Serial.available()) {
        char cout;
        haveLine = EditLine(Serial.read(), &cout);
        print("%c", cout);
    }
    if (haveLine) {
        int result = cmd_process(commands, editline);
        switch (result) {
        case CMD_OK:
            print("OK\n");
            break;
        case CMD_NO_CMD:
            break;
        default:
            print("ERR %d\n", result);
            break;
        }
        printf(">");
    }
}

