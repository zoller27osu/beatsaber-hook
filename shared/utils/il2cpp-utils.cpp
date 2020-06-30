#include <utility>  // for std::pair
#include "il2cpp-utils.hpp"
#include "utils.h"
#include "il2cpp-functions.hpp"
#include "alphanum.hpp"
#include <algorithm>
#include <map>
#include <unordered_set>
#include <unordered_map>

namespace std
{
    // From https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine
    template<class T> void hash_combine(size_t& seed, T v) {
        seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    // Let a "sequence" type be any type that supports .size() and iteration and whose elements are hashable and support !=.
    // Calculates a hash for a sequence.
    template<class T> std::size_t hash_seq(T const& seq) noexcept {
        std::size_t seed = seq.size();
        for(auto i: seq) {
            hash_combine(seed, i);
        }
        return seed;
    }

    // Specializes std::hash for std::vector
    template<class T> struct hash<std::vector<T>> {
        std::size_t operator()(std::vector<T> const& seq) const noexcept {
            return hash_seq(seq);
        }
    };
}

// Please see comments in il2cpp-utils.hpp
// TODO: Make this into a static class
namespace il2cpp_utils {
    // A hash function used to hash a pair of any kind
    struct hash_pair {
        template<class T1, class T2>
        size_t operator()(const std::pair<T1, T2>& p) const {
            auto hash1 = std::hash<T1>{}(p.first);
            auto hash2 = std::hash<T2>{}(p.second);
            return hash1 ^ hash2;
        }
    };
    // A hash function used to hash a pair of an object, pair
    struct hash_pair_3 {
        template<class T1, class T2, class T3>
        size_t operator()(const std::pair<T1, std::pair<T2, T3>>& p) const {
            auto hash1 = std::hash<T1>{}(p.first);
            auto hash2 = hash_pair{}(p.second);
            return hash1 ^ hash2;
        }
    };
    // It doesn't matter what types these are, they just need to be used correctly within the methods
    static std::unordered_map<std::pair<std::string, std::string>, Il2CppClass*, hash_pair> namesToClassesCache;
    // decltype is static so this should make the hashmap use as little memory as needed regardless of il2cpp changes
    static std::unordered_map<std::pair<Il2CppClass*, std::pair<std::string, decltype(MethodInfo::parameters_count)>>, const MethodInfo*, hash_pair_3> classesNamesToMethodsCache;

    typedef std::pair<std::string, std::vector<const Il2CppType*>> classesNamesTypesInnerPairType;
    static std::unordered_map<std::pair<Il2CppClass*, classesNamesTypesInnerPairType>, const MethodInfo*, hash_pair_3> classesNamesTypesToMethodsCache;
    static std::unordered_map<std::pair<Il2CppClass*, std::string>, FieldInfo*, hash_pair> classesNamesToFieldsCache;
    static std::unordered_map<std::pair<Il2CppClass*, std::string>, const PropertyInfo*, hash_pair> classesNamesToPropertiesCache;
    // Maximum length of characters of an exception message - 1
    #define EXCEPTION_MESSAGE_SIZE 4096

    // Returns a legible string from an Il2CppException*
    std::string ExceptionToString(Il2CppException* exp) {
        il2cpp_functions::Init();

        char msg[EXCEPTION_MESSAGE_SIZE];
        il2cpp_functions::format_exception(exp, msg, EXCEPTION_MESSAGE_SIZE);
        // auto exception_message = csstrtostr(exp->message);
        // return to_utf8(exception_message);
        return msg;
    }

    std::vector<const Il2CppType*> ClassVecToTypes(std::vector<const Il2CppClass*> seq) {
        il2cpp_functions::Init();

        std::vector<const Il2CppType*> types(seq.size());
        std::transform(seq.begin(), seq.end(), types.begin(), il2cpp_functions::class_get_type_const);
        return types;
    }

    const Il2CppType* MakeRef(const Il2CppType* type) {
        CRASH_UNLESS(type);
        if (type->byref) return type;
        // could use Class::GetByrefType instead of &->this_arg but it does the same thing
        return &il2cpp_functions::class_from_il2cpp_type(type)->this_arg;
    }

    const Il2CppType* UnRef(const Il2CppType* type) {
        if (!type->byref) return type;
        return il2cpp_functions::class_get_type(il2cpp_functions::class_from_il2cpp_type(type));
    }

    bool IsConvertible(const Il2CppType* to, const Il2CppType* from, bool asArgs) {
        if (asArgs) {
            if (to->byref) {
                if (!from->byref) {
                    log(DEBUG, "IsConvertible: to (%s, %p) is ref/out while from (%s, %p) is not. Not convertible.",
                        TypeGetSimpleName(to), to, TypeGetSimpleName(from), from);
                    return false;
                } else {
                    log(DEBUG, "IsConvertible: to (%s, %p) and from (%s, %p) are both ret/out. May be convertible.",
                        TypeGetSimpleName(to), to, TypeGetSimpleName(from), from);
                }
            }
        }
        auto classTo = il2cpp_functions::class_from_il2cpp_type(to);
        auto classFrom = il2cpp_functions::class_from_il2cpp_type(from);
        auto ret = il2cpp_functions::class_is_assignable_from(classTo, classFrom);
        log(DEBUG, "IsConvertible: class_is_assignable_from(%s, %s) returned %s",
            ClassStandardName(classTo).c_str(), ClassStandardName(classFrom).c_str(), ret ? "true" : "false");
        if (!ret && il2cpp_functions::class_is_enum(classTo)) {
            log(DEBUG, "IsConvertible: but classTo is enum! Comparing against class_enum_basetype.");
            ret = IsConvertible(il2cpp_functions::class_enum_basetype(classTo), from);
        }
        return ret;
    }

    bool ParameterMatch(const MethodInfo* method, const Il2CppType* const* type_arr, decltype(MethodInfo::parameters_count) count) {
        il2cpp_functions::Init();
        if (method->parameters_count != count) {
            return false;
        }
        // TODO: supply boolStrictMatch and use type_equals instead of IsConvertible if supplied?
        for (decltype(method->parameters_count) i = 0; i < method->parameters_count; i++) {
            // TODO: just because two parameter lists match doesn't necessarily mean this is the best match...
            if (!(IsConvertible(method->parameters[i].parameter_type, type_arr[i]))) {
                return false;
            }
        }
        return true;
    }

    static std::unordered_map<Il2CppClass*, const char*> typeMap;

    // TODO: somehow append "out/ref " to the front if type->byref? Make a TypeStandardName?
    const char* TypeGetSimpleName(const Il2CppType* type) {
        il2cpp_functions::Init();

        if (typeMap.empty()) {
            typeMap[il2cpp_functions::defaults->boolean_class] = "bool";
            typeMap[il2cpp_functions::defaults->byte_class] = "byte";
            typeMap[il2cpp_functions::defaults->sbyte_class] = "sbyte";
            typeMap[il2cpp_functions::defaults->char_class] = "char";
            typeMap[il2cpp_functions::defaults->single_class] = "float";
            typeMap[il2cpp_functions::defaults->double_class] = "double";
            typeMap[il2cpp_functions::defaults->int16_class] = "short";
            typeMap[il2cpp_functions::defaults->uint16_class] = "ushort";
            typeMap[il2cpp_functions::defaults->int32_class] = "int";
            typeMap[il2cpp_functions::defaults->uint32_class] = "uint";
            typeMap[il2cpp_functions::defaults->int64_class] = "long";
            typeMap[il2cpp_functions::defaults->uint64_class] = "ulong";
            typeMap[il2cpp_functions::defaults->object_class] = "object";
            typeMap[il2cpp_functions::defaults->string_class] = "string";
            typeMap[il2cpp_functions::defaults->void_class] = "void";
        }
        auto p = typeMap.find(il2cpp_functions::class_from_type(type));
        if (p != typeMap.end()) {
            return p->second;
        } else {
            return il2cpp_functions::type_get_name(type);
        }
    }

    bool IsInterface(const Il2CppClass* klass) {
        return (klass->flags & TYPE_ATTRIBUTE_INTERFACE) ||
            (klass->byval_arg.type == IL2CPP_TYPE_VAR) ||
            (klass->byval_arg.type == IL2CPP_TYPE_MVAR);
    }

    Il2CppClass* GetClassFromName(std::string_view name_space, std::string_view type_name) {
        il2cpp_functions::Init();

        // TODO: avoid creating std::string at any point except new pair insertion via P0919
        // Check cache
        auto key = std::pair<std::string, std::string>(name_space, type_name);
        auto itr = namesToClassesCache.find(key);
        if (itr != namesToClassesCache.end()) {
            return itr->second;
        }
        auto dom = RET_0_UNLESS(il2cpp_functions::domain_get());
        size_t assemb_count;
        const Il2CppAssembly** allAssemb = il2cpp_functions::domain_get_assemblies(dom, &assemb_count);

        for (int i = 0; i < assemb_count; i++) {
            auto assemb = allAssemb[i];
            auto img = il2cpp_functions::assembly_get_image(assemb);
            if (!img) {
                log(ERROR, "Assembly with name: %s has a null image!", assemb->aname.name);
                continue;
            }
            auto klass = il2cpp_functions::class_from_name(img, name_space.data(), type_name.data());
            if (klass) {
                namesToClassesCache.emplace(key, klass);
                return klass;
            }
        }
        log(ERROR, "il2cpp_utils: GetClassFromName: Could not find class with namepace: %s and name: %s",
            name_space.data(), type_name.data());
        return nullptr;
    }

    const MethodInfo* FindMethodUnsafe(Il2CppClass* klass, std::string_view methodName, int argsCount) {
        il2cpp_functions::Init();
        RET_0_UNLESS(klass);

        // Check Cache
        auto innerPair = std::pair<std::string, decltype(MethodInfo::parameters_count)>(methodName, argsCount);
        auto key = std::pair<Il2CppClass*, decltype(innerPair)>(klass, innerPair);
        auto itr = classesNamesToMethodsCache.find(key);
        if (itr != classesNamesToMethodsCache.end()) {
            return itr->second;
        }
        // Recurses through klass's parents
        auto methodInfo = il2cpp_functions::class_get_method_from_name(klass, methodName.data(), argsCount);
        if (!methodInfo) {
            log(ERROR, "could not find method %s with %i parameters in class '%s'!", methodName.data(), argsCount,
                ClassStandardName(klass).c_str());
            LogMethods(klass, true);
        }
        classesNamesToMethodsCache.emplace(key, methodInfo);
        return methodInfo;
    }

    const MethodInfo* FindMethodUnsafe(std::string_view nameSpace, std::string_view className, std::string_view methodName, int argsCount) {
        return FindMethodUnsafe(GetClassFromName(nameSpace, className), methodName, argsCount);
    }

    const MethodInfo* FindMethodUnsafe(Il2CppObject* instance, std::string_view methodName, int argsCount) {
        auto klass = RET_0_UNLESS(il2cpp_functions::object_get_class(instance));
        return FindMethodUnsafe(klass, methodName, argsCount);
    }

    const MethodInfo* FindMethod(Il2CppClass* klass, std::string_view methodName, std::vector<const Il2CppType*> argTypes) {
        il2cpp_functions::Init();
        RET_0_UNLESS(klass);

        // Check Cache
        auto innerPair = classesNamesTypesInnerPairType(methodName, argTypes);
        auto key = std::pair<Il2CppClass*, classesNamesTypesInnerPairType>(klass, innerPair);
        auto itr = classesNamesTypesToMethodsCache.find(key);
        if (itr != classesNamesTypesToMethodsCache.end()) {
            return itr->second;
        }

        void* myIter = nullptr;
        const MethodInfo* methodInfo = nullptr;
        bool multipleMatches = false;
        // Does NOT automatically recurse through klass's parents
        while (const MethodInfo* current = il2cpp_functions::class_get_methods(klass, &myIter)) {
            if ((methodName == current->name) && ParameterMatch(current, argTypes)) {
                if (methodInfo) {
                    multipleMatches = true;
                    break;
                }
                methodInfo = current;
            }
        }
        if (!methodInfo && klass->parent && klass->parent != klass) {
            methodInfo = FindMethod(klass->parent, methodName, argTypes);
        }

        if (!methodInfo || multipleMatches) {
            std::stringstream ss;
            ss << ((multipleMatches) ? "found multiple matches for" : "could not find");
            ss << " method " << methodName << "(";
            bool first = true;
            for (auto t : argTypes) {
                if (!first) ss << ", ";
                first = false;
                ss << TypeGetSimpleName(t);
            }
            ss << ") in class '" << ClassStandardName(klass) << "'!";
            log(ERROR, "%s", ss.str().c_str());
            LogMethods(klass);
            if (multipleMatches) methodInfo = nullptr;
        }
        classesNamesTypesToMethodsCache.emplace(key, methodInfo);
        return methodInfo;
    }

    const MethodInfo* FindMethod(Il2CppClass* klass, std::string_view methodName, std::vector<const Il2CppClass*> classFromes) {
        std::vector<const Il2CppType*> argTypes = ClassVecToTypes(classFromes);
        return FindMethod(klass, methodName, argTypes);
    }

    const MethodInfo* FindMethod(Il2CppClass* klass, std::string_view methodName, std::vector<std::string_view> argSpaceClass) {
        std::vector<const Il2CppType*> argTypes;
        for (int i = 0; i < argSpaceClass.size() - 1; i += 2) {
            auto clazz = GetClassFromName(argSpaceClass[i].data(), argSpaceClass[i+1].data());
            argTypes.push_back(il2cpp_functions::class_get_type(clazz));
        }
        return FindMethod(klass, methodName, argTypes);
    }

    FieldInfo* FindField(Il2CppClass* klass, std::string_view fieldName) {
        il2cpp_functions::Init();
        RET_0_UNLESS(klass);

        // Check Cache
        auto key = std::pair<Il2CppClass*, std::string>(klass, fieldName);
        auto itr = classesNamesToFieldsCache.find(key);
        if (itr != classesNamesToFieldsCache.end()) {
            return itr->second;
        }
        auto field = il2cpp_functions::class_get_field_from_name(klass, fieldName.data());
        if (!field) {
            log(ERROR, "could not find field %s in class '%s'!", fieldName.data(), ClassStandardName(klass).c_str());
            LogFields(klass);
            if (klass->parent != klass) field = FindField(klass->parent, fieldName);
        }
        classesNamesToFieldsCache.emplace(key, field);
        return field;
    }

    const PropertyInfo* FindProperty(Il2CppClass* klass, std::string_view propName) {
        il2cpp_functions::Init();
        RET_0_UNLESS(klass);

        // Check Cache
        auto key = std::pair<Il2CppClass*, std::string>(klass, propName);
        auto itr = classesNamesToPropertiesCache.find(key);
        if (itr != classesNamesToPropertiesCache.end()) {
            return itr->second;
        }
        auto prop = il2cpp_functions::class_get_property_from_name(klass, propName.data());
        if (!prop) {
            log(ERROR, "could not find property %s in class '%s'!", propName.data(), ClassStandardName(klass).c_str());
            LogProperties(klass);
            if (klass->parent != klass) prop = FindProperty(klass->parent, propName);
        }
        classesNamesToPropertiesCache.emplace(key, prop);
        return prop;
    }

    const PropertyInfo* FindProperty(std::string_view nameSpace, std::string_view className, std::string_view propertyName) {
        return FindProperty(GetClassFromName(nameSpace, className), propertyName);
    }

    Il2CppClass* GetParamClass(const MethodInfo* method, int paramIdx) {
        auto type = RET_0_UNLESS(il2cpp_functions::method_get_param(method, paramIdx));
        return il2cpp_functions::class_from_il2cpp_type(type);
    }

    Il2CppClass* GetFieldClass(FieldInfo* field) {
        auto type = RET_0_UNLESS(il2cpp_functions::field_get_type(field));
        return il2cpp_functions::class_from_il2cpp_type(type);
    }

    Il2CppReflectionType* MakeGenericType(Il2CppReflectionType* gt, Il2CppArray* types) {
        il2cpp_functions::Init();

        auto runtimeType = RET_0_UNLESS(il2cpp_functions::defaults->runtimetype_class);
        auto makeGenericMethod = RET_0_UNLESS(FindMethodUnsafe(runtimeType, "MakeGenericType", 2));

        Il2CppException* exp = nullptr;
        void* params[] = {reinterpret_cast<void*>(gt), reinterpret_cast<void*>(types)};
        auto genericType = il2cpp_functions::runtime_invoke(makeGenericMethod, nullptr, params, &exp);
        if (exp) {
            log(ERROR, "il2cpp_utils: MakeGenericType: Failed with exception: %s", ExceptionToString(exp).c_str());
            return nullptr;
        }
        return reinterpret_cast<Il2CppReflectionType*>(genericType);
    }

    Il2CppClass* MakeGeneric(const Il2CppClass* klass, std::vector<const Il2CppClass*> args) {
        il2cpp_functions::Init();

        auto typ = RET_0_UNLESS(il2cpp_functions::defaults->systemtype_class);
        auto klassType = RET_0_UNLESS(GetSystemType(klass));

        // Call Type.MakeGenericType on it
        auto arr = il2cpp_functions::array_new_specific(typ, args.size());
        if (!arr) {
            log(ERROR, "il2cpp_utils: MakeGeneric: Failed to make new array with length: %zu", args.size());
            return nullptr;
        }

        int i = 0;
        for (auto arg : args) {
            auto* o = GetSystemType(arg);
            if (!o) {
                log(ERROR, "il2cpp_utils: MakeGeneric: Failed to get type for %s", il2cpp_functions::class_get_name_const(arg));
                return nullptr;
            }
            il2cpp_array_set(arr, void*, i, reinterpret_cast<void*>(o));
            i++;
        }

        auto* reflection_type = RET_0_UNLESS(MakeGenericType(reinterpret_cast<Il2CppReflectionType*>(klassType), arr));
        auto* ret = RET_0_UNLESS(il2cpp_functions::class_from_system_type(reflection_type));
        log(DEBUG, "il2cpp_utils: MakeGeneric: returning '%s'", ClassStandardName(ret).c_str());
        return ret;
    }

    Il2CppClass* MakeGeneric(const Il2CppClass* klass, const Il2CppType** types, uint32_t numTypes) {
        il2cpp_functions::Init();

        auto typ = RET_0_UNLESS(il2cpp_functions::defaults->systemtype_class);
        auto klassType = RET_0_UNLESS(GetSystemType(klass));

        // Call Type.MakeGenericType on it
        auto arr = il2cpp_functions::array_new_specific(typ, numTypes);
        if (!arr) {
            log(ERROR, "il2cpp_utils: MakeGeneric: Failed to make new array with length: %u", numTypes);
            return nullptr;
        }

        int i = 0;
        for (int i = 0; i < numTypes; i++) {
            const Il2CppType* arg = types[i];
            auto* o = GetSystemType(arg);
            if (!o) {
                log(ERROR, "il2cpp_utils: MakeGeneric: Failed to get system type for %s", il2cpp_functions::type_get_name(arg));
                return nullptr;
            }
            il2cpp_array_set(arr, void*, i, reinterpret_cast<void*>(o));
        }

        auto* reflection_type = RET_0_UNLESS(MakeGenericType(reinterpret_cast<Il2CppReflectionType*>(klassType), arr));
        auto* ret = RET_0_UNLESS(il2cpp_functions::class_from_system_type(reflection_type));
        log(DEBUG, "il2cpp_utils: MakeGeneric: returning '%s'", ClassStandardName(ret).c_str());
        return ret;
    }

    Il2CppReflectionType* GetSystemType(const Il2CppType* typ) {
        return reinterpret_cast<Il2CppReflectionType*>(il2cpp_functions::type_get_object(typ));
    }

    Il2CppReflectionType* GetSystemType(const Il2CppClass* klass) {
        il2cpp_functions::Init();
        RET_0_UNLESS(klass);

        auto* typ = il2cpp_functions::class_get_type_const(klass);
        return GetSystemType(typ);
    }

    Il2CppReflectionType* GetSystemType(std::string_view nameSpace, std::string_view className) {
        return GetSystemType(il2cpp_utils::GetClassFromName(nameSpace, className));
    }

    void LogMethod(const MethodInfo* method) {
        il2cpp_functions::Init();
        RET_V_UNLESS(method);

        auto flags = il2cpp_functions::method_get_flags(method, nullptr);
        std::stringstream flagStream;
        if (flags & METHOD_ATTRIBUTE_STATIC) flagStream << "static ";
        if (flags & METHOD_ATTRIBUTE_VIRTUAL) flagStream << "virtual ";
        if (flags & METHOD_ATTRIBUTE_ABSTRACT) flagStream << "abstract ";
        const auto& flagStrRef = flagStream.str();
        const char* flagStr = flagStrRef.c_str();
        auto* retType = il2cpp_functions::method_get_return_type(method);
        auto retTypeStr = TypeGetSimpleName(retType);
        auto methodName = il2cpp_functions::method_get_name(method);
        methodName = methodName ? methodName : "__noname__";
        std::stringstream paramStream;
        for (int i = 0; i < il2cpp_functions::method_get_param_count(method); i++) {
            if (i > 0) paramStream << ", ";
            auto* argType = il2cpp_functions::method_get_param(method, i);
            if (il2cpp_functions::type_is_byref(argType)) {
                paramStream << "out/ref ";
            }
            paramStream << TypeGetSimpleName(argType) << " ";
            auto name = il2cpp_functions::method_get_param_name(method, i);
            paramStream << (name ? name : "__noname__");
        }
        const auto& paramStrRef = paramStream.str();
        const char* paramStr = paramStrRef.c_str();
        log(DEBUG, "%s%s %s(%s);", flagStr, retTypeStr, methodName, paramStr);
    }

    void LogField(FieldInfo* field) {
        il2cpp_functions::Init();
        RET_V_UNLESS(field);

        auto flags = il2cpp_functions::field_get_flags(field);
        const char* flagStr = (flags & FIELD_ATTRIBUTE_STATIC) ? "static " : "";
        auto* type = il2cpp_functions::field_get_type(field);
        auto typeStr = TypeGetSimpleName(type);
        auto name = il2cpp_functions::field_get_name(field);
        name = name ? name : "__noname__";
        auto offset = il2cpp_functions::field_get_offset(field);

        log(DEBUG, "%s%s %s; // 0x%lx, flags: 0x%.4X", flagStr, typeStr, name, offset, flags);
    }

    void LogFields(Il2CppClass* klass, bool logParents) {
        il2cpp_functions::Init();
        RET_V_UNLESS(klass);

        void* myIter = nullptr;
        FieldInfo* field;
        if (klass->name) il2cpp_functions::Class_Init(klass);
        if (logParents) log(INFO, "class name: %s", ClassStandardName(klass).c_str());

        log(DEBUG, "field_count: %i", klass->field_count);
        while ((field = il2cpp_functions::class_get_fields(klass, &myIter))) {
            LogField(field);
        }
        usleep(100);
        if (logParents && klass->parent && klass->parent != klass) {
            LogFields(klass->parent, logParents);
        }
    }

    void LogProperty(const PropertyInfo* prop) {
        il2cpp_functions::Init();
        RET_V_UNLESS(prop);

        auto flags = il2cpp_functions::property_get_flags(prop);
        const char* flagStr = (flags & FIELD_ATTRIBUTE_STATIC) ? "static " : "";
        auto name = il2cpp_functions::property_get_name(prop);
        name = name ? name : "__noname__";
        auto* getter = il2cpp_functions::property_get_get_method(prop);
        auto getterName = getter ? il2cpp_functions::method_get_name(getter) : "";
        auto* setter = il2cpp_functions::property_get_set_method(prop);
        auto setterName = setter ? il2cpp_functions::method_get_name(setter) : "";
        const Il2CppType* type = nullptr;
        if (getter) {
            type = il2cpp_functions::method_get_return_type(getter);
        } else if (setter) {
            type = il2cpp_functions::method_get_param(setter, 0);
        }
        auto typeStr = type ? TypeGetSimpleName(type) : "?type?";

        log(DEBUG, "%s%s %s { %s; %s; }; // flags: 0x%.4X", flagStr, typeStr, name, getterName, setterName, flags);
    }

    void LogProperties(Il2CppClass* klass, bool logParents) {
        il2cpp_functions::Init();
        RET_V_UNLESS(klass);

        void* myIter = nullptr;
        const PropertyInfo* prop;
        if (klass->name) il2cpp_functions::Class_Init(klass);
        if (logParents) log(INFO, "class name: %s", ClassStandardName(klass).c_str());

        log(DEBUG, "property_count: %i", klass->property_count);
        while ((prop = il2cpp_functions::class_get_properties(klass, &myIter))) {
            LogProperty(prop);
        }
        usleep(100);
        if (logParents && klass->parent && klass->parent != klass) {
            LogProperties(klass->parent, logParents);
        }
    }

    void GenericsToStringHelper(Il2CppGenericClass* genClass, std::ostream& os) {
        auto genContext = &genClass->context;
        auto* genInst = genContext->class_inst;
        if (!genInst) {
            genInst = genContext->method_inst;
            if (genInst) log(WARNING, "Missing class_inst! Trying method_inst?");
        }
        if (genInst) {
            os << "<";
            for (int i = 0; i < genInst->type_argc; i++) {
                auto typ = genInst->type_argv[i];
                if (i > 0) os << ", ";
                const char* typName = TypeGetSimpleName(typ);
                os << typName;
            }
            os << ">";
        } else {
            log(WARNING, "context->class_inst missing for genClass!");
        }
    }

    std::string ClassStandardName(Il2CppClass* klass, bool generics) {
        std::stringstream ss;
        const char* namespaze = il2cpp_functions::class_get_namespace(klass);
        auto* declaring = il2cpp_functions::class_get_declaring_type(klass);
        bool hasNamespace = (namespaze && namespaze[0] != '\0');
        if (!hasNamespace && declaring) {
            ss << ClassStandardName(declaring) << "/";
        } else {
            ss << namespaze << "::";
        }
        ss << il2cpp_functions::class_get_name(klass);

        if (generics) {
            il2cpp_functions::class_is_generic(klass);
            auto* genClass = klass->generic_class;
            if (genClass) {
                GenericsToStringHelper(genClass, ss);
            }
        }
        return ss.str();
    }

    std::string GenericClassStandardName(Il2CppGenericClass* genClass) {
        if (genClass->cached_class) {
            return ClassStandardName(genClass->cached_class);
        }
        if (genClass->typeDefinitionIndex != kTypeDefinitionIndexInvalid) {
            auto* klass = il2cpp_functions::GenericClass_GetClass(genClass);
            return ClassStandardName(klass);
        }
        return "?";
    }

    void LogMethods(Il2CppClass* klass, bool logParents) {
        RET_V_UNLESS(klass);

        if (klass->name) il2cpp_functions::Class_Init(klass);
        if (klass->method_count && !(klass->methods)) {
            log(WARNING, "Class is valid and claims to have methods but ->methods is null! class name: %s", ClassStandardName(klass).c_str());
            return;
        }
        if (logParents) log(INFO, "class name: %s", ClassStandardName(klass).c_str());

        log(DEBUG, "method_count: %i", klass->method_count);
        for (int i = 0; i < klass->method_count; i++) {
            if (klass->methods[i]) {
                log(DEBUG, "Method %i:", i);
                LogMethod(klass->methods[i]);
            } else {
                log(WARNING, "Method: %i Does not exist!", i);
            }
        }
        usleep(100);  // 0.0001s
        if (logParents && klass->parent && (klass->parent != klass)) {
            LogMethods(klass->parent, logParents);
        }
    }

    static int indent = -1;
    static int maxIndent;
    std::unordered_set<Il2CppClass*> loggedClasses;
    void LogClass(Il2CppClass* klass, bool logParents) {
        il2cpp_functions::Init();
        RET_V_UNLESS(klass);

        if (loggedClasses.count(klass)) {
            log(DEBUG, "Already logged %p!", klass);
            return;
        }
        loggedClasses.insert(klass);

        RET_V_UNLESS(klass->klass == klass);  // otherwise, klass is likely NOT an Il2CppClass*!
        RET_V_UNLESS(klass->name);  // ditto

        indent++;
        // Note: il2cpp stops at GenericMetadata::MaximumRuntimeGenericDepth (which is 8)
        maxIndent = std::max(indent, maxIndent);

        bool methodInit = false;
        if (klass->name) {
            // Note: unless vm/Class.cpp is wrong, Class::Init always returns true
            il2cpp_functions::Class_Init(klass);
            if (klass->initialized_and_no_error) {
                methodInit = true;
            }
        }

        log(DEBUG, "%i ======================CLASS INFO FOR CLASS: %s======================", indent, ClassStandardName(klass).c_str());
        void* myIter = nullptr;
        if (!methodInit) {
            // log results of Class::Init
            log(WARNING, "klass->initialized: %i, init_pending: %i, has_initialization_error: %i, initializationExceptionGCHandle: %Xll",
                    klass->initialized, klass->init_pending, klass->has_initialization_error, klass->initializationExceptionGCHandle);
            auto* m1 = il2cpp_functions::class_get_methods(klass, &myIter);  // attempt again to initialize the method data
            if (klass->method_count && !klass->methods) {
                log(ERROR, "Class::Init and class_get_methods failed to initialize klass->methods! class_get_methods returned: %p",
                    m1);
                if (m1) LogMethod(m1);
            }
        }

        log(DEBUG, "Pointer: %p", klass);
        log(DEBUG, "Type Token: %i", il2cpp_functions::class_get_type_token(klass));
        auto typeDefIdx = klass->generic_class ? klass->generic_class->typeDefinitionIndex : il2cpp_functions::MetadataCache_GetIndexForTypeDefinition(klass);
        log(DEBUG, "TypeDefinitionIndex: %i", typeDefIdx);
        // Repair the typeDefinition value if it was null but we found one
        if (!klass->typeDefinition && typeDefIdx > 0) klass->typeDefinition = il2cpp_functions::MetadataCache_GetTypeDefinitionFromIndex(typeDefIdx);
        log(DEBUG, "Type definition: %p", klass->typeDefinition);

        log(DEBUG, "Assembly Name: %s", il2cpp_functions::class_get_assemblyname(klass));

        auto* typ = il2cpp_functions::class_get_type(klass);
        if (typ) {
            log(DEBUG, "Type name: %s", il2cpp_functions::type_get_name(typ));
            if (auto* reflName = il2cpp_functions::Type_GetName(typ, IL2CPP_TYPE_NAME_FORMAT_REFLECTION)) {
                log(DEBUG, "Type reflection name: %s", reflName);
                il2cpp_functions::free(reflName);
            }
            log(DEBUG, "Fully qualifed type name: %s", il2cpp_functions::type_get_assembly_qualified_name(typ));
        }
        log(DEBUG, "Rank: %i", il2cpp_functions::class_get_rank(klass));
        log(DEBUG, "Flags: 0x%.8X", il2cpp_functions::class_get_flags(klass));
        log(DEBUG, "Event Count: %i", klass->event_count);
        log(DEBUG, "Method Count: %i", klass->method_count);
        log(DEBUG, "Is Generic: %i", il2cpp_functions::class_is_generic(klass));
        log(DEBUG, "Is Abstract: %i", il2cpp_functions::class_is_abstract(klass));

        // Some methods, such as GenericClass::GetClass, may not initialize all fields in Il2CppClass, and thus not meet all implicit contracts defined by the comments in Il2CppClass's struct definition.
        // But unless we're blind, the only method that sets is_generic on non-methods is MetadataCache::FromTypeDefinition. That method also contains the only assignment of genericContainerIndex.
        // Therefore, this code makes only the following assumptions:
        // 1. If is_generic is set, then genericContainerIndex was also intentionally set (even if it's 0) and is not -1 (invalid)
        // 2. Even if is_generic wasn't set, a positive genericContainerIndex was intentionally set that way and is a valid index.
        if (klass->is_generic || klass->genericContainerIndex > 0) {
            auto* genContainer = il2cpp_functions::MetadataCache_GetGenericContainerFromIndex(klass->genericContainerIndex);
            log(DEBUG, "genContainer: idx %i, ownerIndex: %i, is_method: %i", klass->genericContainerIndex, genContainer->ownerIndex, genContainer->is_method);
            if (genContainer->ownerIndex != typeDefIdx) {
                log(ERROR, "genContainer ownerIndex mismatch!");
            }
            for (int i = 0; i < genContainer->type_argc; i++) {
                auto genParamIdx = genContainer->genericParameterStart + i;
                auto* genParam = il2cpp_functions::MetadataCache_GetGenericParameterFromIndex(genParamIdx);
                if (genParam) {
                    log(DEBUG, "genParam #%i, idx %i: ownerIdx %i, name %s, num %i, flags (see "
                        "IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_X in il2cpp-tabledefs.h) 0x%.2X", i, genParamIdx, genParam->ownerIndex,
                        il2cpp_functions::MetadataCache_GetStringFromIndex(genParam->nameIndex), genParam->num, genParam->flags);
                } else {
                    log(WARNING, "genParam %i, idx %i: null", i, genParamIdx);
                }
            }
        } else {
            log(DEBUG, "genericContainerIndex: %i", klass->genericContainerIndex);
        }

        auto* typDef = klass->typeDefinition;

        log(DEBUG, "%i =========METHODS=========", indent);
        LogMethods(klass);
        log(DEBUG, "%i =======END METHODS=======", indent);

        auto* declaring = il2cpp_functions::class_get_declaring_type(klass);
        log(DEBUG, "declaring type: %p (%s)", declaring, declaring ? ClassStandardName(declaring).c_str() : "");
        if (declaring && logParents) LogClass(declaring, logParents);
        auto* element = il2cpp_functions::class_get_element_class(klass);
        log(DEBUG, "element class: %p ('%s', self = %p)", element, element ? ClassStandardName(element).c_str() : "", klass);
        if (element && element != klass && logParents) LogClass(element, logParents);

        log(DEBUG, "%i =======PROPERTIES=======", indent);
        LogProperties(klass);
        log(DEBUG, "%i =====END PROPERTIES=====", indent);
        log(DEBUG, "%i =========FIELDS=========", indent);
        LogFields(klass);
        log(DEBUG, "%i =======END FIELDS=======", indent);

        auto* parent = il2cpp_functions::class_get_parent(klass);
        log(DEBUG, "parent: %p (%s)", parent, parent ? ClassStandardName(parent).c_str() : "");
        if (parent && logParents) LogClass(parent, logParents);
        log(DEBUG, "%i, ==================================================================================", indent);
        indent--;
    }

    static std::unordered_map<Il2CppClass*, std::map<std::string, Il2CppGenericClass*, doj::alphanum_less<std::string>>> classToGenericClassMap;
    void BuildGenericsMap() {
        auto* metadataReg = RET_V_UNLESS(*il2cpp_functions::s_Il2CppMetadataRegistrationPtr);
        log(DEBUG, "metadataReg: %p, offset = %lX", metadataReg, ((intptr_t)metadataReg) - getRealOffset(0));

        int uncached_class_count = 0;
        for (int i=0; i < metadataReg->genericClassesCount; i++) {
            Il2CppGenericClass* genClass = metadataReg->genericClasses[i];
            if (!genClass) continue;
            if (!(genClass->cached_class)) {
                uncached_class_count++;
            }
            std::string genClassName = GenericClassStandardName(genClass);

            auto* typeDefClass = il2cpp_functions::MetadataCache_GetTypeInfoFromTypeDefinitionIndex(genClass->typeDefinitionIndex);
            if (!typeDefClass) continue;

            classToGenericClassMap[typeDefClass][genClassName.c_str()] = genClass;
        }
        log(DEBUG, "uncached_class_count: %i (%f proportion of total)", uncached_class_count, uncached_class_count * 1.0 / metadataReg->genericClassesCount);
    }

    void LogClasses(std::string_view classPrefix, bool logParents) {
        il2cpp_functions::Init();
        BuildGenericsMap();

        // Begin prefix matching
        std::map<std::string, Il2CppClass*, doj::alphanum_less<std::string>> matches;
        // Get il2cpp domain
        auto* dom = il2cpp_functions::domain_get();
        // Get all il2cpp assemblies
        size_t size;
        auto** assembs = il2cpp_functions::domain_get_assemblies(dom, &size);

        for (size_t i = 0; i < size; ++i) {
            // Get image for each assembly
            if (!assembs[i]) {
                log(WARNING, "Assembly %zu was null! Skipping.", i);
                continue;
            }
            log(DEBUG, "Scanning assembly \"%s\"", assembs[i]->aname.name);
            auto* img = il2cpp_functions::assembly_get_image(assembs[i]);
            if (!img) {
                log(WARNING, "Assembly's image was null! Skipping.");
                continue;
            }

            if (img->nameToClassHashTable == nullptr) {
                log(DEBUG, "Assembly's nameToClassHashTable is empty. Populating it instead.");

                img->nameToClassHashTable = new Il2CppNameToTypeDefinitionIndexHashTable();
                for (uint32_t index = 0; index < img->typeCount; index++) {
                    TypeDefinitionIndex typeIndex = img->typeStart + index;
                    AddTypeToNametoClassHashTable(img, typeIndex);
                }

                for (uint32_t index = 0; index < img->exportedTypeCount; index++) {
                    auto typeIndex = il2cpp_functions::MetadataCache_GetExportedTypeFromIndex(img->exportedTypeStart + index);
                    if (typeIndex != kTypeIndexInvalid)
                        AddTypeToNametoClassHashTable(img, typeIndex);
                }
            }

            auto length = img->nameToClassHashTable->size();
            for (auto itr = img->nameToClassHashTable->begin(); itr != img->nameToClassHashTable->end(); ++itr) {
                // ->first is a KeyWrapper(pair(namespaceName, className))
                // ->second is TypeDefinitionIndex
                if (strncmp(classPrefix.data(), itr->first.key.second, classPrefix.length()) == 0) {
                    // Starts with!
                    // Convert TypeDefinitionIndex --> class
                    auto klazz = il2cpp_functions::MetadataCache_GetTypeInfoFromTypeDefinitionIndex(itr->second);
                    matches[ClassStandardName(klazz)] = klazz;
                }
            }
        }

        usleep(1000);  // 0.001s
        log(DEBUG, "LogClasses:");
        for ( const auto &pair : matches ) {
            LogClass(pair.second, logParents);
            indent = -1;
            for ( const auto &genPair : classToGenericClassMap[pair.second] ) {
                log(DEBUG, "%s", genPair.first.c_str());
            }
            usleep(1000);  // 0.001s
        }
        log(DEBUG, "LogClasses(\"%s\") is complete.", classPrefix.data());
        log(DEBUG, "maxIndent: %i", maxIndent);
    }

    void AddTypeToNametoClassHashTable(const Il2CppImage* img, TypeDefinitionIndex index) {
        const Il2CppTypeDefinition* typeDefinition = il2cpp_functions::MetadataCache_GetTypeDefinitionFromIndex(index);
        // don't add nested types
        if (typeDefinition->declaringTypeIndex != kTypeIndexInvalid)
            return;

        if (img != il2cpp_functions::get_corlib())
            AddNestedTypesToNametoClassHashTable(img, typeDefinition);

        img->nameToClassHashTable->insert(std::make_pair(std::make_pair(il2cpp_functions::MetadataCache_GetStringFromIndex(typeDefinition->namespaceIndex), il2cpp_functions::MetadataCache_GetStringFromIndex(typeDefinition->nameIndex)), index));
    }

    void AddNestedTypesToNametoClassHashTable(const Il2CppImage* img, const Il2CppTypeDefinition* typeDefinition) {
        for (int i = 0; i < typeDefinition->nested_type_count; ++i) {
            Il2CppClass *klass = il2cpp_functions::MetadataCache_GetNestedTypeFromIndex(typeDefinition->nestedTypesStart + i);
            AddNestedTypesToNametoClassHashTable(img->nameToClassHashTable, il2cpp_functions::MetadataCache_GetStringFromIndex(typeDefinition->namespaceIndex), il2cpp_functions::MetadataCache_GetStringFromIndex(typeDefinition->nameIndex), klass);
        }
    }

    void AddNestedTypesToNametoClassHashTable(Il2CppNameToTypeDefinitionIndexHashTable* hashTable, const char *namespaze, const std::string& parentName, Il2CppClass *klass) {
        std::string name = parentName + "/" + klass->name;
        char *pName = (char*)calloc(name.size() + 1, sizeof(char));
        strcpy(pName, name.c_str());

        hashTable->insert(std::make_pair(std::make_pair(namespaze, (const char*)pName), il2cpp_functions::MetadataCache_GetIndexForTypeDefinition(klass)));

        void *iter = NULL;
        while (Il2CppClass *nestedClass = il2cpp_functions::class_get_nested_types(klass, &iter))
            AddNestedTypesToNametoClassHashTable(hashTable, namespaze, name, nestedClass);
    }

    Il2CppString* createcsstr(std::string_view inp, bool pinned) {
        il2cpp_functions::Init();
        auto* obj = il2cpp_functions::string_new_len(inp.data(), (uint32_t)inp.length());
        if (!pinned) {
            auto gchandle = il2cpp_functions::gchandle_new(&obj->object, pinned);
            log(DEBUG, "gchandle created for string! handle ID: %u", gchandle);
            // gchandle (probably) does not need to be returned here.
        }
        return obj;
    }

    [[nodiscard]] bool Match(const Il2CppObject* source, const Il2CppClass* klass) noexcept {
        return (source->klass == klass);
    }

    bool AssertMatch(const Il2CppObject* source, Il2CppClass* klass) {
        il2cpp_functions::Init();
        if (!Match(source, klass)) {
            log(CRITICAL, "il2cpp_utils: AssertMatch: source with class '%s' does not match class '%s'!",
                ClassStandardName(source->klass).c_str(), ClassStandardName(klass).c_str());
            SAFE_ABORT();
        }
        return true;
    }
}
