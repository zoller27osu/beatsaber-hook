#ifndef IL2CPP_UTILS_H
#define IL2CPP_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "typedefs.h"
#include "il2cpp-functions.h"
#include "utils-functions.h"
#include "logging.h"
#include <string>
#include <string_view>

// Code courtesy of DaNike
template<typename TRet, typename ...TArgs>
// A generic function pointer, which can be called with and set to a `getRealOffset` call
using function_ptr_t = TRet(*)(TArgs...);

namespace il2cpp_utils {
    namespace array_utils {
        char* il2cpp_array_addr_with_size(Il2CppArray *array, int32_t size, uintptr_t idx);
        #define load_array_elema(arr, idx, size) ((((uint8_t*)(arr)) + kIl2CppSizeOfArray) + ((size) * (idx)))

        #define il2cpp_array_setwithsize(array, elementSize, index, value)  \
            do {    \
                void*__p = (void*) il2cpp_array_addr_with_size ((array), elementSize, (index)); \
                memcpy(__p, &(value), elementSize); \
            } while (0)
        #define il2cpp_array_setrefwithsize(array, elementSize, index, value)  \
            do {    \
                void*__p = (void*) il2cpp_array_addr_with_size ((array), elementSize, (index)); \
                memcpy(__p, value, elementSize); \
                } while (0)
        #define il2cpp_array_addr(array, type, index) ((type*)(void*) il2cpp_array_addr_with_size (array, sizeof (type), index))
        #define il2cpp_array_get(array, type, index) ( *(type*)il2cpp_array_addr ((array), type, (index)) )
        #define il2cpp_array_set(array, type, index, value)    \
            do {    \
                type *__p = (type *) il2cpp_array_addr ((array), type, (index));    \
                *__p = (value); \
            } while (0)
        #define il2cpp_array_setref(array, index, value)  \
            do {    \
                void* *__p = (void* *) il2cpp_array_addr ((array), void*, (index)); \
                /* il2cpp_gc_wbarrier_set_arrayref ((array), __p, (MonoObject*)(value));    */\
                *__p = (value);    \
            } while (0)
    }
    // Init all of the usable il2cpp API, if it has yet to be initialized
    inline void InitFunctions() {
        if (!il2cpp_functions::initialized) {
            log(WARNING, "il2cpp_utils: GetClassFromName: IL2CPP Functions Not Initialized!");
            il2cpp_functions::Init();
        }
    }
    // Maximum length of characters of an exception message - 1
    #define EXCEPTION_MESSAGE_SIZE 4096
    // Returns a legible string from an Il2CppException*
    inline std::string ExceptionToString(Il2CppException* exp) {
        char msg[EXCEPTION_MESSAGE_SIZE];
        il2cpp_functions::format_exception(exp, msg, EXCEPTION_MESSAGE_SIZE);
        // auto exception_message = csstrtostr(exp->message);
        // return to_utf8(exception_message);
        return msg;
    }

    // Returns the first matching class from the given namespace and typeName by searching through all assemblies that are loaded.
    inline Il2CppClass* GetClassFromName(const char* nameSpace, const char* typeName) {
        InitFunctions();

        size_t assemb_count;
        const Il2CppAssembly** allAssemb = il2cpp_functions::domain_get_assemblies(il2cpp_functions::domain_get(), &assemb_count);
        // const Il2CppAssembly** allAssemb = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &assemb_count);

        for (int i = 0; i < assemb_count; i++) {
            auto assemb = allAssemb[i];
            // auto img = il2cpp_assembly_get_image(assemb);
            // auto klass = il2cpp_class_from_name(img, nameSpace, typeName);
            auto img = il2cpp_functions::assembly_get_image(assemb);
            auto klass = il2cpp_functions::class_from_name(img, nameSpace, typeName);
            if (klass) {
                return klass;
            }
        }
        log(ERROR, "il2cpp_utils: GetClassFromName: Could not find class with namepace: %s and name: %s", nameSpace, typeName);
        return NULL;
    }

    template<typename TObj, typename... TArgs>
    // Creates a new object of the given class and Il2CppTypes parameters and casts it to TObj*
    inline TObj* New(Il2CppClass* klass, TArgs* ...args) {
        InitFunctions();

        void* invoke_params[] = {reinterpret_cast<void*>(args)...};
        // object_new call
        auto obj = il2cpp_functions::object_new(klass);
        // runtime_invoke constructor with right number of args, return null if multiple matches (or take a vector of type pointers to resolve it), return null if constructor errors
        void* myIter = nullptr;
        const MethodInfo* current;
        const MethodInfo* ctor = nullptr;
        constexpr auto count = sizeof...(TArgs);
        Il2CppType* argarr[] = {reinterpret_cast<Il2CppType*>(args)...};
        while ((current = il2cpp_functions::class_get_methods(klass, &myIter))) {
            if (ctor->parameters_count != count + 1) {
                continue;
            }
            // Start at 1 to ignore 'self' param
            for (int i = 1; i < current->parameters_count; i++) {
                if (!il2cpp_functions::type_equals(current->parameters[i].parameter_type, argarr[i - 1])) {
                    goto next_method;
                }
            }
            ctor = current;
            next_method:;
        }
        if (!ctor) {
            log(ERROR, "il2cpp_utils: New: Could not find constructor for provided class!");
            return nullptr;
        }
        // TODO FIX CTOR CHECKING
        if (strcmp(ctor->name, ".ctor") != 0) {
            log(ERROR, "il2cpp_utils: New: Found a method matching parameter count and types, but it is not a constructor!");
            return nullptr;
        }
        Il2CppException* exp = nullptr;
        il2cpp_functions::runtime_invoke(ctor, obj, invoke_params, &exp);
        if (exp) {
            log(ERROR, "il2cpp_utils: New: Failed with exception: %s", ExceptionToString(exp).c_str());
            return nullptr;
        }
        return reinterpret_cast<TObj*>(obj);
    }

    template<typename TObj, typename... TArgs>
    // Creates a New object of the given class and parameters and casts it to TObj*
    // DOES NOT PERFORM TYPE-SAFE CHECKING!
    inline TObj* NewUnsafe(Il2CppClass* klass, TArgs* ...args) {
        InitFunctions();

        void* invoke_params[] = {reinterpret_cast<void*>(args)...};
        // object_new call
        auto obj = il2cpp_functions::object_new(klass);
        // runtime_invoke constructor with right number of args, return null if multiple matches, return null if constructor errors
        void* myIter = nullptr;
        constexpr auto count = sizeof...(TArgs);

        const MethodInfo* ctor = il2cpp_functions::class_get_method_from_name(klass, ".ctor", count + 1);

        if (!ctor) {
            log(ERROR, "il2cpp_utils: New: Could not find constructor for provided class!");
            return nullptr;
        }
        // TODO FIX CTOR CHECKING
        Il2CppException* exp = nullptr;
        il2cpp_functions::runtime_invoke(ctor, obj, invoke_params, &exp);
        if (exp) {
            log(ERROR, "il2cpp_utils: New: Failed with exception: %s", ExceptionToString(exp).c_str());
            return nullptr;
        }
        return reinterpret_cast<TObj*>(obj);
    }
    // Calls the System.RuntimeType.MakeGenericType(System.Type gt, System.Type[] types) function
    inline Il2CppReflectionType* MakeGenericType(Il2CppReflectionType* gt, Il2CppArray* types) {
        InitFunctions();

        auto runtimeType = GetClassFromName("System", "RuntimeType");
        if (!runtimeType) {
            log(ERROR, "il2cpp_utils: MakeGenericType: Failed to get System.RuntimeType!");
            return nullptr;
        }
        auto makeGenericMethod = il2cpp_functions::class_get_method_from_name(runtimeType, "MakeGenericType", 2);
        if (!makeGenericMethod) {
            log(ERROR, "il2cpp_utils: MakeGenericType: Failed to get RuntimeType.MakeGenericType(param1, param2) method!");
            return nullptr;
        }
        Il2CppException* exp = nullptr;
        void* params[] = {reinterpret_cast<void*>(gt), reinterpret_cast<void*>(types)};
        auto genericType = il2cpp_functions::runtime_invoke(makeGenericMethod, nullptr, params, &exp);
        if (exp) {
            log(ERROR, "il2cpp_utils: MakeGenericType: Failed with exception: %s", ExceptionToString(exp).c_str());
            return nullptr;
        }
        return reinterpret_cast<Il2CppReflectionType*>(genericType);
    }

    // Creates a cs string (allocates it) with the given string_view and returns it
    inline Il2CppString* createcsstr(std::string_view inp) {
        InitFunctions();
        return il2cpp_functions::string_new_len(inp.data(), (uint32_t)inp.length());
    }

    // Returns if a given source object is an object of the given class
    [[nodiscard]] inline bool Match(const Il2CppObject* source, const Il2CppClass* klazz) noexcept {
        return (source->klass == klazz);
    }
    // Asserts that a given source object is an object of the given class
    inline bool AssertMatch(const Il2CppObject* source, const Il2CppClass* klazz) {
        InitFunctions();
        if (!Match(source, klazz)) {
            log(CRITICAL, "il2cpp_utils: AssertMatch: Unhandled subtype: namespace %s, class %s", 
                il2cpp_functions::class_get_namespace(source->klass), il2cpp_functions::class_get_name(source->klass));
            std::terminate();
        }
        return true;
    }

    template<class To, class From>
    // Downcasts a class from From* to To*
    [[nodiscard]] inline auto down_cast(From* in) noexcept {
        static_assert(std::is_convertible<To*, From*>::value);
        return static_cast<To*>(in);
    }
}
#endif /* IL2CPP_UTILS_H */
