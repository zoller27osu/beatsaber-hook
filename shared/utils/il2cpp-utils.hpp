#ifndef IL2CPP_UTILS_H
#define IL2CPP_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <optional>

#if __has_include(<concepts>)
#include <concepts>
template<typename T>
constexpr bool has_no_arg_get = requires(const T& t) {
    t.get();
};
#elif __has_include(<experimental/type_traits>)
#include <experimental/type_traits>
template<typename T>
using no_arg_get_t = decltype( std::declval<T&>().get() );

template<typename T>
constexpr bool has_no_arg_get = std::experimental::is_detected_v<no_arg_get_t, T>;
#else
#error No libraries for the implementation of has_no_arg_get available!
#endif

#include "il2cpp-functions.hpp"
#include "logging.hpp"
#include "utils.h"

namespace il2cpp_utils {
    namespace array_utils {
        static char* il2cpp_array_addr_with_size(Il2CppArray *array, int32_t size, uintptr_t idx)
        {
            return ((char*)array) + kIl2CppSizeOfArray + size * idx;
        }
        #define load_array_elema(arr, idx, size) ((((uint8_t*)(arr)) + kIl2CppSizeOfArray) + ((size) * (idx)))

        #define il2cpp_array_setwithsize(array, elementSize, index, value)  \
            do {    \
                void*__p = (void*) il2cpp_utils::array_utils::il2cpp_array_addr_with_size ((array), elementSize, (index)); \
                memcpy(__p, &(value), elementSize); \
            } while (0)
        #define il2cpp_array_setrefwithsize(array, elementSize, index, value)  \
            do {    \
                void*__p = (void*) il2cpp_utils::array_utils::il2cpp_array_addr_with_size ((array), elementSize, (index)); \
                memcpy(__p, value, elementSize); \
                } while (0)
        #define il2cpp_array_addr(array, type, index) ((type*)(void*) il2cpp_utils::array_utils::il2cpp_array_addr_with_size (array, sizeof (type), index))
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
    // Maximum length of characters of an exception message - 1
    #define EXCEPTION_MESSAGE_SIZE 4096
    // Returns a legible string from an Il2CppException*
    std::string ExceptionToString(Il2CppException* exp);

    // Returns the first matching class from the given namespace and typeName by searching through all assemblies that are loaded.
    Il2CppClass* GetClassFromName(std::string_view name_space, std::string_view type_name);

    // Seriously, don't un-const the returned Type
    const Il2CppType* MakeRef(const Il2CppType* type);

    // Generally, it's better to just use class_from_type!
    const Il2CppType* UnRef(const Il2CppType* type);

    // Framework provided by DaNike
    // TODO: rewrite these to il2cpp_arg_class with il2cpp_arg_type wrappers? Should make many cases faster.
    namespace il2cpp_type_check {
        // To fix "no member named 'get' in il2cpp_type_check::il2cpp_arg_class<Blah>", define il2cpp_arg_class<Blah>!
        // When the Il2CppClass* would depend only on the type of T (not its value), just use DEFINE_IL2CPP_ARG_TYPE!
        template<typename T>
        struct il2cpp_arg_class { };

        #define DEFINE_IL2CPP_DEFAULT_TYPE(type, fieldName) \
        template<> \
        struct il2cpp_utils::il2cpp_type_check::il2cpp_arg_class<type> { \
            static inline Il2CppClass* get() { \
                il2cpp_functions::Init(); \
                return il2cpp_functions::defaults->fieldName##_class; \
            } \
            static inline Il2CppClass* get(type arg) { return get(); } \
        }

        #define DEFINE_IL2CPP_ARG_TYPE(type, nameSpace, className) \
        template<> \
        struct il2cpp_utils::il2cpp_type_check::il2cpp_arg_class<type> { \
            static inline Il2CppClass* get() { \
                il2cpp_functions::Init(); \
                return il2cpp_utils::GetClassFromName(nameSpace, className); \
            } \
            static inline Il2CppClass* get(type arg) { return get(); } \
        }

        #define DEFINE_IL2CPP_ARG_TYPE_GENERIC(type, postfix, nameSpace, className) \
        template<typename... TArgs> \
        struct ::il2cpp_utils::il2cpp_type_check::il2cpp_arg_class<type<TArgs...>postfix> { \
            static inline Il2CppClass* get() { \
                auto* klass = il2cpp_utils::GetClassFromName(nameSpace, className); \
                return il2cpp_utils::MakeGeneric(klass, {il2cpp_arg_class<TArgs>::get()...}); \
            } \
            static inline Il2CppClass* get(type<TArgs...>postfix arg) { return get(); } \
        }

        DEFINE_IL2CPP_DEFAULT_TYPE(int8_t, sbyte);
        DEFINE_IL2CPP_DEFAULT_TYPE(uint8_t, byte);
        DEFINE_IL2CPP_DEFAULT_TYPE(int16_t, int16);  // "short"
        DEFINE_IL2CPP_DEFAULT_TYPE(uint16_t, uint16);  // "ushort"
        DEFINE_IL2CPP_DEFAULT_TYPE(int32_t, int32);  // "int"
        DEFINE_IL2CPP_DEFAULT_TYPE(uint32_t, uint32);  // "uint"
        DEFINE_IL2CPP_DEFAULT_TYPE(int64_t, int64);  // "long"
        DEFINE_IL2CPP_DEFAULT_TYPE(uint64_t, uint64);  // "ulong"

        DEFINE_IL2CPP_DEFAULT_TYPE(float, single);
        DEFINE_IL2CPP_DEFAULT_TYPE(double, double);

        DEFINE_IL2CPP_DEFAULT_TYPE(bool, boolean);
        DEFINE_IL2CPP_DEFAULT_TYPE(Il2CppChar, char);

        DEFINE_IL2CPP_ARG_TYPE(long double, "System", "Decimal");
        DEFINE_IL2CPP_ARG_TYPE(Color, "UnityEngine", "Color");
        DEFINE_IL2CPP_ARG_TYPE(Vector2, "UnityEngine", "Vector2");
        DEFINE_IL2CPP_ARG_TYPE(Vector3, "UnityEngine", "Vector3");
        DEFINE_IL2CPP_ARG_TYPE(Vector4, "UnityEngine", "Vector4");
        DEFINE_IL2CPP_ARG_TYPE(Quaternion, "UnityEngine", "Quaternion");
        DEFINE_IL2CPP_ARG_TYPE(Rect, "UnityEngine", "Rect");
        DEFINE_IL2CPP_ARG_TYPE(Scene, "UnityEngine.SceneManagement", "Scene");

        template<>
        struct il2cpp_arg_class<Il2CppClass*> {
            static inline Il2CppClass* get(Il2CppClass* arg) {
                return arg;
            }
        };

        template<>
        struct il2cpp_arg_class<Il2CppType*> {
            static inline Il2CppClass* get(Il2CppType* arg) {
                RET_0_UNLESS(arg);
                il2cpp_functions::Init();
                return il2cpp_functions::class_from_il2cpp_type(arg);
            }
        };

        template<>
        struct il2cpp_arg_class<Il2CppObject*> {
            static inline Il2CppClass* get(Il2CppObject* arg) {
                RET_0_UNLESS(arg);
                il2cpp_functions::Init();
                return RET_0_UNLESS(il2cpp_functions::object_get_class(arg));
            }
        };

        template<typename TArg>
        struct il2cpp_arg_class<Array<TArg>*> {
            static inline Il2CppClass* get(Array<TArg>* arg) {
                RET_0_UNLESS(arg);
                il2cpp_functions::Init();
                // try {
                //     auto* klass = il2cpp_functions::object_get_class(arg);
                //     if (klass && klass->klass == klass) return klass;
                // } catch () {}
                if constexpr (std::is_same_v<std::decay_t<TArg>, Il2CppObject*>) {
                    il2cpp_functions::CheckS_GlobalMetadata();
                    return il2cpp_functions::array_class_get(il2cpp_functions::defaults->object_class, 1);
                } else {
                    Il2CppClass* eClass = RET_0_UNLESS(il2cpp_arg_class<TArg>::get());
                    return il2cpp_functions::array_class_get(eClass, 1);
                }
            }
        };

        // Unlike many Il2CppObject*, this has no real subclasses to worry about
        DEFINE_IL2CPP_DEFAULT_TYPE(Il2CppString*, string);

        DEFINE_MEMBER_CHECKER(obj)
        DEFINE_MEMBER_CHECKER(object)
        template<typename T>
        struct il2cpp_arg_class<T*> {
            static inline Il2CppClass* get(T* arg) {
                RET_0_UNLESS(arg);
                // These first 2 conditions handle Il2CppObject subclasses that were created
                //   in libil2cpp via composition instead of inheritance
                auto constexpr hasObj = has_obj<T, Il2CppObject>::value;
                auto constexpr hasObject = has_object<T, Il2CppObject>::value;
                // Double check inheritance here
                if constexpr(std::is_convertible_v<T*, Il2CppObject*>) {
                    return il2cpp_arg_class<Il2CppObject*>::get(arg);
                } else if constexpr(hasObj) {
                    return il2cpp_arg_class<Il2CppObject*>::get(&arg->obj);
                } else if constexpr(hasObject) {
                    return il2cpp_arg_class<Il2CppObject*>::get(&arg->object);
                } else {
                    #ifdef NEED_UNSAFE_CSHARP
                    using arg_class = il2cpp_arg_class<T>;
                    if constexpr (has_no_arg_get<arg_class>) {
                        Il2CppClass* elementClass = arg_class::get();
                        return il2cpp_functions::Class_GetPtrClass(elementClass);
                    } else
                    #endif
                    static_assert(false_t<T*>, "Turning this kind of pointer into an Il2CppClass is not implemented! "
                        "Please pass primitives and structs as themselves instead of taking their address. "
                        "If the pointer should be treatable as Il2CppObject*, please file an issue on sc2ad/beatsaber-hook.");
                }
            }
        };
        #undef has_obj
        #undef has_object

        template<typename T>
        struct il2cpp_arg_type { };

        template<typename T>
        struct il2cpp_arg_type<T&> {
            static inline const Il2CppType* get(T& arg) {
                // A method can store a result back to a non-const ref! Make the type byref!
                Il2CppClass* klass = il2cpp_arg_class<T>::get(arg);
                return &klass->this_arg;
            }
        };

        template<typename T>
        struct il2cpp_arg_type<const T&> {
            static inline const Il2CppType* get(const T& arg) {
                // A method cannot store a result back to a const ref. It is not a C# ref.
                const Il2CppClass* klass = il2cpp_arg_class<const T>::get(arg);
                return &klass->byval_arg;
            }
        };

        template<typename T>
        struct il2cpp_arg_type<T&&> {
            static inline const Il2CppType* get(T&& arg) {
                Il2CppClass* klass = il2cpp_arg_class<T>::get(arg);
                return &klass->byval_arg;
            }
        };

        template<typename T>
        struct il2cpp_arg_ptr {
            static inline void* get(T const& arg) {
                return reinterpret_cast<void*>(&arg);
            }
        };
        template<typename T>
        struct il2cpp_arg_ptr<T*> {
            static inline void* get(T* arg) {
                return reinterpret_cast<void*>(arg);
            }
        };
    }

    std::vector<const Il2CppType*> ClassVecToTypes(std::vector<const Il2CppClass*> seq);

    // Gets the System.Type Il2CppObject* (actually an Il2CppReflectionType*) for an Il2CppClass*
    Il2CppReflectionType* GetSystemType(const Il2CppClass* klass);

    // Gets the System.Type Il2CppObject* (actually an Il2CppReflectionType*) for the class with the given namespace and name
    Il2CppReflectionType* GetSystemType(std::string_view nameSpace, std::string_view className);

    // Gets the standard class name of an Il2CppClass*
    std::string ClassStandardName(Il2CppClass* klass, bool generics = true);

    // Gets a C# name of a type
    const char* TypeGetSimpleName(const Il2CppType* type);

    bool IsInterface(const Il2CppClass* klass);

    template<typename T>
    Il2CppClass* ExtractClass(T&& arg) {
        Il2CppClass* klass = il2cpp_type_check::il2cpp_arg_class<std::decay_t<T>>::get(arg);
        if (!klass) {
            Logger::get().error("ExtractClass: failed to determine class! Tips: instead of nullptr, pass the Il2CppType* or Il2CppClass* of the argument instead!");
        }
        return klass;
    }

    template<typename T>
    const Il2CppType* ExtractType(T&& arg) {
        const Il2CppType* typ = il2cpp_type_check::il2cpp_arg_type<T>::get(arg);
        if (!typ) {
            Logger::get().error("ExtractType: failed to determine type! Tips: instead of nullptr, pass the Il2CppType* or Il2CppClass* of the argument instead!");
        }
        return typ;
    }

    inline auto ExtractTypes() {
        return std::vector<const Il2CppType*>();
    }

    template<typename T, typename... TArgs>
    std::vector<const Il2CppType*> ExtractTypes(T&& arg, TArgs&&... args) {
        auto* tFirst = ExtractType(arg);
        auto tOthers = ExtractTypes(args...);
        if (tFirst) tOthers.insert(tOthers.begin(), tFirst);
        return tOthers;
    }

    // Returns if a given MethodInfo's parameters match the Il2CppType array provided as type_arr
    bool ParameterMatch(const MethodInfo* method, const Il2CppType* const* type_arr, decltype(MethodInfo::parameters_count) count);

    template<typename... TArgs>
    // Returns if a given MethodInfo's parameters match the Il2CppTypes provided as args
    bool ParameterMatch(const MethodInfo* method, TArgs* ...args) {
        constexpr auto count = sizeof...(TArgs);
        Il2CppType* argarr[] = {reinterpret_cast<Il2CppType*>(args)...};
        return ParameterMatch(method, argarr, count);
    }

    // Returns if a given MethodInfo's parameters match the Il2CppTypes provided as a vector
    inline bool ParameterMatch(const MethodInfo* method, std::vector<const Il2CppType*> seq) {
        return ParameterMatch(method, seq.data(), seq.size());
    }

    // Returns the MethodInfo for the method on the given class with the given name and number of arguments
    // Created by zoller27osu
    const MethodInfo* FindMethodUnsafe(Il2CppClass* klass, std::string_view methodName, int argsCount);
    const MethodInfo* FindMethodUnsafe(Il2CppObject* instance, std::string_view methodName, int argsCount);
    const MethodInfo* FindMethodUnsafe(std::string_view nameSpace, std::string_view className, std::string_view methodName, int argsCount);

    // Returns the MethodInfo for the method on the given class with the given name and types of arguments
    // Created by zoller27osu
    const MethodInfo* FindMethod(Il2CppClass* klass, std::string_view methodName, std::vector<const Il2CppType*> argTypes);
    const MethodInfo* FindMethod(Il2CppClass* klass, std::string_view methodName, std::vector<const Il2CppClass*> argClasses);
    const MethodInfo* FindMethod(Il2CppClass* klass, std::string_view methodName, std::vector<std::string_view> argSpaceClass);
    // Returns the MethodInfo for the method on the given class or instance. Also the only non-vector arg types version.
    template <typename T, typename... TArgs>
    // prevents template recursion and ambiguity with the double string version:
    std::enable_if_t<(... && !is_vector<TArgs>::value) && !std::is_convertible_v<T, std::string_view>, const MethodInfo*>
    FindMethod(T&& classOrInstance, std::string_view methodName, TArgs&&... args) {
        if constexpr (sizeof...(TArgs) == 1 && (std::is_integral_v<std::decay_t<TArgs>> && ...)) {
            static_assert(false_t<TArgs...>,
                "FindMethod using argCount is invalid! If argCount is 0 then remove it; otherwise use FindMethodUnsafe!");
        } else {
            auto* klass = RET_0_UNLESS(ExtractClass(classOrInstance));
            if constexpr (sizeof...(TArgs) == 0) {
                return FindMethodUnsafe(klass, methodName, 0);
            } else {
                return FindMethod(klass, methodName, {args...});
            }
        }
    }

    // Returns the MethodInfo for the method on class found via namespace and name with the given other arguments
    template<class... TArgs>
    const MethodInfo* FindMethod(std::string_view nameSpace, std::string_view className, TArgs&&... params) {
        return FindMethod(GetClassFromName(nameSpace, className), params...);
    }

    template<class T>
    void* ExtractValue(T&& arg) {
        il2cpp_functions::Init();

        using Dt = std::decay_t<T>;
        if constexpr (std::is_same_v<Dt, Il2CppType*> || std::is_same_v<Dt, Il2CppClass*>) {
            return nullptr;
        } else if constexpr (std::is_pointer_v<Dt>) {
            if constexpr (std::is_base_of_v<Il2CppObject, std::remove_pointer_t<Dt>>) {
                if (arg) {
                    auto* klass = il2cpp_functions::object_get_class(arg);
                    if (klass && klass->valuetype) {
                        // Arg is an Il2CppObject* of a value type. It needs to be unboxed.
                        return il2cpp_functions::object_unbox(arg);
                    }
                }
            }
            return arg;
        } else {
            return const_cast<Dt*>(&arg);
        }
    }

    inline auto ExtractValues() {
        return std::vector<void*>();
    }

    template<class T, class... TArgs>
    std::vector<void*> ExtractValues(T&& arg, TArgs&& ...args) {
        auto* firstVal = ExtractValue(arg);
        auto otherVals = ExtractValues(args...);
        otherVals.insert(otherVals.begin(), firstVal);  // inserts at front
        return otherVals;
    }

    Il2CppClass* GetParamClass(const MethodInfo* method, int paramIdx);
    Il2CppClass* GetFieldClass(FieldInfo* field);
    bool IsConvertible(const Il2CppType* to, const Il2CppType* from, bool asArgs = true);

    // Like ExtractType, but only returns an Il2CppType* if it can be extracted without an instance of T.
    template<class T>
    const Il2CppType* ExtractIndependentType() {
        using arg_type = il2cpp_type_check::il2cpp_arg_type<T>;
        if constexpr (has_no_arg_get<arg_type>) {
            return arg_type::get();
        }
        return nullptr;
    }

    template<class TOut = Il2CppObject*, bool checkTypes = true, class T, class... TArgs>
    // Runs a MethodInfo with the specified parameters and instance, with return type TOut.
    // Assumes a static method if instance == nullptr. May fail due to exception or wrong name, hence the std::optional.
    std::optional<TOut> RunMethod(T&& instance, const MethodInfo* method, TArgs&& ...params) {
        RET_NULLOPT_UNLESS(method);

        if constexpr (checkTypes && sizeof...(TArgs) > 0) {
            auto typeVec = ExtractTypes(params...);
            RET_NULLOPT_UNLESS(typeVec.size() == sizeof...(TArgs));
            RET_NULLOPT_UNLESS(ParameterMatch(method, typeVec));
        }

        void* inst = ExtractValue(instance);  // null is allowed (for T = Il2CppType* or Il2CppClass*)
        Il2CppException* exp = nullptr;
        auto invokeParamsVec = ExtractValues(params...);
        auto* ret = il2cpp_functions::runtime_invoke(method, inst, invokeParamsVec.data(), &exp);

        // Check if the TOut that the user requested makes sense given the Il2CppObject* we actually got
        if (ret) {
            // By using this instead of ExtractType, we avoid unboxing because the ultimate type in that case would depend on the
            // method in the first place
            auto* outType = ExtractIndependentType<TOut>();
            if (outType) {
                auto* retType = ExtractType(ret);
                if (!IsConvertible(retType, outType, false)) {
                    Logger::get().warning("User requested TOut %s does not match the method's return object of type %s!",
                        TypeGetSimpleName(outType), TypeGetSimpleName(retType));
                }
            }
        }

        // Convert the Il2CppObject* we got from runtime_invoke to TOut.
        TOut out;
        if constexpr (std::is_pointer_v<TOut>) {
            out = reinterpret_cast<TOut>(ret);
        } else {
            out = *reinterpret_cast<TOut*>(il2cpp_functions::object_unbox(ret));
        }

        if (exp) {
            Logger::get().error("il2cpp_utils: RunMethod: %s: Failed with exception: %s", il2cpp_functions::method_get_name(method),
                il2cpp_utils::ExceptionToString(exp).c_str());
            return std::nullopt;
        }
        return out;
    }

    template<class TOut = Il2CppObject*, class T, class... TArgs>
    // Runs a (static) method with the specified method name, with return type TOut.
    // Checks the types of the parameters against the candidate methods.
    std::enable_if_t<!std::is_convertible_v<T, std::string_view>, std::optional<TOut>>
    RunMethod(T&& classOrInstance, std::string_view methodName, TArgs&& ...params) {
        auto types = ExtractTypes(params...);
        if (types.size() != sizeof...(TArgs)) {
            Logger::get().warning("RunMethod: ExtractTypes for method %s failed!", methodName.data());
            return std::nullopt;
        }

        auto* method = RET_NULLOPT_UNLESS(FindMethod(classOrInstance, methodName, types));
        return RunMethod<TOut, false>(classOrInstance, method, params...);
    }

    template<class TOut = Il2CppObject*, class T, class... TArgs>
    // Runs a (static) method with the specified method name and number of arguments, with return type TOut.
    // DOES NOT PERFORM TYPE CHECKING.
    std::enable_if_t<std::is_base_of_v<Il2CppClass, T> || std::is_base_of_v<Il2CppObject, T>, std::optional<TOut>>
    RunMethodUnsafe(T* classOrInstance, std::string_view methodName, TArgs&& ...params) {
        RET_NULLOPT_UNLESS(classOrInstance);
        auto* method = RET_NULLOPT_UNLESS(FindMethodUnsafe(classOrInstance, methodName, sizeof...(TArgs)));
        return RunMethod<TOut, false>(classOrInstance, method, params...);
    }

    template<class TOut = Il2CppObject*, class... TArgs>
    // Runs a static method with the specified method name and arguments, on the class with the specified namespace and class name.
    // The method also has return type TOut.
    std::optional<TOut> RunMethod(std::string_view nameSpace, std::string_view klassName, std::string_view methodName, TArgs&& ...params) {
        auto* klass = RET_NULLOPT_UNLESS(GetClassFromName(nameSpace, klassName));
        return RunMethod<TOut>(klass, methodName, params...);
    }

    template<class TOut = Il2CppObject*, class... TArgs>
    // Runs a static method with the specified method name and arguments, on the class with the specified namespace and class name.
    // The method also has return type TOut.
    // DOES NOT PERFORM TYPE CHECKING.
    std::optional<TOut> RunMethodUnsafe(std::string_view nameSpace, std::string_view klassName, std::string_view methodName, TArgs&& ...params) {
        auto* klass = RET_NULLOPT_UNLESS(GetClassFromName(nameSpace, klassName));
        return RunMethodUnsafe<TOut>(klass, methodName, params...);
    }

    template<typename TObj = Il2CppObject, typename... TArgs>
    // Creates a new object of the given class using the given constructor parameters and casts it to TObj*
    // Will only run a .ctor whose parameter types match the given arguments.
    TObj* New(Il2CppClass* klass, TArgs const& ...args) {
        il2cpp_functions::Init();

        // object_new call
        auto obj = il2cpp_functions::object_new(klass);
        // runtime_invoke constructor with right type(s) of arguments, return null if constructor errors
        if (!RunMethod(obj, ".ctor", args...)) return nullptr;
        return reinterpret_cast<TObj*>(obj);
    }

    template<typename TObj = Il2CppObject, typename... TArgs>
    // Creates a new object of the given class using the given constructor parameters and casts it to TObj*
    // DOES NOT PERFORM ARGUMENT TYPE CHECKING! Uses the first .ctor with the right number of parameters it sees.
    TObj* NewUnsafe(Il2CppClass* klass, TArgs* ...args) {
        il2cpp_functions::Init();

        // object_new call
        auto obj = il2cpp_functions::object_new(klass);
        // runtime_invoke constructor with right number of args, return null if constructor errors
        if (!RunMethodUnsafe(obj, ".ctor", args...)) return nullptr;
        return reinterpret_cast<TObj*>(obj);
    }

    template<typename TObj = Il2CppObject, typename... TArgs>
    // Creates a new object of the class with the given nameSpace and className using the given constructor parameters and casts
    // it to TObj*. Will only run a .ctor whose parameter types match the given arguments.
    TObj* New(std::string_view nameSpace, std::string_view className, TArgs const& ...args) {
        auto* klass = RET_0_UNLESS(GetClassFromName(nameSpace, className));
        return New(klass, args...);
    }

    template<typename TObj = Il2CppObject, typename... TArgs>
    // Creates a new object of the class with the given nameSpace and className using the given constructor parameters and casts
    // it to TObj*. DOES NOT PERFORM ARGUMENT TYPE CHECKING! Uses the first .ctor with the right number of parameters it sees.
    TObj* NewUnsafe(std::string_view nameSpace, std::string_view className, TArgs* ...args) {
        auto* klass = RET_0_UNLESS(GetClassFromName(nameSpace, className));
        return NewUnsafe(klass, args...);
    }

    // Returns the FieldInfo for the field of the given class with the given name
    // Created by zoller27osu
    FieldInfo* FindField(Il2CppClass* klass, std::string_view fieldName);
    // Wrapper for FindField taking a namespace and class name in place of an Il2CppClass*
    template<class... TArgs>
    FieldInfo* FindField(std::string_view nameSpace, std::string_view className, TArgs&&... params) {
        return FindField(GetClassFromName(nameSpace, className), params...);
    }
    // Wrapper for FindField taking an instance to extract the Il2CppClass* from
    template<class T, class... TArgs>
    std::enable_if_t<!std::is_convertible_v<T, std::string_view>, FieldInfo*>
    FindField(T&& instance, TArgs&&... params) {
        il2cpp_functions::Init();

        auto* klass = RET_0_UNLESS(ExtractClass(instance));
        return FindField(klass, params...);
    }

    template<typename TOut = Il2CppObject*>
    // Gets a value from the given object instance, and FieldInfo, with return type TOut
    // Assumes a static field if instance == nullptr
    // Created by darknight1050, modified by Sc2ad and zoller27osu
    std::optional<TOut> GetFieldValue(Il2CppObject* instance, FieldInfo* field) {
        il2cpp_functions::Init();
        RET_NULLOPT_UNLESS(field);

        // Check that the TOut requested by the user matches the field.
        auto* outType = ExtractIndependentType<TOut>();
        if (outType && !IsConvertible(field->type, outType, false)) {
            Logger::get().warning("User requested TOut %s does not match the field's type, %s!",
                TypeGetSimpleName(outType), TypeGetSimpleName(field->type));
        }

        TOut out;
        if constexpr (std::is_convertible_v<TOut, Il2CppObject*>) {
            out = static_cast<TOut>(il2cpp_functions::field_get_value_object(field, instance));
        } else {
            if (instance) {
                il2cpp_functions::field_get_value(instance, field, &out);
            } else { // Fallback to perform a static field set
                il2cpp_functions::field_static_get_value(field, &out);
            }
        }
        return out;
    }

    template<class T>
    Il2CppObject* ExtractObject(T&& arg) {
        il2cpp_functions::Init();

        using Dt = std::decay_t<T>;
        if constexpr (std::is_same_v<Dt, Il2CppType*> || std::is_same_v<Dt, Il2CppClass*>) {
            return nullptr;
        } else if constexpr(std::is_convertible_v<Dt, Il2CppObject*>) {
            return arg;
        }

        auto* typ = RET_0_UNLESS(ExtractType(arg));
        auto* klass = RET_0_UNLESS(il2cpp_functions::class_from_il2cpp_type(typ));
        void* val = ExtractValue(arg);
        return il2cpp_functions::value_box(klass, val);
    }

    template<class T>
    bool UnextractObject(T& orig, Il2CppObject* modified) {
        il2cpp_functions::Init();
        if (!modified) return true;

        using Dt = std::decay_t<T>;
        if constexpr(std::is_convertible_v<Dt, Il2CppObject*>) {
            orig = modified;
        } else {
            void* val = RET_0_UNLESS(il2cpp_functions::object_unbox(modified));
            if constexpr (std::is_pointer_v<Dt>) {
                // TODO: would orig = static_cast<Dt>(val); work?
                *orig = *static_cast<Dt>(val);
            } else {
                orig = *static_cast<Dt*>(val);
            }
        }
        return true;
    }

    template<typename TOut = Il2CppObject*, typename T>
    // Gets the value of the field with type TOut and the given name from the given class
    // Adapted by zoller27osu
    std::optional<TOut> GetFieldValue(T&& classOrInstance, std::string_view fieldName) {
        auto* field = RET_NULLOPT_UNLESS(FindField(classOrInstance, fieldName));
        Il2CppObject* obj = ExtractObject(classOrInstance);  // null is allowed (for T = Il2CppType* or Il2CppClass*)
        return GetFieldValue<TOut>(obj, field);
    }

    template<typename TOut = Il2CppObject*>
    // Gets the value of the static field with the given name from the class with the given nameSpace and className.
    std::optional<TOut> GetFieldValue(std::string_view nameSpace, std::string_view className, std::string_view fieldName) {
        auto* klass = RET_NULLOPT_UNLESS(GetClassFromName(nameSpace, className));
        return GetFieldValue<TOut>(klass, fieldName);
    }

    // TODO: typecheck the Set[Field/Property]Value methods' value params?

    // Sets the value of a given field, given an object instance and the FieldInfo.
    // Returns false if it fails
    // Assumes static field if instance == nullptr
    template<class TArg>
    bool SetFieldValue(Il2CppObject* instance, FieldInfo* field, TArg&& value) {
        il2cpp_functions::Init();
        RET_0_UNLESS(field);

        // Ensure supplied value matches field's type
        auto* typ = ExtractType(value);
        RET_0_UNLESS(IsConvertible(field->type, typ));

        void* val = ExtractValue(value);
        if (instance) {
            il2cpp_functions::field_set_value(instance, field, val);
        } else { // Fallback to perform a static field set
            il2cpp_functions::field_static_set_value(field, val);
        }
        return true;
    }

    // Sets the value of a given field, given a class or instance and the field name.
    // Returns false if it fails
    template<class T, class TArg>
    bool SetFieldValue(T& classOrInstance, std::string_view fieldName, TArg&& value) {
        auto* field = RET_0_UNLESS(FindField(classOrInstance, fieldName));
        Il2CppObject* obj = ExtractObject(classOrInstance);  // null is allowed (for T = Il2CppType* or Il2CppClass*)
        RET_0_UNLESS(SetFieldValue(obj, field, value));
        RET_0_UNLESS(UnextractObject(classOrInstance, obj));
        return true;
    }

    // Sets the value of the static field with the given name on the class with the given nameSpace and className.
    // Returns false if it fails
    template<class TArg>
    bool SetFieldValue(std::string_view nameSpace, std::string_view className, std::string_view fieldName, TArg&& value) {
        auto* klass = RET_0_UNLESS(GetClassFromName(nameSpace, className));
        return SetFieldValue(klass, fieldName, value);
    }

    // Returns the PropertyInfo for the property of the given class with the given name
    // Created by zoller27osu
    const PropertyInfo* FindProperty(Il2CppClass* klass, std::string_view propertyName);
    // Wrapper for FindProperty taking a namespace and class name in place of an Il2CppClass*
    const PropertyInfo* FindProperty(std::string_view nameSpace, std::string_view className, std::string_view propertyName);
    // Wrapper for FindProperty taking an instance to extract the Il2CppClass* from
    template<class T>
    const PropertyInfo* FindProperty(T&& instance, std::string_view propertyName) {
        il2cpp_functions::Init();

        auto* klass = RET_0_UNLESS(ExtractClass(instance));
        return FindProperty(klass, propertyName);
    }

    template<class TOut = Il2CppObject*, class T>
    // Gets a value from the given object instance, and PropertyInfo, with return type TOut.
    // Assumes a static property if instance == nullptr
    std::optional<TOut> GetPropertyValue(T&& classOrInstance, const PropertyInfo* prop) {
        il2cpp_functions::Init();
        RET_NULLOPT_UNLESS(prop);

        auto* getter = RET_NULLOPT_UNLESS(il2cpp_functions::property_get_get_method(prop));
        return RunMethod<TOut>(classOrInstance, getter);
    }

    template<typename TOut = Il2CppObject*, typename T>
    // Gets the value of the property with the given name from the given class or instance, and returns it as TOut.
    std::optional<TOut> GetPropertyValue(T&& classOrInstance, std::string_view propName) {
        auto* prop = RET_NULLOPT_UNLESS(FindProperty(classOrInstance, propName));
        return GetPropertyValue<TOut>(classOrInstance, prop);
    }

    template<typename TOut = Il2CppObject*>
    // Gets the value of the static property with the given name from the class with the given nameSpace and className.
    std::optional<TOut> GetPropertyValue(std::string_view nameSpace, std::string_view className, std::string_view propName) {
        auto* klass = RET_0_UNLESS(GetClassFromName(nameSpace, className));
        return GetPropertyValue<TOut>(klass, propName);
    }

    // Sets the value of a given property, given an object instance or class and PropertyInfo
    // Returns false if it fails.
    // Only static properties work with classOrInstance == nullptr
    template<class T, class TArg>
    bool SetPropertyValue(T& classOrInstance, const PropertyInfo* prop, TArg&& value) {
        il2cpp_functions::Init();
        RET_0_UNLESS(prop);

        auto* setter = RET_0_UNLESS(il2cpp_functions::property_get_set_method(prop));
        return (bool)RunMethod(classOrInstance, setter, value);
    }

    // Sets the value of a given property, given an object instance or class and property name
    // Returns false if it fails
    template<class T, class TArg>
    bool SetPropertyValue(T& classOrInstance, std::string_view propName, TArg&& value) {
        auto* prop = RET_0_UNLESS(FindProperty(classOrInstance, propName));
        return SetPropertyValue(classOrInstance, prop, value);
    }

    // Sets the value of a given static property, given the class' namespace and name, and the property name and value.
    // Returns false if it fails
    template<class TArg>
    bool SetPropertyValue(std::string_view nameSpace, std::string_view className, std::string_view propName, TArg&& value) {
        auto* klass = RET_0_UNLESS(GetClassFromName(nameSpace, className));
        return SetPropertyValue(klass, propName, value);
    }

    template<typename T = MulticastDelegate, typename TObj, typename R, typename... TArgs>
    // Creates an Action of type actionType, with the given callback and callback self 'obj', and casts it to a T*
    // PLEASE!!! use the below FieldInfo or MethodInfo versions instead if you can.
    // Created by zoller27osu
    T* MakeAction(const Il2CppType* actionType, TObj* obj, function_ptr_t<R, TArgs...> callback) {
        Il2CppClass* actionClass = il2cpp_functions::class_from_il2cpp_type(actionType);

        /*
        * TODO: call PlatformInvoke::MarshalFunctionPointerToDelegate directly instead of copying code from it,
        * or at least use a cache like utils::NativeDelegateMethodCache::GetNativeDelegate(nativeFunctionPointer);
        */
        const MethodInfo* invoke = il2cpp_functions::class_get_method_from_name(actionClass, "Invoke", -1);  // well-formed Actions have only 1 invoke method
        auto* method = (MethodInfo*) calloc(1, sizeof(MethodInfo));
        method->methodPointer = (Il2CppMethodPointer)callback;
        method->invoker_method = NULL;
        method->parameters_count = invoke->parameters_count;
        method->slot = kInvalidIl2CppMethodSlot;
        method->is_marshaled_from_native = true;  // "a fake MethodInfo wrapping a native function pointer"
        // In the event that a function is static, this will behave as normal
        if (obj == nullptr) method->flags |= METHOD_ATTRIBUTE_STATIC;

        // TODO: figure out why passing method directly doesn't work
        auto* action = il2cpp_utils::NewUnsafe<T>(actionClass, obj, &method);
        auto* asDelegate = reinterpret_cast<Delegate*>(action);
        if (asDelegate->method_ptr != (void*)callback) {
            Logger::get().error("Created Action's method_ptr (%p) is incorrect (should be %p)!", asDelegate->method_ptr, callback);
            return nullptr;
        }
        return action;
    }

    template<typename T = MulticastDelegate, typename TObj>
    T* MakeAction(const Il2CppType* actionType, TObj* obj, void* callback) {
        auto tmp = reinterpret_cast<function_ptr_t<void>>(callback);
        return MakeAction(actionType, obj, tmp);
    }

    // Creates an Action fit to be passed in the given parameter position to the given method.
    template<typename T = MulticastDelegate, typename T1, typename T2>
    T* MakeAction(const MethodInfo* method, int paramIdx, T1&& arg1, T2&& arg2) {
        auto* actionType = RET_0_UNLESS(il2cpp_functions::method_get_param(method, paramIdx));
        return MakeAction<T, void>(actionType, arg1, arg2);
    }

    // Creates an Action fit to be assigned to the given field.
    template<typename T = MulticastDelegate, typename T1, typename T2>
    T* MakeAction(FieldInfo* field, T1&& arg1, T2&& arg2) {
        auto* actionType = RET_0_UNLESS(il2cpp_functions::field_get_type(field));
        return MakeAction<T, void>(actionType, arg1, arg2);
    }

    // Intializes an object (using the given args) fit to be passed to the given method at the given parameter index.
    template<typename... TArgs>
    Il2CppObject* CreateParam(const MethodInfo* method, int paramIdx, TArgs&& ...args) {
        auto* klass = RET_0_UNLESS(GetParamClass(method, paramIdx));
        return il2cpp_utils::New(klass, args...);
    }

    template<typename... TArgs>
    Il2CppObject* CreateParamUnsafe(const MethodInfo* method, int paramIdx, TArgs&& ...args) {
        auto* klass = RET_0_UNLESS(GetParamClass(method, paramIdx));
        return il2cpp_utils::NewUnsafe(klass, args...);
    }

    // Intializes an object (using the given args) fit to be assigned to the given field.
    template<typename... TArgs>
    Il2CppObject* CreateFieldValue(FieldInfo* field, TArgs&& ...args) {
        auto* klass = RET_0_UNLESS(GetFieldClass(field));
        return il2cpp_utils::New(klass, args...);
    }

    template<typename... TArgs>
    Il2CppObject* CreateFieldValueUnsafe(FieldInfo* field, TArgs&& ...args) {
        auto* klass = RET_0_UNLESS(GetFieldClass(field));
        return il2cpp_utils::NewUnsafe(klass, args...);
    }

    // Calls the System.RuntimeType.MakeGenericType(System.Type gt, System.Type[] types) function
    Il2CppReflectionType* MakeGenericType(Il2CppReflectionType* gt, Il2CppArray* types);

    // Function made by zoller27osu, modified by Sc2ad
    // PLEASE don't use, there are easier ways to get generics (see CreateParam, CreateFieldValue)
    Il2CppClass* MakeGeneric(const Il2CppClass* klass, std::vector<const Il2CppClass*> args);

    // Function made by zoller27osu, modified by Sc2ad
    // Logs information about the given MethodInfo* as log(DEBUG)
    void LogMethod(const MethodInfo* method);

    // Created by zoller27osu
    // Calls LogMethod on all methods in the given class
    void LogMethods(Il2CppClass* klass, bool logParents = false);

    // Created by zoller27osu
    // Logs information about the given FieldInfo* as log(DEBUG)
    void LogField(FieldInfo* field);

    // Created by zoller27osu
    // Calls LogField on all fields in the given class
    void LogFields(Il2CppClass* klass, bool logParents = false);

    // Created by zoller27osu
    // Logs information about the given PropertyInfo* as log(DEBUG)
    void LogProperty(const PropertyInfo* field);

    // Created by zoller27osu
    // Calls LogProperty on all properties in the given class
    void LogProperties(Il2CppClass* klass, bool logParents = false);

    // Some parts provided by zoller27osu
    // Logs information about the given Il2CppClass* as log(DEBUG)
    void LogClass(Il2CppClass* klass, bool logParents = false);

    // Logs all classes (from every namespace) that start with the given prefix
    // WARNING: THIS FUNCTION IS VERY SLOW. ONLY USE THIS FUNCTION ONCE AND WITH A FAIRLY SPECIFIC PREFIX!
    void LogClasses(std::string_view classPrefix, bool logParents = false);

    // Adds the given TypeDefinitionIndex to the class hash table of a given image
    // Mainly used in LogClasses
    void AddTypeToNametoClassHashTable(const Il2CppImage* img, TypeDefinitionIndex index);

    // Adds the given nested types of the namespaze, parentName, and klass to the hastable
    // Mainly used in AddTypeToNametoClassHashTable
    void AddNestedTypesToNametoClassHashTable(Il2CppNameToTypeDefinitionIndexHashTable* hashTable, const char *namespaze, const std::string& parentName, Il2CppClass *klass);

    // Adds the given nested types of typeDefinition to the class hash table of a given image
    // Mainly used in AddTypeToNametoClassHashTable
    void AddNestedTypesToNametoClassHashTable(const Il2CppImage* img, const Il2CppTypeDefinition* typeDefinition);

    // Creates a cs string (allocates it) with the given string_view and returns it
    // If pinned is false, will create a gchandle for the created string
    Il2CppString* createcsstr(std::string_view inp, bool pinned = false);

    // Returns if a given source object is an object of the given class
    // Created by zoller27osu
    [[nodiscard]] bool Match(const Il2CppObject* source, const Il2CppClass* klass) noexcept;

    // Asserts that a given source object is an object of the given class
    // Created by zoller27osu
    bool AssertMatch(const Il2CppObject* source, const Il2CppClass* klass);

    template<class To, class From>
    // Downcasts a class from From* to To*
    [[nodiscard]] auto down_cast(From* in) noexcept {
        static_assert(std::is_convertible_v<To*, From*>);
        return static_cast<To*>(in);
    }

    template<typename... TArgs>
    // Runtime Invoke, but with a list initializer for args
    Il2CppObject* RuntimeInvoke(const MethodInfo* method, Il2CppObject* reference, Il2CppException** exc, TArgs* ...args) {
        il2cpp_functions::Init();

        void* invokeParams[] = {reinterpret_cast<void*>(args)...};
        return il2cpp_functions::runtime_invoke(method, reference, invokeParams, exc);
    }
}
#endif /* IL2CPP_UTILS_H */
