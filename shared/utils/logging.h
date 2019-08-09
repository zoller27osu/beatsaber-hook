#ifndef LOGGING_H
#define LOGGING_H

#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

enum LOG_VERBOSE_TYPE {
    CRITICAL = 1,
    ERROR = 2,
    WARNING = 4,
    INFO = 8,
    DEBUG = 16
};

#ifndef LOG_LEVEL
#define LOG_LEVEL WARNING | ERROR | CRITICAL
#endif

#ifndef MOD_ID
// This is too annoying, let's change it to default to some stupid stuff
// #error "'MOD_ID' must be defined in the mod!"
#define MOD_ID "PLACEHOLDER_MOD_ID"
#endif
#ifndef VERSION
// This is too annoying, let's change ti to default to some stupid stuff
// #error "'VERSION' must be defined in the mod!"
#define VERSION "0.0.0"
#endif

#ifdef log
#undef log
#endif
#define log_base(...) __android_log_print(ANDROID_LOG_INFO, "QuestHook [" MOD_ID " v" VERSION "] ", __VA_ARGS__)

#define log(level, ...) if ((LOG_LEVEL & level) != 0) {\
if (level == CRITICAL) log_base("[CRITICAL] " __VA_ARGS__); \
if (level == ERROR) log_base("[ERROR] " __VA_ARGS__); \
if (level == WARNING) log_base("[WARNING] " __VA_ARGS__); \
if (level == INFO) log_base("[INFO] " __VA_ARGS__); \
if (level == DEBUG) log_base("[DEBUG] " __VA_ARGS__); }

#endif /* LOGGING_H */