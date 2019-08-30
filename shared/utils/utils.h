long getRealOffset(long offset);

typedef struct {
    char padding[0x8];
    unsigned int len;
    char str[];
} cs_string;


#define log(...) __android_log_print(ANDROID_LOG_INFO, "QuestHook", __VA_ARGS__)

#define MAKE_HOOK(name, addr, retval, ...) \
long addr_ ## name = (long) addr; \
retval (*name)(__VA_ARGS__) = NULL; \
retval hook_ ## name(__VA_ARGS__) 

#define MAKE_HOOK_NAT(name, addr, retval, ...) \
long addr_ ## name = (long) addr; \
retval (*name)(__VA_ARGS__) = NULL; \
retval hook_ ## name(__VA_ARGS__) 


#if defined(__aarch64__)

#define INSTALL_HOOK(name) \
A64HookFunction((void*)getRealOffset(addr_ ## name),(void*) hook_ ## name, (void**)&name);\


#define INSTALL_HOOK_NAT(name) \
A64HookFunction((void*)(addr_ ## name),(void*) hook_ ## name, (void**)&name);\


#else

#define INSTALL_HOOK(name) \
registerInlineHook((uint32_t)getRealOffset(addr_ ## name), (uint32_t)hook_ ## name, (uint32_t **)&name);\
inlineHook((uint32_t)getRealOffset(addr_ ## name));\

#define INSTALL_HOOK_NAT(name) \
registerInlineHook((uint32_t)(addr_ ## name), (uint32_t)hook_ ## name, (uint32_t **)&name);\
inlineHook((uint32_t)(addr_ ## name));\

#endif

void csstrtostr(cs_string* in, char* out);