#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <stdlib.h>

extern unsigned int gba_log_verbosity;

#define COLOR_RED       "\033[0;31m"
#define COLOR_YELLOW    "\033[0;33m"
#define COLOR_CYAN      "\033[0;36m"
#define COLOR_END       "\033[0;m"

#define LOG_VERBOSITY_WARN 1
#define LOG_VERBOSITY_INFO 2
#define LOG_VERBOSITY_DEBUG 3


#define log_set_verbosity(new_verbosity) do {gba_log_verbosity = new_verbosity;} while(0);

#define logfatal(message,...) if (1) { \
    fprintf(stderr, COLOR_RED "[FATAL] at %s:%d ", __FILE__, __LINE__);\
    fprintf(stderr, message "\n" COLOR_END, ##__VA_ARGS__);\
    exit(EXIT_FAILURE);}

#define logwarn(message,...) if (gba_log_verbosity >= LOG_VERBOSITY_WARN) {printf(COLOR_YELLOW "[WARN]  " message "\n" COLOR_END, ##__VA_ARGS__);}
#define loginfo(message,...) if (gba_log_verbosity >= LOG_VERBOSITY_INFO) {printf(COLOR_CYAN "[INFO]  " message "\n" COLOR_END, ##__VA_ARGS__);}
#define logdebug(message,...) if (gba_log_verbosity >= LOG_VERBOSITY_DEBUG) {printf("[DEBUG] " message "\n", ##__VA_ARGS__);}

#define unimplemented(condition, message) if (condition) { logfatal("UNIMPLEMENTED CASE DETECTED: %s", message) }
#endif
