#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <stdio.h>
#include <stdlib.h>
#include <type_traits>
// from https://gcc.gnu.org/bugzilla//show_bug.cgi?id=71579#c4, leading underscores removed
namespace std {
    template <class _Tp>
    struct is_complete_impl
    {
        template <class _Up, size_t = sizeof(_Up)>
        static true_type _S_test(int);

        template <class _Up>
        static false_type _S_test(...);

        using type = decltype(_S_test<_Tp>(0));
    };

    template<typename _Tp>
    using is_complete = typename is_complete_impl<_Tp>::type;

    // my own (trivial) addition
    template<typename _Tp>
    constexpr bool is_complete_v = is_complete<_Tp>::value;
}

#include <cassert>
#include "il2cpp-includes.h"

#ifdef __cplusplus
template<class T, class Enable = void>
struct is_value_type : std::integral_constant< 
    bool,
    (std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_pointer_v<T> || std::is_pod_v<T>) && !std::is_base_of_v<Il2CppObject, T>
> {};
template<class _T> constexpr bool is_value_type_v = is_value_type<_T>::value;

struct NamespaceAndNamePairHash
{
    size_t operator()(const std::pair<const char*, const char*>& pair) const
    {
        return il2cpp::utils::HashUtils::Combine(il2cpp::utils::StringUtils::Hash(pair.first), il2cpp::utils::StringUtils::Hash(pair.second));
    }
};

struct NamespaceAndNamePairEquals
{
    bool operator()(const std::pair<const char*, const char*>& p1, const std::pair<const char*, const char*>& p2) const
    {
        return !strcmp(p1.first, p2.first) && !strcmp(p1.second, p2.second);
    }
};

struct Il2CppNameToTypeDefinitionIndexHashTable : public Il2CppHashMap<std::pair<const char*, const char*>, TypeDefinitionIndex, NamespaceAndNamePairHash, NamespaceAndNamePairEquals>
{
    typedef Il2CppHashMap<std::pair<const char*, const char*>, TypeDefinitionIndex, NamespaceAndNamePairHash, NamespaceAndNamePairEquals> Base;
    Il2CppNameToTypeDefinitionIndexHashTable() : Base()
    {
    }
};

template<class T>
struct Array;

extern "C" {
#else
typedef struct Il2CppObject {
    void* vtable;
    void* monitor;
} Il2CppObject;
#endif /* !__cplusplus */

// C# SPECIFIC

// System.IntPtr
typedef struct IntPtr {
    void* value;
} IntPtr;

// System.DelegateData
typedef struct DelegateData : Il2CppObject {
    Il2CppReflectionType* target_type;
    Il2CppString* method_name;
    bool curied_first_arg;
} DelegateData;

// See il2cpp-object-internals.h/Il2CppDelegate
// System.Delegate
typedef struct Delegate : Il2CppObject
{
    Il2CppMethodPointer method_ptr; // 0x8
    InvokerMethod invoke_impl; // 0xC
    Il2CppObject* m_target; // 0x10
    IntPtr* method; // 0x14
    void* delegate_trampoline; // 0x18
    intptr_t extra_arg; // 0x1C

    /*
    * If non-NULL, this points to a memory location which stores the address of
    * the compiled code of the method, or NULL if it is not yet compiled.
    */
    uint8_t** method_code; // 0x20
    Il2CppReflectionMethod* method_info; // 0x24
    Il2CppReflectionMethod* original_method_info; // 0x28
    DelegateData* data; // 0x2C
    bool method_is_virtual; // 0x30
} Delegate;

// System.MulticastDelegate
typedef struct MulticastDelegate : Delegate
{
    ::Array<Delegate*>* delegates;
} MulticastDelegate;

// UNITY SPECIFIC

// UnityEngine.Color
typedef struct Color {
    float r;
    float g;
    float b;
    float a;
} Color;

// UnityEngine.Vector2
typedef struct Vector2 {
    float x;
    float y;
} Vector2;

// UnityEngine.Vector3
typedef struct Vector3 {
    float x;
    float y;
    float z;
} Vector3;

// UnityEngine.Vector4
typedef struct Vector4 {
    float x;
    float y;
    float z;
    float w;
} Vector4;

// UnityEngine.Quaternion
typedef struct Quaternion {
    float x;
    float y;
    float z;
    float w;
} Quaternion;

// UnityEngine.Rect
typedef struct Rect {
    float m_XMin;
    float m_YMin;
    float m_Width;
    float m_Height;
} Rect;

// UnityEngine.Scene
typedef struct Scene {
    int m_Handle;
} Scene;

#ifdef __cplusplus
}  /* extern "C" */
#endif /* __cplusplus */

#if __has_include("System/Array.hpp")
#define HAS_CODEGEN 1
#include "System/Array.hpp"
#include "System/Collections/Generic/IEnumerable_1.hpp"
#endif

// TODO: Move these to extern "C" region
template<class T>
#ifdef HAS_CODEGEN
struct Array : public Il2CppArray, public System::Collections::Generic::IEnumerable_1<T>
#else
struct Array : public Il2CppArray
#endif
{
    static_assert(is_value_type_v<T>, "T must be a C# value type! (primitive, pointer or Struct)");
    ALIGN_TYPE(8) T values[IL2CPP_ZERO_LEN_ARRAY];

    il2cpp_array_size_t Length() {
        if (bounds) {
            return bounds->length;
        }
        return max_length;
    }
};

// System.Collections.Generic.List
template<class T>
struct List : Il2CppObject
{
    Array<T>* items;
    int size;
    int version;
    Il2CppObject* syncRoot;
};
#endif /* TYPEDEFS_H */

#ifdef DEFINE_IL2CPP_ARG_TYPE
#ifndef IL2CPP_TYPEDEFS_DEFINES
#define IL2CPP_TYPEDEFS_DEFINES
namespace il2cpp_utils::il2cpp_type_check {
    DEFINE_IL2CPP_ARG_TYPE(Color, "UnityEngine", "Color");
    DEFINE_IL2CPP_ARG_TYPE(Vector2, "UnityEngine", "Vector2");
    DEFINE_IL2CPP_ARG_TYPE(Vector3, "UnityEngine", "Vector3");
    DEFINE_IL2CPP_ARG_TYPE(Vector4, "UnityEngine", "Vector4");
    DEFINE_IL2CPP_ARG_TYPE(Quaternion, "UnityEngine", "Quaternion");
    DEFINE_IL2CPP_ARG_TYPE(Rect, "UnityEngine", "Rect");
    DEFINE_IL2CPP_ARG_TYPE(Scene, "UnityEngine.SceneManagement", "Scene");

    template<typename TArg>
    struct il2cpp_no_arg_class<Array<TArg>*> {
        static inline Il2CppClass* get() {
            il2cpp_functions::Init();
            if constexpr (std::is_same_v<std::decay_t<TArg>, Il2CppObject*>) {
                il2cpp_functions::CheckS_GlobalMetadata();
                return il2cpp_functions::array_class_get(il2cpp_functions::defaults->object_class, 1);
            } else {
                Il2CppClass* eClass = RET_0_UNLESS(il2cpp_no_arg_class<TArg>::get());
                return il2cpp_functions::array_class_get(eClass, 1);
            }
        }
    };
}
#endif /* IL2CPP_TYPEDEFS_DEFINES */
#endif /* DEFINE_IL2CPP_ARG_TYPE */
