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
// il2cpp_string_new, used to find string construction offset: 0x2ed144
// Creation of string method(char* chars, size_t length): 0x31A1E1
static const long NEW_STRING_OFFSET = 0x31A1E1;

cs_string* createcsstr(char* characters, size_t length) {
    cs_string* (*create_str)(char*, size_t) = (void*)getRealOffset(NEW_STRING_OFFSET);
    return create_str(characters, length);
    
    // Creates a cs_cstring by doing concatenation, substring, and setting.
    // cs_string* (*concat)(cs_string*, cs_string*) = (void*)getRealOffset(CONCAT_STRING_OFFSET);
    // cs_string* temp = concat(existing, existing);
    // int i = 0;
    // while (temp->len < length) {
    //     if (i > 10000) {
    //         // ERROR!
    //         return NULL;
    //     }
    //     i++;
    //     temp = concat(temp, existing);
    // }

    // cs_string* (*substring)(cs_string*, int, int) = (void*)getRealOffset(SUBSTRING_OFFSET);
    // cs_string* end = substring(temp, 0, length);
    // if (end->len != (int)length) {
    //     // ERROR!
    //     return NULL;
    // }
    // for (int j = 0; j < length; j++) {
    //     end->str[j] = characters[j];
    // }
    // return end;

    // Write padding for C# strings
    // str->padding[0] = 128;
    // str->padding[1] = 67;
    // str->padding[2] = 15;
    // str->padding[3] = 207;
    // Rest are 0s
    // System.string.ctor(char, int): 0x97D778
    // void (*string_ctor)(cs_string*, unsigned short, unsigned int) = (void*)getRealOffset(0x97D778);
    // string_ctor(str, (unsigned short) 44, (unsigned int)length);
    // for (int i = 0; i < length; i++) {
    //     str->str[i] = characters[i];
    //     str->len++;
    // }
    // return str;
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