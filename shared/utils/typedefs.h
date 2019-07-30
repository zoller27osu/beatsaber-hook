#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <type_traits>
#include <cassert>
#include "../libil2cpp/il2cpp-api-types.h"
#include "../libil2cpp/il2cpp-class-internals.h"
#include "../libil2cpp/il2cpp-object-internals.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
// C# SPECIFIC

typedef struct __Struct Struct;

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

#ifdef __cplusplus
}

template< class T >
struct is_value_type : std::integral_constant< 
    bool,
    std::is_arithmetic<T>::value || std::is_enum<T>::value ||
    std::is_pointer<T>::value ||
    std::is_base_of<Struct, T>::value
> {};

typedef struct __ArrayBounds
{
    int32_t length;
    int32_t lower_bound;
} ArrayBounds;

template<class T>
struct Array : public Il2CppObject
{
    static_assert(is_value_type<T>::value, "T must be a C# value type! (primitive, pointer or Struct)");
    /* bounds is NULL for szarrays */
    ArrayBounds *bounds;
    /* total number of elements of the array */
    int32_t max_length;
    T values[0];

    int32_t Length() {
        if (bounds) {
            return bounds->length;
        }
        return max_length;
    }
};
#endif /* __cplusplus */
#endif /* TYPEDEFS_H */
