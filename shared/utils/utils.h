#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#ifndef JSMN_INCLUDED

#define JSMN_INCLUDED
#define JSMN_STATIC
#include "../../jsmn/jsmn.h"

#endif /* JSMN_INCLUDED */

long getRealOffset(long offset);

#define log(...) __android_log_print(ANDROID_LOG_INFO, "QuestHook", __VA_ARGS__)

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

// C# SPECIFIC

// System.string
typedef struct {
    char padding[0x8];
    unsigned int len;
    unsigned short str[];
} cs_string;

// BEAT SABER SPECIFIC

// UnityEngine.Color
typedef struct {
    float r;
    float g;
    float b;
    float a;
} Color;

// UnityEngine.Vector2
typedef struct {
    float x;
    float y;
} Vector2;

// UnityEngine.Vector3
typedef struct {
    float x;
    float y;
    float z;
} Vector3;

// OFFSETS

// Create an object using garbage collection offset
#define GC_CREATOR_OFFSET 0x308740
// GameObject.ctor() offset
#define GO_CTOR_OFFSET 0xC86558
// GameObject type offset
#define GO_TYPE_OFFSET 0x19C7998
// System.GetType(string typeName) offset
#define GET_TYPE_OFFSET 0x104B254
// System.String.Concat(cs_string* left, cs_string* right) offset
#define CONCAT_STRING_OFFSET 0x972F2C
// System.String.CreateString(char* array, int start, int length) offset
#define CREATE_STRING_OFFSET 0x9831BC
// System.String.FastAllocateString(int length) offset
#define ALLOCATE_STRING_OFFSET 0x97A704
// System.String.Substring(cs_string* this, int start, int length) offset
#define SUBSTRING_OFFSET 0x96EBEC
// System.String.Replace(cs_string* original, cs_string* old, cs_string* new) offset
#define STRING_REPLACE_OFFSET 0x97FF04

// FUNCTIONS

// Sets the unsigned short array of the given cs_string
void csstrtowstr(cs_string* in, unsigned short* out);
// Sets the character array of the given cs_string
void csstrtostr(cs_string* in, char* out);
// Sets the given cs_string using the given unsigned short array and length
void setcswstr(cs_string* in, unsigned short* value, size_t length);
// Sets the given cs_string using the given character array and length
void setcsstr(cs_string* in, char* value, size_t length);
// Creates a cs string (allocates it) with the given character array and length and returns it
cs_string* createcsstr(char* characters, size_t length);

// SETTINGS

// ParseError is thrown when failing to parse a JSON file
typedef enum ParseError {
    PARSE_ERROR_FILE_DOES_NOT_EXIST = -1
} ParseError_t;

// WriteError is thrown when failing to create a file
typedef enum WriteError {
    WRITE_ERROR_COULD_NOT_MAKE_FILE = -1
} WriteError_t;

// Returns if a file exists and can be written to / read from.
char fileexists(const char* filename);
// Reads all of the text of a file at the given filename. If the file does not exist, returns NULL
char* readfile(const char* filename);
// Writes all of the text to a file at the given filename. Returns either 0 or WriteError code
int writefile(const char* filename, const char* text);
// Parses a JSON string, given an array of tokens and a maximum number of tokens and returns the number of tokens or JSMN_ERROR code
int parsejson(const char* js, jsmntok_t** tokens, const unsigned int count);
// Parses the JSON of the filename, returning the number of tokens parsed or ParseError code
int parsejsonfile(const char* filename, jsmntok_t** tokens, const unsigned int token_count);
// Returns the buffer string from a given JSON string and token. RELATIVELY UNSAFE
char* bufferfromtoken(const char* js, jsmntok_t token);
// Returns the integer from the JSON string at the provided token. NO ERROR CHECKING
int intfromjson(const char* js, jsmntok_t token);
// Returns the double from the JSON string at the provided token. NO ERROR CHECKING
double doublefromjson(const char* js, jsmntok_t token);
// Returns the boolean from the JSON string at the provided token. NO ERROR CHECKING
char boolfromjson(const char* js, jsmntok_t token);

#endif /* UTILS_H_INCLUDED */