#ifndef CONFIG_DEFINED_H
#define CONFIG_DEFINED_H

#include <stddef.h>
#include <utils.h>
#include <config-utils.h>
#include <utils-functions.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

// CONFIG

ConfigDocument Configuration::config;
bool readJson = false;

using namespace std;
using namespace rapidjson;


bool parsejsonfile(ConfigDocument& doc, string filename) {
    if (!fileexists(filename.c_str())) {
        return {};
    }
    // FILE* fp = fopen(filename.c_str(), "r");

    std::ifstream is;
    is.open(filename.c_str());

    IStreamWrapper wrapper {is};
    
    if (doc.ParseStream(wrapper).HasParseError()) {
        return false;
    }
    return true;
}

// Loads the config for the given MOD_ID, if it doesn't exist, will leave it as an empty object.
void Configuration::Load() {
    if (readJson) {
        return;
    }
    // document = {};
    if (!direxists(CONFIG_PATH)) {
        mkdir(CONFIG_PATH, 0700);
    }
    string filename = getconfigpath();

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
    string filename = getconfigpath();

    parsejsonfile(config, filename);
    if (!config.IsObject()) {
        config.SetObject();
    }
    readJson = true;
}

void Configuration::Write() {
    if (!direxists(CONFIG_PATH)) {
        mkdir(CONFIG_PATH, 0700);
    }
    string filename = getconfigpath();

    StringBuffer buf;
    PrettyWriter<StringBuffer> writer(buf);
    config.Accept(writer);
    writefile(filename.c_str(), buf.GetString());
}

#endif /* CONFIG_DEFINED_H */