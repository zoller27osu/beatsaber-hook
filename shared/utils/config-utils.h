#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "rapidjson-utils.h"

namespace Configuration {
    // Provides a class for configuration.
    // You are responsible for Loading and Writing to it as necessary.
    class Config {
        public:
            Config();
            rapidjson::Document document;
            // Loads Config
            static Config& Load();
            // Reloads Config from file
            void Reload();
            // Writes Config
            void Write();
    };
}

#endif /* CONFIG_UTILS_H */
