#ifndef LOGGING_H
#define LOGGING_H

#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <iostream>
#include <unistd.h>

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
// This is too annoying, let's change it to default to some stupid stuff
// #error "'VERSION' must be defined in the mod!"
#define VERSION "0.0.0"
#endif

namespace logging {
    #ifdef log
    #undef log
    #endif
    #define log_base(...) __android_log_print(ANDROID_LOG_INFO, "QuestHook [" MOD_ID " v" VERSION "] ", __VA_ARGS__)

    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-security"
    template<typename... TArgs>
    int log(LOG_VERBOSE_TYPE level, TArgs ...args) {
        if ((LOG_LEVEL & level) != 0) {
            if (level == CRITICAL) return log_base("[CRITICAL] ", args...);
            if (level == ERROR) return log_base("[ERROR] ", args...);
            if (level == WARNING) return log_base("[WARNING] ", args...);
            if (level == INFO) return log_base("[INFO] ", args...);
            if (level == DEBUG) return log_base("[DEBUG] ", args...);
        }
        return 0;
    }
    #pragma clang diagnostic pop
}
using logging::log;


#ifndef STD_BUFFER_SIZE
#define STD_BUFFER_SIZE 256
#endif

// From: https://codelab.wordpress.com/2014/11/03/how-to-use-standard-output-streams-for-logging-in-android-apps/
static int pfd[2];
static pthread_t thr;
static void *thread_func(void*)
{
    ssize_t rdsz;
    char buf[STD_BUFFER_SIZE];
    while((rdsz = read(pfd[0], buf, sizeof buf - 1)) > 0) {
        if(buf[rdsz - 1] == '\n') --rdsz;
        buf[rdsz] = 0;  /* add null-terminator */
        __android_log_write(ANDROID_LOG_INFO, MOD_ID, buf);
    }
    return 0;
}
// Redirects stdout and stderr to the android log stream. Call this once before using stdout/stderr
// Returns 0 on success, -1 otherwise
static int start_logger()
{
    /* make stdout line-buffered and stderr unbuffered */
    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);

    /* create the pipe and redirect stdout and stderr */
    pipe(pfd);
    dup2(pfd[1], 1);
    dup2(pfd[1], 2);

    /* spawn the logging thread */
    if(pthread_create(&thr, 0, thread_func, 0) == -1)
        return -1;
    pthread_detach(thr);
    return 0;
}

#endif /* LOGGING_H */