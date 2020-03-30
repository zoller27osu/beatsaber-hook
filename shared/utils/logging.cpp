#ifdef FILE_LOG
#include "logging.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

static FILE *fp;

void log_create() {
    if (!direxists(LOG_PATH)) {
        // DANGER
        system((std::string("mkdir ") + LOG_PATH).data());
    }
    fp = fopen((std::string(LOG_PATH) + MOD_ID + ".log").data(), "w");
}

std::string get_level(int level) {
    switch (level)
    {
    case CRITICAL:
        return std::string("CRITICAL");
    case ERROR:
        return std::string("ERROR");
    case WARNING:
        return std::string("WARNING");
    case INFO:
        return std::string("INFO");
    case DEBUG:
    default:
        return std::string("DEBUG");
    }
}

void log_file(int level, const char* format, ...) {
    va_list args;
    if (fp == nullptr) {
        log_create();
    }
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm bt = *std::localtime(&in_time_t);
    std::ostringstream oss;
    oss << std::put_time(&bt, "%Y/%M/%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    // Write prefix
    fprintf(fp, "%s [%s] %s: ", oss.str().data(), TAG, get_level(level).data());
    va_start(args, format);
    vfprintf(fp, format, args);
    va_end(args);
    fprintf(fp, "\n");
}

void log_flush() {
    if (fp) {
        fflush(fp);
    }
}

void log_close() {
    if (fp) {
        fclose(fp);
    }
}

#endif