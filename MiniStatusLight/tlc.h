#include <stdint.h>

#include "tlcmode.h"

void tlc_init(void);
void tlc_run(unsigned long ms);
void tlc_set_mode(tlc_mode_t mode);
void tlc_set_colors(bool top, bool middle, bool bottom);
