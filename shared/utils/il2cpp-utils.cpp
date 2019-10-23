#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string>
#include <string_view>
#include "il2cpp-utils.hpp"
#include "logging.h"

namespace il2cpp_utils {
    Il2CppClass* GetClassFromName(const char* name_space, const char* type_name) {
        InitFunctions();

        size_t assemb_count;
        const Il2CppAssembly** allAssemb = il2cpp_functions::domain_get_assemblies(il2cpp_functions::domain_get(), &assemb_count);
        // const Il2CppAssembly** allAssemb = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &assemb_count);

        for (int i = 0; i < assemb_count; i++) {
            auto assemb = allAssemb[i];
            // auto img = il2cpp_assembly_get_image(assemb);
            // auto klass = il2cpp_class_from_name(img, name_space, type_name);
            auto img = il2cpp_functions::assembly_get_image(assemb);
            if (!img) {
                log_print(ERROR, "Assembly with name: %s has a null image!", assemb->aname.name);
                continue;
            }
            auto klass = il2cpp_functions::class_from_name(img, name_space, type_name);
            if (klass) {
                return klass;
            }
        }
        log_print(ERROR, "il2cpp_utils: GetClassFromName: Could not find class with namepace: %s and name: %s", name_space, type_name);
        return nullptr;
    }

    Il2CppReflectionType* MakeGenericType(Il2CppReflectionType* gt, Il2CppArray* types) {
        InitFunctions();

        auto runtimeType = GetClassFromName("System", "RuntimeType");
        if (!runtimeType) {
            log_print(ERROR, "il2cpp_utils: MakeGenericType: Failed to get System.RuntimeType!");
            return nullptr;
        }
        auto makeGenericMethod = il2cpp_functions::class_get_method_from_name(runtimeType, "MakeGenericType", 2);
        if (!makeGenericMethod) {
            log_print(ERROR, "il2cpp_utils: MakeGenericType: Failed to get RuntimeType.MakeGenericType(param1, param2) method!");
            return nullptr;
        }

        Il2CppException* exp = nullptr;
        void* params[] = {reinterpret_cast<void*>(gt), reinterpret_cast<void*>(types)};
        auto genericType = il2cpp_functions::runtime_invoke(makeGenericMethod, nullptr, params, &exp);
        if (exp) {
            log_print(ERROR, "il2cpp_utils: MakeGenericType: Failed with exception: %s", ExceptionToString(exp).c_str());
            return nullptr;
        }
        return reinterpret_cast<Il2CppReflectionType*>(genericType);
    }

    Il2CppClass* MakeGeneric(const Il2CppClass* klass, std::initializer_list<const Il2CppClass*> args) {
        InitFunctions();
 
        auto typ = GetClassFromName("System", "Type");
        if (!typ) {
            return nullptr;
        }
        auto getType = il2cpp_functions::class_get_method_from_name(typ, "GetType", 1);
        if (!getType) {
            log_print(ERROR, "il2cpp_utils: MakeGeneric: Failed to get System.Type.GetType(param1) method!");
            return nullptr;
        }
 
        auto klassType = il2cpp_functions::type_get_object(il2cpp_functions::class_get_type_const(klass));
        if (!klassType) {
            log_print(ERROR, "il2cpp_utils: MakeGeneric: Failed to get class type object!");
            return nullptr;
        }
 
        // Call Type.MakeGenericType on it
        auto a = il2cpp_functions::array_new_specific(typ, args.size());
        if (!a) {
            log_print(ERROR, "il2cpp_utils: MakeGeneric: Failed to make new array with length: %zu", args.size());
            return nullptr;
        }
 
        int i = 0;
        for (auto arg : args) {
            auto t = il2cpp_functions::class_get_type_const(arg);
            auto o = il2cpp_functions::type_get_object(t);
            if (!o) {
                log_print(ERROR, "il2cpp_utils: MakeGeneric: Failed to get type for %s", il2cpp_functions::class_get_name_const(arg));
                return nullptr;
            }
            il2cpp_array_set(a, void*, i, reinterpret_cast<void*>(o));
            i++;
        }

        auto reflection_type = MakeGenericType(reinterpret_cast<Il2CppReflectionType*>(klassType), a);
        if (!reflection_type) {
            log_print(ERROR, "il2cpp_utils: MakeGeneric: Failed to MakeGenericType from Il2CppReflectionType and Il2CppArray!");
            return nullptr;
        }

        auto ret = il2cpp_functions::class_from_system_type(reinterpret_cast<Il2CppReflectionType*>(reflection_type));
        if (!ret) {
            log_print(ERROR, "il2cpp_utils: MakeGeneric: Failed to get class from Il2CppReflectionType!");
            return nullptr;
        }
        log_print(DEBUG, "il2cpp_utils: MakeGeneric: returning %s", il2cpp_functions::class_get_name(ret));
        return ret;
    }
}