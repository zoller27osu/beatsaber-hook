#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "typedefs.h"
#include "config-utils.h"
#include "rapidjson-utils.h"
#include "il2cpp-utils.h"
#include "utils-functions.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

long getRealOffset(long offset);

#define MAKE_HOOK(name, addr, retval, ...) \
long addr_ ## name = (long) addr; \
retval (*name)(__VA_ARGS__) = NULL; \
retval hook_ ## name(__VA_ARGS__) 

#define INSTALL_HOOK(name) \
registerInlineHook((uint32_t)getRealOffset(addr_ ## name), (uint32_t)hook_ ## name, (uint32_t **)&name);\
inlineHook((uint32_t)getRealOffset(addr_ ## name));\


#define MAKE_HOOK_NAT(name, addr, retval, ...) \
long addr_ ## name = (long) addr; \
retval (*name)(__VA_ARGS__) = NULL; \
retval hook_ ## name(__VA_ARGS__) 

#define INSTALL_HOOK_NAT(name) \
registerInlineHook((uint32_t)(addr_ ## name), (uint32_t)hook_ ## name, (uint32_t **)&name);\
inlineHook((uint32_t)(addr_ ## name));\

// OFFSETS

// DEPRECATED
// // Create an object using il2cpp_object_new offset
// #define OBJ_CREATOR_OFFSET 0x308740
// // GameObject.ctor() offset
// #define GO_CTOR_OFFSET 0xC86558
// // GameObject type offset
// #define GO_TYPE_OFFSET 0x19C7998
// // System.GetType(string typeName) offset
// #define GET_TYPE_OFFSET 0x104B254
// // System.String.Concat(cs_string* left, cs_string* right) offset
// #define CONCAT_STRING_OFFSET 0x972F2C
// // System.String.CreateString(char* array, int start, int length) offset
// #define CREATE_STRING_OFFSET 0x9831BC
// // System.String.FastAllocateString(int length) offset
// #define ALLOCATE_STRING_OFFSET 0x97A704
// // System.String.Substring(cs_string* this, int start, int length) offset
// #define SUBSTRING_OFFSET 0x96EBEC
// // System.String.Replace(cs_string* original, cs_string* old, cs_string* new) offset
// #define STRING_REPLACE_OFFSET 0x97FF04

// SETTINGS

// ParseError is thrown when failing to parse a JSON file
typedef enum ParseError {
    PARSE_ERROR_FILE_DOES_NOT_EXIST = -1
} ParseError_t;

// WriteError is thrown when failing to create a file
typedef enum WriteError {
    WRITE_ERROR_COULD_NOT_MAKE_FILE = -1
} WriteError_t;

// JSON Parse Errors
typedef enum JsonParseError {
    JSON_PARSE_ERROR = -1
} JsonParseError_t;

// CONFIG

#define CONFIG_PATH "/sdcard/Android/data/com.beatgames.beatsaber/files/mod_cfgs/"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UTILS_H_INCLUDED */