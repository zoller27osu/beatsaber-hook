#ifndef CONFIG_DEFINED_H
#define CONFIG_DEFINED_H

#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../../shared/config/config-utils.hpp"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "../../include/modloader.hpp"

// CONFIG

std::string Configuration::configPath;
bool Configuration::configPathSet;

using namespace rapidjson;

// Loads the config for the given mod, if it doesn't exist, will leave it as an empty object.
void Configuration::Load() {
    if (readJson) {
        return;
    }
    std::string filename = getconfigpath(info);
    Logger::get().info("Loading config from path: %s", filename.c_str());

    if (!fileexists(filename.c_str())) {
        writefile(filename.c_str(), "{}");
    }
    if (!parsejsonfile(config, filename)) {
        readJson = false;
    }
    if (!config.IsObject()) {
        config.SetObject();
    }
    readJson = true;
}

void Configuration::Reload() {
    parsejsonfile(config, getconfigpath(info));
    if (!config.IsObject()) {
        config.SetObject();
    }
    readJson = true;
}

void Configuration::Write() {
    if (!config.IsObject()) {
        config.SetObject();
    }
    StringBuffer buf;
    PrettyWriter<StringBuffer> writer(buf);
    config.Accept(writer);
    writefile(getconfigpath(info).c_str(), buf.GetString());
}

bool parsejsonfile(ConfigDocument& doc, std::string filename) {
    if (!fileexists(filename.c_str())) {
        return false;
    }
    std::ifstream is;
    is.open(filename.c_str());

    IStreamWrapper wrapper {is};
    
    return !doc.ParseStream(wrapper).HasParseError();
}

bool parsejson(ConfigDocument& doc, std::string_view js) {
    char temp[js.length() + 1];
    memcpy(temp, js.data(), js.length());
    temp[js.length()] = '\0';

    if (doc.ParseInsitu(temp).HasParseError()) {
        return false;
    }
    return true;
}

std::string getconfigpath(const ModInfo& info) {
    if (!Configuration::configPathSet) {
        Configuration::configPath = string_format(CONFIG_PATH_FORMAT, Modloader::getApplicationId().c_str());
        if (!direxists(Configuration::configPath.c_str())) {
            mkpath(Configuration::configPath.data(), 0700);
        }
        Configuration::configPathSet = true;
    }
    return Configuration::configPath + info.id + ".json";
}

#endif /* CONFIG_DEFINED_H */