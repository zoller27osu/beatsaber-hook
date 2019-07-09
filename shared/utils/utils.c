// thx https://github.com/jbro129/Unity-Substrate-Hook-Android

#include <jni.h>

#include "utils.h"
#include <android/log.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>


long baseAddr(char *soname)  // credits to https://github.com/ikoz/AndroidSubstrate_hookingC_examples/blob/master/nativeHook3/jni/nativeHook3.cy.cpp
{
    void *imagehandle = dlopen(soname, RTLD_LOCAL | RTLD_LAZY);
    if (soname == NULL)
        return NULL;
    if (imagehandle == NULL){
        return NULL;
    }
    uintptr_t * irc = NULL;
    FILE *f = NULL;
    char line[200] = {0};
    char *state = NULL;
    char *tok = NULL;
    char * baseAddr = NULL;
    if ((f = fopen("/proc/self/maps", "r")) == NULL)
        return NULL;
    while (fgets(line, 199, f) != NULL)
    {
        tok = strtok_r(line, "-", &state);
        baseAddr = tok;
        tok = strtok_r(NULL, "\t ", &state);
        tok = strtok_r(NULL, "\t ", &state); // "r-xp" field
        tok = strtok_r(NULL, "\t ", &state); // "0000000" field
        tok = strtok_r(NULL, "\t ", &state); // "01:02" field
        tok = strtok_r(NULL, "\t ", &state); // "133224" field
        tok = strtok_r(NULL, "\t ", &state); // path field

        if (tok != NULL) {
            int i;
            for (i = (int)strlen(tok)-1; i >= 0; --i) {
                if (!(tok[i] == ' ' || tok[i] == '\r' || tok[i] == '\n' || tok[i] == '\t'))
                    break;
                tok[i] = 0;
            }
            {
                size_t toklen = strlen(tok);
                size_t solen = strlen(soname);
                if (toklen > 0) {
                    if (toklen >= solen && strcmp(tok + (toklen - solen), soname) == 0) {
                        fclose(f);
                        return (long)strtoll(baseAddr,NULL,16);
                    }
                }
            }
        }
    }
    fclose(f);
    return NULL;
}

long location; // save lib.so base address so we do not have to recalculate every time causing lag.

long getRealOffset(long offset) // calculate dump.cs address + lib.so base address.
{
    if (location == 0)
    {
        //arm
        location = baseAddr("/data/app/com.beatgames.beatsaber-1/lib/arm/libil2cpp.so"); // replace the com.package.name with the package name of the app you are modding.
    }
    return location + offset;
}

// BEAT SABER SPECIFIC

// Create an object using garbage collection offset
const long GC_CREATOR_OFFSET = 0x308740;
// GameObject.ctor() offset
const long GO_CTOR_OFFSET = 0xc86558;
// GameObject type offset
const long GO_TYPE_OFFSET = 0x19c7998;
// System.GetType(string typeName) offset
const long GET_TYPE_OFFSET = 0x104B254;
// System.String.Concat(cs_string* left, cs_string* right) offset
const long CONCAT_STRING_OFFSET = 0x972F2C;
// System.String.CreateString(char* array, int start, int length) offset
const long CREATE_STRING_OFFSET = 0x9831BC;
// System.String.FastAllocateString(int length) offset
const long ALLOCATE_STRING_OFFSET = 0x97A704;
// System.String.Substring(cs_string* this, int start, int length) offset
const long SUBSTRING_OFFSET = 0x96EBEC;
// il2cpp_string_new, used to find string construction offset: 0x2DD144
// il2cpp_string_new immediate call offset: 0x30A1C8
// Creation of string method(char* chars, size_t length): 0x30A1E8
static const long NEW_STRING_OFFSET = 0x30A1E8;
// System.String.Replace(cs_string* original, cs_string* old, cs_string* new) offset
const long STRING_REPLACE_OFFSET = 0x97FF04;

cs_string* createcsstr(char* characters, size_t length) {
    cs_string* (*create_str)(char*, size_t) = (void*)getRealOffset(NEW_STRING_OFFSET);
    return create_str(characters, length);
}

void csstrtowstr(cs_string* in, unsigned short* out)
{
    for(int i = 0; i < in->len; i++) {
        out[i] = in->str[i];
    }
}

void setcswstr(cs_string* in, unsigned short* value, size_t length) {
    unsigned int l = (unsigned int)length;
    in->len = l;
    for(int i = 0; i < l; i++) {
        in->str[i] = value[i];
    }
}

void setcsstr(cs_string* in, char* value, size_t length) {
    unsigned int l = (unsigned int)length;
    in->len = l;
    for(int i = 0; i < l; i++) {
        // Can assume that each char is only a single char (a single word --> double word)
        in->str[i] = (unsigned short)value[i];
    }
}

void csstrtostr(cs_string* in, char* out)
{
    for(int i = 0; i < in->len; i++) {
        // Can assume that each short can just be a literal char
        out[i*2] = in->str[i];
    }
    out[in->len] = '\x0';
}

// BEAT SABER SETTINGS