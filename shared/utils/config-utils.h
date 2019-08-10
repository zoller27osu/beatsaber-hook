#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "rapidjson-utils.h"
#include "../../rapidjson/include/rapidjson/document.h"
#include "../../rapidjson/include/rapidjson/allocators.h"

#ifdef __cplusplus
// Provides helper functions for configuration.
// You are responsible for Loading and Writing to it as necessary.
namespace Configuration {
    // Loads JSON config
    rapidjson::Document& Load();
    // Reloads JSON config
    rapidjson::Document& Reload();
    // Writes JSON config
    void Write();
}
#endif /* __cplusplus */

#endif /* CONFIG_UTILS_H */
