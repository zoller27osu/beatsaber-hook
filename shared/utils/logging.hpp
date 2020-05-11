#ifndef LOGGING_H
#define LOGGING_H

#include <android/log.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <iostream>
#include <unistd.h>
#include <cmath> // Included to support cmath's definition of log

#ifndef LOG_VERBOSE_TYPE
#define LOG_VERBOSE_TYPE android_LogPriority
#endif

#ifndef CRITICAL
#define CRITICAL ANDROID_LOG_FATAL
#endif
#ifndef ERROR
#define ERROR ANDROID_LOG_ERROR
#endif
#ifndef WARNING
#define WARNING ANDROID_LOG_WARN
#endif
#ifndef INFO
#define INFO ANDROID_LOG_INFO
#endif
#ifndef DEBUG
#define DEBUG ANDROID_LOG_DEBUG
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

#define TAG "QuestHook[" MOD_ID "|" VERSION "]"

#ifdef log
#undef log
#endif

#define logv(lvl, ...) __android_log_vprint(lvl, TAG, __VA_ARGS__)
#ifndef FILE_LOG
#define log(lvl, ...) __android_log_print(lvl, TAG, __VA_ARGS__)
#else

#ifndef CHARS_PER_ARG
#define CHARS_PER_ARG 50
#endif

#ifndef LOG_PATH
#define LOG_PATH "/sdcard/Android/data/com.beatgames.beatsaber/files/logs/"
#endif

bool direxists(const char* dirname);

std::string get_level(int level);

// Logs with the corresponding level and format string to the log file
void log_file(int level, const char* format, ...);

// Flushes the log file
void log_flush();

// Closes the log file
void log_close();

#define log(lvl, ...) do { __android_log_print(lvl, TAG, __VA_ARGS__); \
log_file(lvl, __VA_ARGS__);} while(0);
#endif

#ifndef STD_BUFFER_SIZE
#define STD_BUFFER_SIZE 256
#endif

// From: https://codelab.wordpress.com/2014/11/03/how-to-use-standard-output-streams-for-logging-in-android-apps/
static int pfd[2];
static pthread_t thr;
static void* thread_func(void*) {
    ssize_t rdsz;
    char buf[STD_BUFFER_SIZE];
    while((rdsz = read(pfd[0], buf, sizeof buf - 1)) > 0) {
        if(buf[rdsz - 1] == '\n') --rdsz;
        buf[rdsz] = 0;  /* add null-terminator */
        __android_log_write(ANDROID_LOG_INFO, TAG, buf);
    }
    return 0;
}

extern "C" {
// Redirects stdout and stderr to the android log stream. Call this once before using stdout/stderr
// Returns 0 on success, -1 otherwise
    static int start_logger()
    {
        /* make stdout line-buffered and stderr unbuffered */
        setvbuf(stdout, NULL, _IOLBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);

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
}

#endif /* LOGGING_H */