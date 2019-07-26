#ifndef IL2CPP_UTILS_H
#define IL2CPP_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "typedefs.h"
#include <string>
#include <string_view>


// Code courtesy of DaNike
template<typename TRet, typename ...TArgs>
// A generic function pointer, which can be called with and set to a `getRealOffset` call
using function_ptr_t = TRet(*)(TArgs...);

namespace il2cpp_utils {
    static bool __cachedClassFromName = false;
    static Il2CppDomain* (*domain_get)(void);
    static const Il2CppAssembly** (*domain_get_assemblies)(Il2CppDomain*, size_t*);
    static const Il2CppImage* (*get_image)(const Il2CppAssembly*);
    static Il2CppClass* (*get_class)(const Il2CppImage*, const char*, const char*);
    // Returns the first matching class from the given namespace and typeName by searching through all assemblies that are loaded.
    inline Il2CppClass* GetClassFromName(const char* nameSpace, const char* typeName) {
        if (!__cachedClassFromName) {
            void *imagehandle = dlopen("/data/app/com.beatgames.beatsaber-1/lib/arm/libil2cpp.so", 0x00000 | 0x00001);
            *(void**)(&domain_get) = dlsym(imagehandle, "il2cpp_domain_get");
            *(void**)(&domain_get_assemblies) = dlsym(imagehandle, "il2cpp_domain_get_assemblies");
            *(void**)(&get_image) = dlsym(imagehandle, "il2cpp_assembly_get_image");
            *(void**)(&get_class) = dlsym(imagehandle, "il2cpp_class_from_name");
            dlclose(imagehandle);
            __cachedClassFromName = true;
        }

        size_t assemb_count;
        const Il2CppAssembly** allAssemb = domain_get_assemblies(domain_get(), &assemb_count);
        // const Il2CppAssembly** allAssemb = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &assemb_count);

        for (int i = 0; i < assemb_count; i++) {
            auto assemb = allAssemb[i];
            // auto img = il2cpp_assembly_get_image(assemb);
            // auto klass = il2cpp_class_from_name(img, nameSpace, typeName);
            auto img = get_image(assemb);
            auto klass = get_class(img, nameSpace, typeName);
            if (klass) {
                return klass;
            }
        }
        return NULL;
    }
    static bool __cachedNew = false;
    static Il2CppObject* (*object_new)(Il2CppClass*);
    static const MethodInfo* (*class_get_methods)(Il2CppClass*, void** iter);
    static bool (*type_equals)(const Il2CppType*, const Il2CppType*);
    static Il2CppObject* (*runtime_invoke)(const MethodInfo*, void*, void**, Il2CppException**);

    template<typename TObj, typename... TArgs>
    // Creates a new object of the given class and Il2CppTypes parameters
    inline TObj* New(Il2CppClass* klass, TArgs* ...args) {
        if (!__cachedNew) {
            void *imagehandle = dlopen("/data/app/com.beatgames.beatsaber-1/lib/arm/libil2cpp.so", 0x00000 | 0x00001);
            *(void**)(&object_new) = dlsym(imagehandle, "il2cpp_object_new");
            *(void**)(&class_get_methods) = dlsym(imagehandle, "il2cpp_class_get_methods");
            *(void**)(&type_equals) = dlsym(imagehandle, "il2cpp_type_equals");
            *(void**)(&runtime_invoke) = dlsym(imagehandle, "il2cpp_runtime_invoke");
            dlclose(imagehandle);
            __cachedNew = true;
        }

        void* invoke_params[] = {reinterpret_cast<void*>(args)...};
        // object_new call
        auto obj = object_new(klass);
        // runtime_invoke constructor with right number of args, return null if multiple matches (or take a vector of type pointers to resolve it), return null if constructor errors
        void* myIter = nullptr;
        const MethodInfo* current;
        const MethodInfo* ctor = nullptr;
        constexpr auto count = sizeof...(TArgs);
        Il2CppType* argarr[] = {reinterpret_cast<Il2CppType*>(args)...};
        while ((current = class_get_methods(klass, &myIter))) {
            if (ctor->parameters_count != count + 1) {
                continue;
            }
            // Start at 1 to ignore 'self' param
            for (int i = 1; i < current->parameters_count; i++) {
                if (!type_equals(current->parameters[i].parameter_type, argarr[i - 1])) {
                    goto next_method;
                }
            }
            ctor = current;
            next_method:;
        }
        if (!ctor) {
            return nullptr;
        }
        Il2CppException* exp = nullptr;
        runtime_invoke(ctor, obj, invoke_params, &exp);
        if (exp) {
            return nullptr;
        }
        return reinterpret_cast<TObj*>(obj);
    }

    static bool __cachedCreateStr = false;
    static cs_string* (*string_new_len)(const char*, uint32_t);
    // Creates a cs string (allocates it) with the given string_view and returns it
    inline cs_string* createcsstr(std::string_view inp) {
        if (!__cachedCreateStr) {
            void *imagehandle = dlopen("/data/app/com.beatgames.beatsaber-1/lib/arm/libil2cpp.so", 0x00000 | 0x00001);
            *(void**)(&string_new_len) = dlsym(imagehandle, "il2cpp_string_new_len");
            dlclose(imagehandle);
            __cachedCreateStr = true;
        }
        return string_new_len(inp.data(), inp.length());
    }
}
#endif /* IL2CPP_UTILS_H */
