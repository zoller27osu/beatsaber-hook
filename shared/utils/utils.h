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

#include "typedefs.h"
#include "utils-functions.h"
#include "../inline-hook/And64InlineHook.hpp"
#include "il2cpp-utils.hpp"
#include "../config/config-utils.hpp"

#ifdef __cplusplus
class Register {
public:
    int_fast8_t num;
    Register(int_fast8_t reg) : num(reg) {};
    std::string toString();
    friend std::ostream& operator<<(std::ostream& os, const Register& n);
};

class Instruction {
public:
    const int32_t* addr;  // the pointer to the instruction
    // Rd and Rs are capitalized in accordance with typical Register notation
    int_fast8_t Rd = -2;  // the destination register's number, or -1 if none
    int numSourceRegisters = -1;  // the number of source registers this instruction has
    union {
        int_fast8_t Rs[2] = { -1, -1 };  // the number(s) of the source register(s), e.g. {12, 31} for X12 & SP
        int64_t result;  // iff numSourceRegisters is 0, the value that will be stored to Rd by this instruction
    };
    int64_t imm = 0;  // the immediate, if applicable
    enum ShiftType {
        LSL, LSR, ASR, ROR, none
    } shiftType = none;
    bool parsed;  // whether the instruction was fully and successfully parsed
    bool valid = true;  // iff parsed, whether the instruction is a valid one

    Instruction(const int32_t* inst);
    std::string toString();
    friend std::ostream& operator<<(std::ostream& os, const Instruction& inst);
private:
    const char* kind[3];  // strings describing the kind of instruction, from least to most specific
    char parseLevel;  // The lowest level we were able to parse at, 1-3 (subtract 1 for index of most specific string in 'kind')
};

// Returns the value of the bits in x at index high through low inclusive, where the LSB is index 0 and the MSB's index >= high.
template<class T>
T bits(T x, unsigned char high, unsigned char low) {
    typedef typename std::make_unsigned<T>::type unsignedT;
    T noLeft = x << (sizeof(T) * CHAR_BIT - 1 - high);
    T trimmed = *reinterpret_cast<unsignedT*>(&noLeft) >> (sizeof(T) * CHAR_BIT - 1 - high + low);
    return trimmed;
}

// Transforms the given integer (with M denoting the number of significant bits) into a properly signed number of type To.
template<class To, class From>
To SignExtend(From bits, char M) {
    constexpr char N = sizeof(To) * CHAR_BIT;
    assert(N >= M);
    auto prep = ((To)bits) << (N - M);
    return (prep >> (N - M));
}

// Wrapper for easier use (no need to cast the pointer to void*)
template<class T>
void analyzeBytes(const T* ptr) {
    analyzeBytes((const void*)ptr);
}

extern "C" {
#endif /* __cplusplus */

// Restores an existing stringstream to a newly created state.
void resetSS(std::stringstream& ss);
// Prints the given number of "tabs" as spaces to the given output stream.
void tabs(std::ostream& os, int tabs, int spacesPerTab = 2);
// Logs the given stringstream and clears it.
void print(std::stringstream& ss, LOG_VERBOSE_TYPE lvl = INFO);

// Attempts to print what is stored at the given pointer.
// For a given pointer, it will scan 4 void*'s worth of bytes at the location pointed to.
// For each void* of bytes, it will print the raw bytes and interpretations of the bytes as ints and char*s.
// When the bytes look like a valid pointer, it will attempt to follow that pointer, increasing the indentation.
//   It will not follow pointers that it has already analyzed as a result of the current call.
void analyzeBytes(const void* ptr);

long long getRealOffset(const void* offset);
long long baseAddr(const char* soname);

// Only wildcard is ? and ?? - both are handled the same way. They will skip exactly 1 byte (2 hex digits)
long long findPattern(long long dwAddress, const char* pattern, long long dwSearchRangeLen = 0x1000000);
// Same as findPattern but will continue scanning to make sure your pattern is sufficiently specific.
// Each candidate will be logged. label should describe what you're looking for, like "Class::Init".
// Sets "multiple" iff multiple matches are found, and outputs a log warning message.
// Returns the first match, if any.
long long findUniquePattern(bool& multiple, long long dwAddress, const char* pattern, const char* label = 0, long long dwSearchRangeLen = 0x1000000);

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
log(INFO, "Installing 64 bit hook: %s", #name); \
A64HookFunction((void*)getRealOffset(addr_ ## name),(void*) hook_ ## name, (void**)&name); \

#define INSTALL_HOOK_OFFSETLESS(name, methodInfo) \
log(INFO, "Installing 64 bit offsetless hook: %s", #name); \
A64HookFunction((void*)methodInfo->methodPointer,(void*) hook_ ## name, (void**)&name); \

#define INSTALL_HOOK_NAT(name) \
log(INFO, "Installing 64 bit native hook: %s", #name); \
A64HookFunction((void*)(addr_ ## name),(void*) hook_ ## name, (void**)&name); \

#define INSTALL_HOOK_DIRECT(name, addr) \
log(INFO, "Installing 64 bit direct hook: %s", #name); \
A64HookFunction((void*)addr, (void*) hook_ ## name, (void**)&name); \

// Uninstalls currently just creates a hook at the hooked address
// and sets the hook to call the original function
// No original trampoline is created when uninstalling a hook, hence the nullptr

#define UNINSTALL_HOOK(name) \
log(INFO, "Uninstalling 64 bit hook: %s", #name); \
A64HookFunction((void*)getRealOffset(addr_ ## name),(void*)&name, (void**)nullptr); \

#define UNINSTALL_HOOK_OFFSETLESS(name, methodInfo) \
log(INFO, "Uninstalling 64 bit offsetless hook: %s", #name); \
A64HookFunction((void*)methodInfo->methodPointer,(void*)&name, (void**)nullptr); \

#define UNINSTALL_HOOK_NAT(name) \
log(INFO, "Uninstalling 64 bit native hook: %s", #name); \
A64HookFunction((void*)(addr_ ## name),(void*)&name, (void**)nullptr); \

#define UNINSTALL_HOOK_DIRECT(name, addr) \
log(INFO, "Uninstalling 64 bit direct hook: %s", #name); \
A64HookFunction((void*)addr, (void*)&name, (void**)nullptr); \

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