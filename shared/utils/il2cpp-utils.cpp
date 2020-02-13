#include "utils.h"

#include "alphanum.hpp"
#include <algorithm>
#include <map>
#include <unordered_set>

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

    bool IsConvertible(const Il2CppType* to, const Il2CppType* from) {
        if (il2cpp_functions::type_equals(to, from)) {
            log(DEBUG, "IsConvertible: types equal (%s), returning true", TypeGetSimpleName(to));
            return true;
        }
        log(DEBUG, "%s (%x) != %s (%x)", TypeGetSimpleName(to), to->type, TypeGetSimpleName(from), from->type);

        auto classFrom = il2cpp_functions::class_from_il2cpp_type(from);
        auto classTo = il2cpp_functions::class_from_il2cpp_type(to);
        if (classTo) {
            if (classTo->enumtype) {
                auto e = classTo->element_class;
                if (e != classTo) {
                    log(DEBUG, "IsConvertible: extracted value type '%s' from Enum 'to', recursing!", TypeGetSimpleName(&e->byval_arg));
                    if (IsConvertible(&e->byval_arg, from)) return true;
                }
            }

            // See if 'from' satisfies the interface represented by 'to'.
            if (IsInterface(classTo)) {
                void* iter = nullptr;
                Il2CppClass* interface;
                while ((interface = il2cpp_functions::class_get_interfaces(classFrom, &iter))) {
                    log(DEBUG, "IsConvertible: comparing interfaces: %s, %s", ClassStandardName(classTo).c_str(), ClassStandardName(interface).c_str());
                    if (classTo == interface) {
                        log(DEBUG, "IsConvertible: interface match (%s), returning true!", ClassStandardName(classTo).c_str());
                        return true;
                    }
                }
            }
        }

        // Try from's parent
        if (classFrom && classFrom->parent && classFrom->parent != classFrom) {
            log(DEBUG, "IsConvertible: recursing on 'from's parent");
            return IsConvertible(to, il2cpp_functions::class_get_type(classFrom->parent));
        }
        log(DEBUG, "IsConvertible: no match found, returning false!");
        return false;
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

    // Gets the type enum of a given type
    // TODO Remove this method! Replace with default typesystem
    int GetTypeEnum(std::string_view name_space, std::string_view type_name) {
        auto klass = GetClassFromName(name_space, type_name);
        auto typ = il2cpp_functions::class_get_type(klass);
        return il2cpp_functions::type_get_type(typ);
    }

    static std::unordered_map<int, const char*> typeMap;

    const char* TypeGetSimpleName(const Il2CppType* type) {
        il2cpp_functions::Init();

        if (typeMap.empty()) {
            typeMap[GetTypeEnum("System", "Boolean")] = "bool";
            typeMap[GetTypeEnum("System", "Byte")] = "byte";
            typeMap[GetTypeEnum("System", "SByte")] = "sbyte";
            typeMap[GetTypeEnum("System", "Char")] = "char";
            typeMap[GetTypeEnum("System", "Single")] = "float";
            typeMap[GetTypeEnum("System", "Double")] = "double";
            typeMap[GetTypeEnum("System", "Int16")] = "short";
            typeMap[GetTypeEnum("System", "UInt16")] = "ushort";
            typeMap[GetTypeEnum("System", "Int32")] = "int";
            typeMap[GetTypeEnum("System", "UInt32")] = "uint";
            typeMap[GetTypeEnum("System", "Int64")] = "long";
            typeMap[GetTypeEnum("System", "UInt64")] = "ulong";
            typeMap[GetTypeEnum("System", "Object")] = "object";
            typeMap[GetTypeEnum("System", "String")] = "string";
            typeMap[GetTypeEnum("System", "Void")] = "void";
        }
        auto p = typeMap.find(il2cpp_functions::type_get_type(type));
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

        // Check cache
        auto key = std::pair<std::string, std::string>(name_space, type_name);
        auto itr = namesToClassesCache.find(key);
        if (itr != namesToClassesCache.end()) {
            return itr->second;
        }
        auto dom = il2cpp_functions::domain_get();
        if (!dom) {
            log(ERROR, "GetClassFromName: Could not get domain!");
            return nullptr;
        }
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
                namesToClassesCache.insert({key, klass});
                return klass;
            }
        }
        log(ERROR, "il2cpp_utils: GetClassFromName: Could not find class with namepace: %s and name: %s",
            name_space.data(), type_name.data());
        return nullptr;
    }

    const MethodInfo* FindMethod(Il2CppClass* klass, std::string_view methodName, int argsCount) {
        il2cpp_functions::Init();

        if (!klass) return nullptr;
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
            log(ERROR, "could not find method %s with %i parameters in class %s (namespace '%s')!", methodName.data(),
                argsCount, il2cpp_functions::class_get_name(klass), il2cpp_functions::class_get_namespace(klass));
            LogMethods(klass, true);
        }
        classesNamesToMethodsCache.insert({key, methodInfo});
        return methodInfo;
    }

    const MethodInfo* FindMethod(Il2CppClass* klass, std::string_view methodName, std::vector<const Il2CppType*> argTypes) {
        il2cpp_functions::Init();

        if (!klass) return nullptr;
        // Check Cache
        auto innerPair = classesNamesTypesInnerPairType(methodName, argTypes);
        auto key = std::pair<Il2CppClass*, classesNamesTypesInnerPairType>(klass, innerPair);
        auto itr = classesNamesTypesToMethodsCache.find(key);
        if (itr != classesNamesTypesToMethodsCache.end()) {
            return itr->second;
        }

        void* myIter = nullptr;
        const MethodInfo* current;
        const MethodInfo* methodInfo = nullptr;
        // Does NOT automatically recurse through klass's parents
        while ((current = il2cpp_functions::class_get_methods(klass, &myIter))) {
            if (strcmp(current->name, methodName.data()) == 0 && ParameterMatch(current, argTypes)) {
                methodInfo = current;
            }
        }
        if (!methodInfo && klass->parent && klass->parent != klass) {
            methodInfo = FindMethod(klass->parent, methodName, argTypes);
        }

        if (!methodInfo) {
            std::stringstream ss;
            ss << "could not find method " << methodName << "(";
            bool first = true;
            for (auto t : argTypes) {
                if (!first) ss << ", ";
                first = false;
                ss << TypeGetSimpleName(t);
            }
            ss << ") in class " << il2cpp_functions::class_get_name(klass) << " (namespace '" << il2cpp_functions::class_get_namespace(klass) << "')!";
            log(ERROR, "%s", ss.str().c_str());
            LogMethods(klass);
        }
        classesNamesTypesToMethodsCache.insert({key, methodInfo});
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

        if (!klass) return nullptr;
        // Check Cache
        auto key = std::pair<Il2CppClass*, std::string>(klass, fieldName);
        auto itr = classesNamesToFieldsCache.find(key);
        if (itr != classesNamesToFieldsCache.end()) {
            return itr->second;
        }
        auto field = il2cpp_functions::class_get_field_from_name(klass, fieldName.data());
        if (!field) {
            log(ERROR, "could not find field %s in class %s (namespace '%s')!", fieldName.data(),
                il2cpp_functions::class_get_name(klass), il2cpp_functions::class_get_namespace(klass));
            LogFields(klass, true);
            if (klass->parent != klass) field = FindField(klass->parent, fieldName);
        }
        classesNamesToFieldsCache.insert({key, field});
        return field;
    }

    Il2CppObject* GetFieldValue(Il2CppObject* instance, FieldInfo* field) {
        il2cpp_functions::Init();
        if (!field) {
            log(ERROR, "il2cpp_utils: GetFieldValueObject: Null FieldInfo!");
            return nullptr;
        }
        return il2cpp_functions::field_get_value_object(field, instance);
    }

    Il2CppObject* GetFieldValue(Il2CppClass* klass, std::string_view fieldName) {
        il2cpp_functions::Init();
        if (!klass) {
            log(ERROR, "il2cpp_utils: GetFieldValue: Could not find object class!");
            return nullptr;
        }
        auto field = FindField(klass, fieldName);
        if (!field) return nullptr;
        return GetFieldValue(nullptr, field);
    }

    Il2CppObject* GetFieldValue(Il2CppObject* instance, std::string_view fieldName) {
        il2cpp_functions::Init();
        if (!instance) {
            log(ERROR, "il2cpp_utils: GetFieldValueObject: Null instance parameter!");
            return nullptr;
        }
        auto field = FindField(instance, fieldName);
        if (!field) return nullptr;
        return GetFieldValue(instance, field);
    }

    bool SetFieldValue(Il2CppObject* instance, FieldInfo* field, void* value) {
        il2cpp_functions::Init();

        if (!field) {
            log(ERROR, "il2cpp_utils: SetFieldValue: Null field parameter!");
            return false;
        }
        if (instance) {
            il2cpp_functions::field_set_value(instance, field, value);
        } else { // Fallback to perform a static field set
            il2cpp_functions::field_static_set_value(field, value);
        }
        return true;
    }

    bool SetFieldValue(Il2CppClass* klass, std::string_view fieldName, void* value) {
        il2cpp_functions::Init();

        if (!klass) {
            log(ERROR, "il2cpp_utils: SetFieldValue: Null klass parameter!");
            return false;
        }
        auto field = FindField(klass, fieldName);
        if (!field) return false;
        return SetFieldValue(nullptr, field, value);
    }

    bool SetFieldValue(Il2CppObject* instance, std::string_view fieldName, void* value) {
        il2cpp_functions::Init();

        if (!instance) {
            log(ERROR, "il2cpp_utils: SetFieldValue: Null instance parameter!");
            return false;
        }
        auto field = FindField(instance, fieldName);
        if (!field) return false;
        return SetFieldValue(instance, field, value);
    }

    Il2CppReflectionType* MakeGenericType(Il2CppReflectionType* gt, Il2CppArray* types) {
        il2cpp_functions::Init();

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

    Il2CppClass* MakeGeneric(const Il2CppClass* klass, std::vector<const Il2CppClass*> args) {
        il2cpp_functions::Init();
 
        auto typ = GetClassFromName("System", "Type");
        if (!typ) {
            return nullptr;
        }
 
        auto klassType = GetSystemType(klass);
        if (!klassType) {
            log(ERROR, "il2cpp_utils: MakeGeneric: Failed to get class type object!");
            return nullptr;
        }
 
        // Call Type.MakeGenericType on it
        auto a = il2cpp_functions::array_new_specific(typ, args.size());
        if (!a) {
            log(ERROR, "il2cpp_utils: MakeGeneric: Failed to make new array with length: %zu", args.size());
            return nullptr;
        }
 
        int i = 0;
        for (auto arg : args) {
            auto o = GetSystemType(arg);
            if (!o) {
                log(ERROR, "il2cpp_utils: MakeGeneric: Failed to get type for %s", il2cpp_functions::class_get_name_const(arg));
                return nullptr;
            }
            il2cpp_array_set(a, void*, i, reinterpret_cast<void*>(o));
            i++;
        }

        auto reflection_type = MakeGenericType(reinterpret_cast<Il2CppReflectionType*>(klassType), a);
        if (!reflection_type) {
            log(ERROR, "il2cpp_utils: MakeGeneric: Failed to MakeGenericType from Il2CppReflectionType and Il2CppArray!");
            return nullptr;
        }

        auto ret = il2cpp_functions::class_from_system_type(reinterpret_cast<Il2CppReflectionType*>(reflection_type));
        if (!ret) {
            log(ERROR, "il2cpp_utils: MakeGeneric: Failed to get class from Il2CppReflectionType!");
            return nullptr;
        }
        log(DEBUG, "il2cpp_utils: MakeGeneric: returning %s", il2cpp_functions::class_get_name(ret));
        return ret;
    }

    Il2CppObject* GetSystemType(const Il2CppClass* klass) {
        if (!klass) return nullptr;
        il2cpp_functions::Init();
        return il2cpp_functions::type_get_object(il2cpp_functions::class_get_type_const(klass));
    }

    Il2CppObject* GetSystemType(std::string_view nameSpace, std::string_view className) {
        return GetSystemType(il2cpp_utils::GetClassFromName(nameSpace, className));
    }

    void LogMethod(const MethodInfo* method) {
        il2cpp_functions::Init();
 
        auto flags = il2cpp_functions::method_get_flags(method, nullptr);
        std::stringstream flagStream;
        if (flags & METHOD_ATTRIBUTE_STATIC) flagStream << "static ";
        if (flags & METHOD_ATTRIBUTE_VIRTUAL) flagStream << "virtual ";
        if (flags & METHOD_ATTRIBUTE_ABSTRACT) flagStream << "abstract ";
        const auto& flagStrRef = flagStream.str();  
        const char* flagStr = flagStrRef.c_str();
        auto retType = il2cpp_functions::method_get_return_type(method);
        auto retTypeStr = TypeGetSimpleName(retType);
        auto methodName = il2cpp_functions::method_get_name(method);
        methodName = methodName ? methodName : "__noname__";
        std::stringstream paramStream;
        for (int i = 0; i < il2cpp_functions::method_get_param_count(method); i++) {
            if (i > 0) paramStream << ", ";
            auto argType = il2cpp_functions::method_get_param(method, i);
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

        auto flags = il2cpp_functions::field_get_flags(field);
        const char* flagStr = (flags & FIELD_ATTRIBUTE_STATIC) ? "static " : "";
        auto type = il2cpp_functions::field_get_type(field);
        auto typeStr = TypeGetSimpleName(type);
        auto name = il2cpp_functions::field_get_name(field);
        name = name ? name : "__noname__";
        auto offset = il2cpp_functions::field_get_offset(field);

        log(DEBUG, "%s%s %s; // 0x%lx, flags: 0x%.4X", flagStr, typeStr, name, offset, flags);
    }

    void LogFields(Il2CppClass* klass, bool logParents) {
        if (!klass) return;
        void* myIter = nullptr;
        FieldInfo* field;
        if (klass->name) il2cpp_functions::Class_Init(klass);
        if (logParents) log(INFO, "class name: %s", ClassStandardName(klass).c_str());

        log(DEBUG, "property_count: %i, field_count: %i", klass->property_count, klass->field_count);
        while ((field = il2cpp_functions::class_get_fields(klass, &myIter))) {
            LogField(field);
        }
        usleep(100);
        if (logParents && klass->parent && klass->parent != klass) {
            LogFields(klass->parent, logParents);
        }
    }

    void GenericsToStringHelper(Il2CppGenericClass* genClass, std::ostream& os) {
        auto genContext = &genClass->context;
        auto genInst = genContext->class_inst;
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
        auto declaring = il2cpp_functions::class_get_declaring_type(klass);
        bool hasNamespace = (namespaze && namespaze[0] != '\0');
        if (!hasNamespace && declaring) {
            ss << ClassStandardName(declaring) << "/";
        } else {
            ss << namespaze << "::";
        }
        ss << il2cpp_functions::class_get_name(klass);

        if (generics) {
            il2cpp_functions::class_is_generic(klass);
            auto genClass = klass->generic_class;
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
            return ClassStandardName(il2cpp_functions::GenericClass_GetClass(genClass));
        }
        return "?";
    }

    void LogMethods(Il2CppClass* klass, bool logParents) {
        if (!klass) return;
        if (klass->name) il2cpp_functions::Class_Init(klass);
        if (logParents) log(INFO, "class name: %s", ClassStandardName(klass).c_str());

        log(DEBUG, "method_count: %i", klass->method_count);
        for (int i = 0; i < klass->method_count; i++) {
            if (klass->methods[i]) {
                log(DEBUG, "Method %i:", i);
                LogMethod(klass->methods[i]);
            } else {
                log(DEBUG, "Method: %i Does not exist!", i);
            }
        }
        usleep(100);
        if (logParents && klass->parent && (klass->parent != klass)) {
            LogMethods(klass->parent, logParents);
        }
    }

    static int indent = -1;
    static int maxIndent;
    std::unordered_set<Il2CppClass*> loggedClasses;
    void LogClass(Il2CppClass* klass, bool logParents) {
        il2cpp_functions::Init();

        if (loggedClasses.count(klass)) {
            log(DEBUG, "Already logged %p!", klass);
            return;
        }
        loggedClasses.insert(klass);

        if (!klass) {
            return;
        }
        if (klass->klass != klass) {
            log(WARNING, "LogClass: %p is likely NOT an Il2CppClass*! Returning!", klass);
            return;
        }
        if (!klass->name) {
            log(ERROR, "LogClass: Class does not have a name. Returning.");
            return;
        }

        indent++;
        // Note: il2cpp stops at GenericMetadata::MaximumRuntimeGenericDepth (which is 8)
        maxIndent = std::max(indent, maxIndent);

        void* myIter = nullptr;
        bool methodInit = false;
        if (klass->name) {
            if (!il2cpp_functions::Class_Init(klass)) {
                log(WARNING, "Class::Init failed!");
            } else {
                methodInit = true;
            }
        }
        if (!methodInit) {
            il2cpp_functions::class_get_methods(klass, &myIter);  // this initializes the method data
        }
        log(DEBUG, "%i ======================CLASS INFO FOR CLASS: %s======================", indent, ClassStandardName(klass).c_str());
        log(DEBUG, "Pointer: %p", klass);
        log(DEBUG, "Type Token: %i", il2cpp_functions::class_get_type_token(klass));
        auto typeDefIdx = klass->generic_class ? klass->generic_class->typeDefinitionIndex : il2cpp_functions::MetadataCache_GetIndexForTypeDefinition(klass);
        log(DEBUG, "TypeDefinitionIndex: %i", typeDefIdx);
        // Repair the typeDefinition value if it was null but we found one
        if (!klass->typeDefinition && typeDefIdx > 0) klass->typeDefinition = il2cpp_functions::MetadataCache_GetTypeDefinitionFromIndex(typeDefIdx);
        log(DEBUG, "Type definition: %p", klass->typeDefinition);

        log(DEBUG, "Assembly Name: %s", il2cpp_functions::class_get_assemblyname(klass));

        auto typ = il2cpp_functions::class_get_type(klass);
        if (typ) {
            log(DEBUG, "Type name: %s", il2cpp_functions::type_get_name(typ));
            auto ch = il2cpp_functions::Type_GetName(typ, IL2CPP_TYPE_NAME_FORMAT_REFLECTION);
            log(DEBUG, "Type reflection name: %s", ch);
            // log(DEBUG, "Type full name: %s", il2cpp_functions::Type_GetName(typ, IL2CPP_TYPE_NAME_FORMAT_FULL_NAME));
            log(DEBUG, "Fully qualifed type name: %s", il2cpp_functions::type_get_assembly_qualified_name(typ));
        }
        log(DEBUG, "Rank: %i", il2cpp_functions::class_get_rank(klass));
        log(DEBUG, "Flags: 0x%.8X", il2cpp_functions::class_get_flags(klass));
        log(DEBUG, "Event Count: %i", klass->event_count);
        log(DEBUG, "Method Count: %i", klass->method_count);
        log(DEBUG, "Is Generic: %i", il2cpp_functions::class_is_generic(klass));
        log(DEBUG, "Is Abstract: %i", il2cpp_functions::class_is_abstract(klass));

        // Some methods, such as GenericClass::GetClass, may not initialize all fields in Il2CppClass, thus not meet all implicit contracts defined by the comments in Il2CppClass's struct definition.
        // But unless we're blind, the only method that sets is_generic on non-methods is MetadataCache::FromTypeDefinition. That method also contains the only assignment of genericContainerIndex.
        // Therefore, this code makes only the following assumptions:
        // 1. If is_generic is set, then genericContainerIndex was also intentionally set (even if it's 0) and is not -1 (invalid)
        // 2. Even if is_generic wasn't set, a positive genericContainerIndex was intentionally set that way and is a valid index.
        if (klass->is_generic || klass->genericContainerIndex > 0) {
            auto genContainer = il2cpp_functions::MetadataCache_GetGenericContainerFromIndex(klass->genericContainerIndex);
            log(DEBUG, "genContainer: idx %i, ownerIndex: %i, is_method: %i", klass->genericContainerIndex, genContainer->ownerIndex, genContainer->is_method);
            if (genContainer->ownerIndex != typeDefIdx) {
                log(ERROR, "genContainer ownerIndex mismatch!");
            }
            for (int i = 0; i < genContainer->type_argc; i++) {
                auto genParamIdx = genContainer->genericParameterStart + i;
                auto genParam = il2cpp_functions::MetadataCache_GetGenericParameterFromIndex(genParamIdx);
                if (genParam) {
                    log(DEBUG, "genParam %i, idx %i: ownerIdx %i, name %s, num %i, flags (see IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_X in il2cpp-tabledefs.h) 0x%.2X",
                                i, genParamIdx, genParam->ownerIndex, il2cpp_functions::MetadataCache_GetStringFromIndex(genParam->nameIndex), genParam->num, genParam->flags);
                } else {
                    log(DEBUG, "genParam %i, idx %i: null", i, genParamIdx);
                }
            }
        } else {
            log(DEBUG, "genericContainerIndex: %i", klass->genericContainerIndex);
        }

        auto typDef = klass->typeDefinition;
        if (typDef) {
            Il2CppClass* childClass;
            log(DEBUG, "rgctxCount: %i, startIndex: %i", typDef->rgctxCount, typDef->rgctxStartIndex);
            for (int i = 0; i < typDef->rgctxCount; i++) {
                auto idx = typDef->rgctxStartIndex + i;
                auto def = il2cpp_functions::MetadataCache_GetRGCTXDefinitionFromIndex(idx);
                Il2CppRGCTXData rgctxData = klass->rgctx_data[i];
                log(INFO, "-- RGCTX child %i is type %i:", i, def->type);
                switch(def->type) {
                    case IL2CPP_RGCTX_DATA_METHOD:
                        // TODO: compare rgctxData.method with def->data.methodIndex?
                        LogMethod(rgctxData.method);
                        break;
                    case IL2CPP_RGCTX_DATA_TYPE:
                    case IL2CPP_RGCTX_DATA_CLASS:
                    case IL2CPP_RGCTX_DATA_ARRAY:
                        childClass = il2cpp_functions::MetadataCache_GetTypeInfoFromTypeIndex(def->data.typeIndex);
                        log(INFO, "---- %s (from typeIndex)", ClassStandardName(childClass).c_str());
                        if (def->type == IL2CPP_RGCTX_DATA_TYPE) {
                            if (rgctxData.type) log(INFO, "---- %s (rgctxData.type)", il2cpp_functions::Type_GetName(rgctxData.type, IL2CPP_TYPE_NAME_FORMAT_REFLECTION));
                        } else {
                            log(INFO, "rgctxData.klass: %p (self = %p)", rgctxData.klass, klass);
                            log(INFO, "childClass %p, rgctxData.klass == childClass: %i,", childClass, rgctxData.klass == childClass);
                            if (rgctxData.klass && rgctxData.klass != klass) log(INFO, "---- %s (rgctxData.klass)", ClassStandardName(childClass).c_str());
                        }
                        break;
                    default:
                        log(WARNING, "Don't know how to handle Il2CppRGCTXDataType %i", def->type);
                        break;
                }
            }
        }

        log(DEBUG, "%i =========METHODS=========", indent);
        LogMethods(klass);

        auto declaring = il2cpp_functions::class_get_declaring_type(klass);
        log(DEBUG, "declaring type: %p", declaring);
        if (declaring && logParents) LogClass(declaring, logParents);
        auto element = il2cpp_functions::class_get_element_class(klass);
        log(DEBUG, "element class: %p (self = %p)", element, klass);
        if (element && element != klass && logParents) LogClass(element, logParents);

        log(DEBUG, "%i =========FIELDS=========", indent);
        LogFields(klass);
        log(DEBUG, "%i =========END FIELDS=========", indent);

        auto parent = il2cpp_functions::class_get_parent(klass);
        log(DEBUG, "parent: %p (%s)", parent, parent ? ClassStandardName(parent).c_str() : "");
        if (parent && logParents) LogClass(parent, logParents);
        log(DEBUG, "%i, ==================================================================================", indent);
        indent--;
    }

    static std::unordered_map<Il2CppClass*, std::map<std::string, Il2CppGenericClass*, doj::alphanum_less<std::string>>> classToGenericClassMap;
    void BuildGenericsMap() {
        auto metadataReg = *il2cpp_functions::s_Il2CppMetadataRegistration;
        log(DEBUG, "metadataReg: %p, offset = %llX", metadataReg, ((long long)metadataReg) - getRealOffset(nullptr));
        if (!metadataReg) {
            log(WARNING, "metadataReg not found!");
            return;
        }
        int uncached_class_count = 0;
        for (int i=0; i < metadataReg->genericClassesCount; i++) {
            Il2CppGenericClass* genClass = metadataReg->genericClasses[i];
            if (!genClass) {
                continue;
            }
            if (!(genClass->cached_class)) {
                uncached_class_count++;
            }
            std::string genClassName = GenericClassStandardName(genClass);

            auto typeDefClass = il2cpp_functions::MetadataCache_GetTypeInfoFromTypeDefinitionIndex(genClass->typeDefinitionIndex);
            if (!typeDefClass) {
                continue;
            }
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
        auto dom = il2cpp_functions::domain_get();
        // Get all il2cpp assemblies
        size_t size;
        auto assembs = il2cpp_functions::domain_get_assemblies(dom, &size);
        for (size_t i = 0; i < size; ++i) {
            // Get image for each assembly
            if (assembs[i] == nullptr) {
                log(DEBUG, "Assembly %i was null! Skipping.", i);
                continue;
            }
            log(DEBUG, "Scanning assembly \"%s\"", assembs[i]->aname.name);
            auto img = il2cpp_functions::assembly_get_image(assembs[i]);
            if (img->nameToClassHashTable == nullptr) {
                log(DEBUG, "Assembly's nameToClassHashTable is empty. Populating it instead.");

                img->nameToClassHashTable = new Il2CppNameToTypeDefinitionIndexHashTable();
                for (uint32_t index = 0; index < img->typeCount; index++)
                {
                    TypeDefinitionIndex typeIndex = img->typeStart + index;
                    AddTypeToNametoClassHashTable(img, typeIndex);
                }

                for (uint32_t index = 0; index < img->exportedTypeCount; index++)
                {
                    auto typeIndex = il2cpp_functions::MetadataCache_GetExportedTypeFromIndex(img->exportedTypeStart + index);
                    if (typeIndex != kTypeIndexInvalid)
                        AddTypeToNametoClassHashTable(img, typeIndex);
                }
            }
            auto length = img->nameToClassHashTable->size();
            for (auto itr = img->nameToClassHashTable->begin(); itr != img->nameToClassHashTable->end(); ++itr) {
                // First is a KeyWrapper(pair(namespaceName, className))
                // Second is TypeDefinitionIndex
                if (strncmp(classPrefix.data(), itr->first.key.second, classPrefix.length()) == 0) {
                    // Starts with!
                    // Convert TypeDefinitionIndex --> class
                    auto klazz = il2cpp_functions::MetadataCache_GetTypeInfoFromTypeDefinitionIndex(itr->second);
                    matches[ClassStandardName(klazz)] = klazz;
                }
            }
        }
        log(DEBUG, "LogClasses:");
        for ( const auto &pair : matches ) {
            LogClass(pair.second, logParents);
            indent = -1;

            for ( const auto &genPair : classToGenericClassMap[pair.second] ) {
                log(DEBUG, "%s", genPair.first.c_str());
            }
            usleep(1000);  // 1/100th of the sleep at the end of il2cpp-functions::Init
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

    Il2CppString* createcsstr(std::string_view inp) {
        il2cpp_functions::Init();
        return il2cpp_functions::string_new_len(inp.data(), (uint32_t)inp.length());
    }

    [[nodiscard]] bool Match(const Il2CppObject* source, const Il2CppClass* klass) noexcept {
        return (source->klass == klass);
    }

    bool AssertMatch(const Il2CppObject* source, const Il2CppClass* klass) {
        il2cpp_functions::Init();
        if (!Match(source, klass)) {
            log(CRITICAL, "il2cpp_utils: AssertMatch: Unhandled subtype: namespace %s, class %s", 
                il2cpp_functions::class_get_namespace(source->klass), il2cpp_functions::class_get_name(source->klass));
            std::terminate();
        }
        return true;
    }
}
