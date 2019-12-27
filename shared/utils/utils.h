#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

// Location of libil2cpp.so
#define IL2CPP_SO_PATH "/data/app/com.beatgames.beatsaber-1/lib/arm64/libil2cpp.so"

#if __has_include(<string_view>)
#include <string_view>
#elif __has_include(<experimental/string_view>)
#include <experimental/string_view>
namespace std {
    using experimental::string_view;
    using experimental::basic_string_view;
    using experimental::u16string_view;
}
#else
#error No string_view implementation available!
#endif

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "typedefs.h"
#include "../config/config-utils.hpp"
#include "il2cpp-utils.hpp"
#include "utils-functions.h"
#include "../inline-hook/And64InlineHook.hpp"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

long long getRealOffset(void* offset);
long long baseAddr(const char* soname);
// Only wildcard is ? and ?? - both are handled the same way. They will skip exactly 1 byte (2 hex digits)
// TODO: allow ?x / x? for when half of the byte is known
long long FindPattern(long long dwAddress, const char* pattern, long long dwSearchRangeLen = 0x1000000);
// Same as FindPattern but will continue scanning to make sure your pattern is sufficiently specific.
// Each candidate will be logged. label should describe what you're looking for, like "Class::Init".
// Sets "multiple" iff multiple matches are found, and outputs a log warning message.
// Returns the first match, if any.
long long FindUniquePattern(bool& multiple, long long dwAddress, const char* pattern, const char* label = 0, long long dwSearchRangeLen = 0x1000000);

#define MAKE_HOOK(name, addr, retval, ...) \
void* addr_ ## name = (void*) addr; \
retval (*name)(__VA_ARGS__) = NULL; \
retval hook_ ## name(__VA_ARGS__) 

#define MAKE_HOOK_OFFSETLESS(name, retval, ...) \
retval (*name)(__VA_ARGS__) = NULL; \
retval hook_ ## name(__VA_ARGS__)

#define MAKE_HOOK_NAT(name, addr, retval, ...) \
void* addr_ ## name = (void*) addr; \
retval (*name)(__VA_ARGS__) = NULL; \
retval hook_ ## name(__VA_ARGS__) 

#ifdef __aarch64__

#define INSTALL_HOOK(name) \
log(INFO, "Installing 64 bit hook!"); \
A64HookFunction((void*)getRealOffset(addr_ ## name),(void*) hook_ ## name, (void**)&name); \

#define INSTALL_HOOK_OFFSETLESS(name, methodInfo) \
log(INFO, "Installing 64 bit offsetless hook!"); \
A64HookFunction((void*)methodInfo->methodPointer,(void*) hook_ ## name, (void**)&name); \

#define INSTALL_HOOK_NAT(name) \
log(INFO, "Installing 64 bit native hook!"); \
A64HookFunction((void*)(addr_ ## name),(void*) hook_ ## name, (void**)&name); \

#define INSTALL_HOOK_DIRECT(name, addr) \
log(INFO, "Installing 64 bit direct hook!"); \
A64HookFunction((void*)addr, (void*) hook_ ## name, (void**)&name); \

#else

#define INSTALL_HOOK(name) \
log(INFO, "Installing 32 bit hook!"); \
registerInlineHook((uint32_t)getRealOffset(addr_ ## name), (uint32_t)hook_ ## name, (uint32_t **)&name); \
inlineHook((uint32_t)getRealOffset(addr_ ## name)); \

#define INSTALL_HOOK_OFFSETLESS(name, methodInfo) \
log(INFO, "Installing 32 bit offsetless hook!"); \
registerInlineHook((uint32_t)methodInfo->methodPointer, (uint32_t)hook_ ## name, (uint32_t **)&name); \
inlineHook((uint32_t)methodInfo->methodPointer); \

#define INSTALL_HOOK_NAT(name) \
log(INFO, "Installing 32 bit native hook!"); \
registerInlineHook((uint32_t)(addr_ ## name), (uint32_t)hook_ ## name, (uint32_t **)&name); \
inlineHook((uint32_t)(addr_ ## name)); \

#define INSTALL_HOOK_DIRECT(name, addr) \
log(INFO, "Installing 32 bit offsetless hook!"); \
registerInlineHook((uint32_t)addr, (uint32_t)hook_ ## name, (uint32_t **)&name); \
inlineHook((uint32_t)addr); \

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UTILS_H_INCLUDED */