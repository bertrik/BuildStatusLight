#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "MiniShell.h"

#include "tlc.h"

static MiniShell shell(&Serial);

static int do_set(int argc, char *argv[])
{
    if (argc < 2) {
        return -1;
    }
    tlc_mode_t mode = (tlc_mode_t) atoi(argv[1]);
    tlc_set_mode(mode);
    return 0;
}

static const cmd_t commands[] = {
    { "set", do_set, "<value> Sets the build status" },
    { "", NULL, "" }
};

void setup(void)
{
    Serial.begin(115200);
    Serial.printf("\nBuildStatusLight\n");

    tlc_init();
}

void loop()
{
    // handle commands
    shell.process(">", commands);

    // run the LEDs
    tlc_run(millis());
}
