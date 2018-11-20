/**
 * Initializes the Print sub-system.
 */
void PrintInit(long baud);

/**
 * Prints a formatted text the serial console.
 * 
 * @param[in] fmt printf-style format string
 */
void print(const char *fmt, ...);

