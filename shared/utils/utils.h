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

// UnityEngine.Vector3
typedef struct {
    float x;
    float y;
    float z;
} Vector3;

// Create an object using garbage collection offset
extern const long GC_CREATOR_OFFSET;
// GameObject.ctor() offset
extern const long GO_CTOR_OFFSET;
// GameObject type offset
extern const long GO_TYPE_OFFSET;
// System.GetType(string typeName) offset
extern const long GET_TYPE_OFFSET;
// System.String.CreateString(cs_string* left, cs_string* right) offset
extern const long CONCAT_STRING_OFFSET;
// System.String.CreateString(char* array, int start, int length) offset
extern const long CREATE_STRING_OFFSET;
// System.String.FastAllocateString(int length) offset
extern const long ALLOCATE_STRING_OFFSET;

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