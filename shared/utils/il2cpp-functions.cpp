#include <unistd.h>

#include "utils.h"
#include "il2cpp-functions.hpp"
#include "instruction-parsing.hpp"
#include "logging.hpp"

// copies of the highly-inlinable functions
const Il2CppTypeDefinition* il2cpp_functions::MetadataCache_GetTypeDefinitionFromIndex(TypeDefinitionIndex index) {
    CheckS_GlobalMetadata();
    if (index == kTypeDefinitionIndexInvalid) return NULL;

    IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) < s_GlobalMetadataHeader->typeDefinitionsCount / sizeof(Il2CppTypeDefinition));
    auto typeDefinitions = (const Il2CppTypeDefinition*)((const char*)s_GlobalMetadata + s_GlobalMetadataHeader->typeDefinitionsOffset);
    return typeDefinitions + index;
}

const char* il2cpp_functions::MetadataCache_GetStringFromIndex(StringIndex index) {
    CheckS_GlobalMetadata();
    IL2CPP_ASSERT(index <= s_GlobalMetadataHeader->stringCount);
    const char* strings = ((const char*)s_GlobalMetadata + s_GlobalMetadataHeader->stringOffset) + index;
    return strings;
}

const Il2CppGenericContainer* il2cpp_functions::MetadataCache_GetGenericContainerFromIndex(GenericContainerIndex index) {
    CheckS_GlobalMetadata();
    if (index == kGenericContainerIndexInvalid) return NULL;

    IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) <= s_GlobalMetadataHeader->genericContainersCount / sizeof(Il2CppGenericContainer));
    const Il2CppGenericContainer* genericContainers = (const Il2CppGenericContainer*)((const char*)s_GlobalMetadata + s_GlobalMetadataHeader->genericContainersOffset);
    return genericContainers + index;
}

const Il2CppGenericParameter* il2cpp_functions::MetadataCache_GetGenericParameterFromIndex(GenericParameterIndex index) {
    CheckS_GlobalMetadata();
    if (index == kGenericParameterIndexInvalid) return NULL;

    IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) <= s_GlobalMetadataHeader->genericParametersCount / sizeof(Il2CppGenericParameter));
    const Il2CppGenericParameter* genericParameters = (const Il2CppGenericParameter*)((const char*)s_GlobalMetadata + s_GlobalMetadataHeader->genericParametersOffset);
    return genericParameters + index;
}

TypeDefinitionIndex il2cpp_functions::MetadataCache_GetExportedTypeFromIndex(TypeDefinitionIndex index) {
    CheckS_GlobalMetadata();
    if (index == kTypeDefinitionIndexInvalid) return kTypeDefinitionIndexInvalid;

    IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) < s_GlobalMetadataHeader->exportedTypeDefinitionsCount / sizeof(TypeDefinitionIndex));
    auto exportedTypes = (TypeDefinitionIndex*)((const char*)s_GlobalMetadata + s_GlobalMetadataHeader->exportedTypeDefinitionsOffset);
    return *(exportedTypes + index);
}

Il2CppClass* il2cpp_functions::MetadataCache_GetNestedTypeFromIndex(NestedTypeIndex index) {
    CheckS_GlobalMetadata();
    IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) <= s_GlobalMetadataHeader->nestedTypesCount / sizeof(TypeDefinitionIndex));
    auto nestedTypeIndices = (const TypeDefinitionIndex*)((const char*)s_GlobalMetadata + s_GlobalMetadataHeader->nestedTypesOffset);

    return il2cpp_functions::MetadataCache_GetTypeInfoFromTypeDefinitionIndex(nestedTypeIndices[index]);
}

const TypeDefinitionIndex il2cpp_functions::MetadataCache_GetIndexForTypeDefinition(const Il2CppClass* typeDefinition) {
    CheckS_GlobalMetadata();
    IL2CPP_ASSERT(typeDefinition->typeDefinition);
    const Il2CppTypeDefinition* typeDefinitions = (const Il2CppTypeDefinition*)((const char*)s_GlobalMetadata + s_GlobalMetadataHeader->typeDefinitionsOffset);

    IL2CPP_ASSERT(typeDefinition->typeDefinition >= typeDefinitions && typeDefinition->typeDefinition < typeDefinitions + s_GlobalMetadataHeader->typeDefinitionsCount);

    ptrdiff_t index = typeDefinition->typeDefinition - typeDefinitions;
    IL2CPP_ASSERT(index <= std::numeric_limits<TypeDefinitionIndex>::max());
    return static_cast<TypeDefinitionIndex>(index);
}

char* il2cpp_functions::Type_GetName(const Il2CppType *type, Il2CppTypeNameFormat format) {
    if (!il2cpp_functions::_Type_GetName_) return nullptr;
    // TODO debug the ref/lifetime weirdness with _Type_GetName_ to avoid the need for explicit allocation
    const auto str = il2cpp_functions::_Type_GetName_(type, format);
    char* buffer = static_cast<char*>(il2cpp_functions::alloc(str.length() + 1));
    memcpy(buffer, str.c_str(), str.length() + 1);
    return buffer;
}

#ifdef FILE_LOG
// closes log on application shutdown
// Address is unused, so left as 0
MAKE_HOOK_OFFSETLESS(shutdown_hook, void) {
    log_close();
    shutdown_hook();
}
#endif

static const Il2CppMethodDefinition* GetMethodDefinitionFromIndex(MethodIndex index) {
    il2cpp_functions::CheckS_GlobalMetadata();
    IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) <= il2cpp_functions::s_GlobalMetadataHeader->methodsCount / sizeof(Il2CppMethodDefinition));
    const Il2CppMethodDefinition* methods = (const Il2CppMethodDefinition*)((const char*)il2cpp_functions::s_GlobalMetadata + il2cpp_functions::s_GlobalMetadataHeader->methodsOffset);
    return methods + index;
}

static const MethodInfo* GetMethodInfoFromMethodDefinitionIndex(MethodIndex index) {
    il2cpp_functions::CheckS_GlobalMetadata();
    IL2CPP_ASSERT(index >= 0 && static_cast<uint32_t>(index) <= il2cpp_functions::s_GlobalMetadataHeader->methodsCount / sizeof(Il2CppMethodDefinition));

    const Il2CppMethodDefinition* methodDefinition = GetMethodDefinitionFromIndex(index);
    Il2CppClass* klass = il2cpp_functions::MetadataCache_GetTypeInfoFromTypeDefinitionIndex(methodDefinition->declaringType);
    void* myIter = nullptr;
    auto* m1 = il2cpp_functions::class_get_methods(klass, &myIter);

    return klass->methods[index - klass->typeDefinition->methodStart];
}

// must be done on-demand because the pointers aren't necessarily correct at the time of il2cpp_functions::Init
void il2cpp_functions::CheckS_GlobalMetadata() {
    if (!s_GlobalMetadataHeader) {
        s_GlobalMetadata = *(il2cpp_functions::s_GlobalMetadataPtr);
        s_GlobalMetadataHeader = *(il2cpp_functions::s_GlobalMetadataHeaderPtr);
        log(DEBUG, "sanity: %X (should be 0xFAB11BAF)", s_GlobalMetadataHeader->sanity);
        log(DEBUG, "version: %i", s_GlobalMetadataHeader->version);
        assert(s_GlobalMetadataHeader->sanity == 0xFAB11BAF);
        log(DEBUG, "typeDefinitionsOffset: %i", s_GlobalMetadataHeader->typeDefinitionsOffset);
        log(DEBUG, "exportedTypeDefinitionsOffset: %i", s_GlobalMetadataHeader->exportedTypeDefinitionsOffset);
        log(DEBUG, "nestedTypesOffset: %i", s_GlobalMetadataHeader->nestedTypesOffset);
        // TODO: use il2cpp_functions::defaults to define the il2cpp_defaults variable mentioned in il2cpp-class-internals.h

        for (MethodIndex i = 0; i < il2cpp_functions::s_GlobalMetadataHeader->methodsCount / sizeof(Il2CppMethodDefinition); i++) {
            auto* method = GetMethodInfoFromMethodDefinitionIndex(i);
            if (method) {
                methods.emplace(method->methodPointer, method);
            }
        }
    }
}

// Autogenerated; modified by zoller27osu
// Initializes all of the IL2CPP functions via dlopen and dlsym for use.
void il2cpp_functions::Init() {
    if (initialized) {
        return;
    }
    log(INFO, "il2cpp_functions: Init: Initializing all IL2CPP Functions...");
    dlerror();  // clears existing errors
    void *imagehandle = dlopen(IL2CPP_SO_PATH, RTLD_GLOBAL | RTLD_LAZY);
    if (!imagehandle) {
        log(ERROR, "Failed to dlopen %s: %s!", IL2CPP_SO_PATH, dlerror());
        return;
    }

    // Please verify that these have more than 1 instruction to their name before attempting to hook them!
    *(void**)(&init) = dlsym(imagehandle, "il2cpp_init");
    log(INFO, "Loaded: il2cpp_init, error: %s", dlerror());
    *(void**)(&shutdown) = dlsym(imagehandle, "il2cpp_shutdown");
    log(INFO, "Loaded: il2cpp_shutdown, error: %s", dlerror());
    *(void**)(&init_utf16) = dlsym(imagehandle, "il2cpp_init_utf16");
    log(INFO, "Loaded: il2cpp_init_utf16");
    *(void**)(&set_config_dir) = dlsym(imagehandle, "il2cpp_set_config_dir");
    log(INFO, "Loaded: il2cpp_set_config_dir");
    *(void**)(&set_data_dir) = dlsym(imagehandle, "il2cpp_set_data_dir");
    log(INFO, "Loaded: il2cpp_set_data_dir");
    *(void**)(&set_temp_dir) = dlsym(imagehandle, "il2cpp_set_temp_dir");
    log(INFO, "Loaded: il2cpp_set_temp_dir");
    *(void**)(&set_commandline_arguments) = dlsym(imagehandle, "il2cpp_set_commandline_arguments");
    log(INFO, "Loaded: il2cpp_set_commandline_arguments");
    *(void**)(&set_commandline_arguments_utf16) = dlsym(imagehandle, "il2cpp_set_commandline_arguments_utf16");
    log(INFO, "Loaded: il2cpp_set_commandline_arguments_utf16");
    *(void**)(&set_config_utf16) = dlsym(imagehandle, "il2cpp_set_config_utf16");
    log(INFO, "Loaded: il2cpp_set_config_utf16");
    *(void**)(&set_config) = dlsym(imagehandle, "il2cpp_set_config");
    log(INFO, "Loaded: il2cpp_set_config");
    *(void**)(&set_memory_callbacks) = dlsym(imagehandle, "il2cpp_set_memory_callbacks");
    log(INFO, "Loaded: il2cpp_set_memory_callbacks");
    *(void**)(&get_corlib) = dlsym(imagehandle, "il2cpp_get_corlib");
    log(INFO, "Loaded: il2cpp_get_corlib");
    *(void**)(&add_internal_call) = dlsym(imagehandle, "il2cpp_add_internal_call");
    log(INFO, "Loaded: il2cpp_add_internal_call");
    *(void**)(&resolve_icall) = dlsym(imagehandle, "il2cpp_resolve_icall");
    log(INFO, "Loaded: il2cpp_resolve_icall");
    *(void**)(&alloc) = dlsym(imagehandle, "il2cpp_alloc");
    log(INFO, "Loaded: il2cpp_alloc");
    *(void**)(&free) = dlsym(imagehandle, "il2cpp_free");
    log(INFO, "Loaded: il2cpp_free");
    *(void**)(&array_class_get) = dlsym(imagehandle, "il2cpp_array_class_get");
    log(INFO, "Loaded: il2cpp_array_class_get");
    *(void**)(&array_length) = dlsym(imagehandle, "il2cpp_array_length");
    log(INFO, "Loaded: il2cpp_array_length");
    *(void**)(&array_get_byte_length) = dlsym(imagehandle, "il2cpp_array_get_byte_length");
    log(INFO, "Loaded: il2cpp_array_get_byte_length");
    *(void**)(&array_new) = dlsym(imagehandle, "il2cpp_array_new");
    log(INFO, "Loaded: il2cpp_array_new");
    *(void**)(&array_new_specific) = dlsym(imagehandle, "il2cpp_array_new_specific");
    log(INFO, "Loaded: il2cpp_array_new_specific");
    *(void**)(&array_new_full) = dlsym(imagehandle, "il2cpp_array_new_full");
    log(INFO, "Loaded: il2cpp_array_new_full");
    *(void**)(&bounded_array_class_get) = dlsym(imagehandle, "il2cpp_bounded_array_class_get");
    log(INFO, "Loaded: il2cpp_bounded_array_class_get");
    *(void**)(&array_element_size) = dlsym(imagehandle, "il2cpp_array_element_size");
    log(INFO, "Loaded: il2cpp_array_element_size");
    *(void**)(&assembly_get_image) = dlsym(imagehandle, "il2cpp_assembly_get_image");
    log(INFO, "Loaded: il2cpp_assembly_get_image");
    #ifdef UNITY_2019
    *(void**)(&class_for_each) = dlsym(imagehandle, "il2cpp_class_for_each");
    log(INFO, "Loaded: il2cpp_class_for_each");
    #endif
    *(void**)(&class_enum_basetype) = dlsym(imagehandle, "il2cpp_class_enum_basetype");
    log(INFO, "Loaded: il2cpp_class_enum_basetype");
    *(void**)(&class_is_generic) = dlsym(imagehandle, "il2cpp_class_is_generic");
    log(INFO, "Loaded: il2cpp_class_is_generic");
    *(void**)(&class_is_inflated) = dlsym(imagehandle, "il2cpp_class_is_inflated");
    log(INFO, "Loaded: il2cpp_class_is_inflated");
    *(void**)(&class_is_assignable_from) = dlsym(imagehandle, "il2cpp_class_is_assignable_from");
    log(INFO, "Loaded: il2cpp_class_is_assignable_from");
    *(void**)(&class_is_subclass_of) = dlsym(imagehandle, "il2cpp_class_is_subclass_of");
    log(INFO, "Loaded: il2cpp_class_is_subclass_of");
    *(void**)(&class_has_parent) = dlsym(imagehandle, "il2cpp_class_has_parent");
    log(INFO, "Loaded: il2cpp_class_has_parent");
    *(void**)(&class_from_il2cpp_type) = dlsym(imagehandle, "il2cpp_class_from_il2cpp_type");
    log(INFO, "Loaded: il2cpp_class_from_il2cpp_type");
    *(void**)(&class_from_name) = dlsym(imagehandle, "il2cpp_class_from_name");
    log(INFO, "Loaded: il2cpp_class_from_name");
    *(void**)(&class_from_system_type) = dlsym(imagehandle, "il2cpp_class_from_system_type");
    log(INFO, "Loaded: il2cpp_class_from_system_type");
    *(void**)(&class_get_element_class) = dlsym(imagehandle, "il2cpp_class_get_element_class");
    log(INFO, "Loaded: il2cpp_class_get_element_class");
    *(void**)(&class_get_events) = dlsym(imagehandle, "il2cpp_class_get_events");
    log(INFO, "Loaded: il2cpp_class_get_events");
    *(void**)(&class_get_fields) = dlsym(imagehandle, "il2cpp_class_get_fields");
    log(INFO, "Loaded: il2cpp_class_get_fields");
    *(void**)(&class_get_nested_types) = dlsym(imagehandle, "il2cpp_class_get_nested_types");
    log(INFO, "Loaded: il2cpp_class_get_nested_types");
    *(void**)(&class_get_interfaces) = dlsym(imagehandle, "il2cpp_class_get_interfaces");
    log(INFO, "Loaded: il2cpp_class_get_interfaces");
    *(void**)(&class_get_properties) = dlsym(imagehandle, "il2cpp_class_get_properties");
    log(INFO, "Loaded: il2cpp_class_get_properties");
    *(void**)(&class_get_property_from_name) = dlsym(imagehandle, "il2cpp_class_get_property_from_name");
    log(INFO, "Loaded: il2cpp_class_get_property_from_name");
    *(void**)(&class_get_field_from_name) = dlsym(imagehandle, "il2cpp_class_get_field_from_name");
    log(INFO, "Loaded: il2cpp_class_get_field_from_name");
    *(void**)(&class_get_methods) = dlsym(imagehandle, "il2cpp_class_get_methods");
    log(INFO, "Loaded: il2cpp_class_get_methods");
    *(void**)(&class_get_method_from_name) = dlsym(imagehandle, "il2cpp_class_get_method_from_name");
    log(INFO, "Loaded: il2cpp_class_get_method_from_name");
    *(void**)(&class_get_name) = dlsym(imagehandle, "il2cpp_class_get_name");
    log(INFO, "Loaded: il2cpp_class_get_name");
    #ifdef UNITY_2019
    *(void**)(&type_get_name_chunked) = dlsym(imagehandle, "il2cpp_type_get_name_chunked");
    log(INFO, "Loaded: il2cpp_type_get_name_chunked");
    #endif
    *(void**)(&class_get_namespace) = dlsym(imagehandle, "il2cpp_class_get_namespace");
    log(INFO, "Loaded: il2cpp_class_get_namespace");
    *(void**)(&class_get_parent) = dlsym(imagehandle, "il2cpp_class_get_parent");
    log(INFO, "Loaded: il2cpp_class_get_parent");
    *(void**)(&class_get_declaring_type) = dlsym(imagehandle, "il2cpp_class_get_declaring_type");
    log(INFO, "Loaded: il2cpp_class_get_declaring_type");
    *(void**)(&class_instance_size) = dlsym(imagehandle, "il2cpp_class_instance_size");
    log(INFO, "Loaded: il2cpp_class_instance_size");
    *(void**)(&class_num_fields) = dlsym(imagehandle, "il2cpp_class_num_fields");
    log(INFO, "Loaded: il2cpp_class_num_fields");
    *(void**)(&class_is_valuetype) = dlsym(imagehandle, "il2cpp_class_is_valuetype");
    log(INFO, "Loaded: il2cpp_class_is_valuetype");
    *(void**)(&class_value_size) = dlsym(imagehandle, "il2cpp_class_value_size");
    log(INFO, "Loaded: il2cpp_class_value_size");
    *(void**)(&class_is_blittable) = dlsym(imagehandle, "il2cpp_class_is_blittable");
    log(INFO, "Loaded: il2cpp_class_is_blittable");
    *(void**)(&class_get_flags) = dlsym(imagehandle, "il2cpp_class_get_flags");
    log(INFO, "Loaded: il2cpp_class_get_flags");
    *(void**)(&class_is_abstract) = dlsym(imagehandle, "il2cpp_class_is_abstract");
    log(INFO, "Loaded: il2cpp_class_is_abstract");
    *(void**)(&class_is_interface) = dlsym(imagehandle, "il2cpp_class_is_interface");
    log(INFO, "Loaded: il2cpp_class_is_interface");
    *(void**)(&class_array_element_size) = dlsym(imagehandle, "il2cpp_class_array_element_size");
    log(INFO, "Loaded: il2cpp_class_array_element_size");
    *(void**)(&class_from_type) = dlsym(imagehandle, "il2cpp_class_from_type");
    log(INFO, "Loaded: il2cpp_class_from_type");
    *(void**)(&class_get_type) = dlsym(imagehandle, "il2cpp_class_get_type");
    log(INFO, "Loaded: il2cpp_class_get_type");
    *(void**)(&class_get_type_token) = dlsym(imagehandle, "il2cpp_class_get_type_token");
    log(INFO, "Loaded: il2cpp_class_get_type_token");
    *(void**)(&class_has_attribute) = dlsym(imagehandle, "il2cpp_class_has_attribute");
    log(INFO, "Loaded: il2cpp_class_has_attribute");
    *(void**)(&class_has_references) = dlsym(imagehandle, "il2cpp_class_has_references");
    log(INFO, "Loaded: il2cpp_class_has_references");
    *(void**)(&class_is_enum) = dlsym(imagehandle, "il2cpp_class_is_enum");
    log(INFO, "Loaded: il2cpp_class_is_enum");
    *(void**)(&class_get_image) = dlsym(imagehandle, "il2cpp_class_get_image");
    log(INFO, "Loaded: il2cpp_class_get_image");
    *(void**)(&class_get_assemblyname) = dlsym(imagehandle, "il2cpp_class_get_assemblyname");
    log(INFO, "Loaded: il2cpp_class_get_assemblyname");
    *(void**)(&class_get_rank) = dlsym(imagehandle, "il2cpp_class_get_rank");
    log(INFO, "Loaded: il2cpp_class_get_rank");
    #ifdef UNITY_2019
    *(void**)(&class_get_data_size) = dlsym(imagehandle, "il2cpp_class_get_data_size");
    log(INFO, "Loaded: il2cpp_class_get_data_size");
    *(void**)(&class_get_static_field_data) = dlsym(imagehandle, "il2cpp_class_get_static_field_data");
    log(INFO, "Loaded: il2cpp_class_get_static_field_data");
    #endif
    *(void**)(&class_get_bitmap_size) = dlsym(imagehandle, "il2cpp_class_get_bitmap_size");
    log(INFO, "Loaded: il2cpp_class_get_bitmap_size");
    *(void**)(&class_get_bitmap) = dlsym(imagehandle, "il2cpp_class_get_bitmap");
    log(INFO, "Loaded: il2cpp_class_get_bitmap");
    *(void**)(&stats_dump_to_file) = dlsym(imagehandle, "il2cpp_stats_dump_to_file");
    log(INFO, "Loaded: il2cpp_stats_dump_to_file");
    *(void**)(&stats_get_value) = dlsym(imagehandle, "il2cpp_stats_get_value");
    log(INFO, "Loaded: il2cpp_stats_get_value");
    *(void**)(&domain_get) = dlsym(imagehandle, "il2cpp_domain_get");
    log(INFO, "Loaded: il2cpp_domain_get");
    *(void**)(&domain_assembly_open) = dlsym(imagehandle, "il2cpp_domain_assembly_open");
    log(INFO, "Loaded: il2cpp_domain_assembly_open");
    *(void**)(&domain_get_assemblies) = dlsym(imagehandle, "il2cpp_domain_get_assemblies");
    log(INFO, "Loaded: il2cpp_domain_get_assemblies");
    #ifdef UNITY_2019
    *(void**)(&raise_exception) = dlsym(imagehandle, "il2cpp_raise_exception");
    log(INFO, "Loaded: il2cpp_raise_exception");
    #endif
    *(void**)(&exception_from_name_msg) = dlsym(imagehandle, "il2cpp_exception_from_name_msg");
    log(INFO, "Loaded: il2cpp_exception_from_name_msg");
    *(void**)(&get_exception_argument_null) = dlsym(imagehandle, "il2cpp_get_exception_argument_null");
    log(INFO, "Loaded: il2cpp_get_exception_argument_null");
    *(void**)(&format_exception) = dlsym(imagehandle, "il2cpp_format_exception");
    log(INFO, "Loaded: il2cpp_format_exception");
    *(void**)(&format_stack_trace) = dlsym(imagehandle, "il2cpp_format_stack_trace");
    log(INFO, "Loaded: il2cpp_format_stack_trace");
    *(void**)(&unhandled_exception) = dlsym(imagehandle, "il2cpp_unhandled_exception");
    log(INFO, "Loaded: il2cpp_unhandled_exception");
    *(void**)(&field_get_flags) = dlsym(imagehandle, "il2cpp_field_get_flags");
    log(INFO, "Loaded: il2cpp_field_get_flags");
    *(void**)(&field_get_name) = dlsym(imagehandle, "il2cpp_field_get_name");
    log(INFO, "Loaded: il2cpp_field_get_name");
    *(void**)(&field_get_parent) = dlsym(imagehandle, "il2cpp_field_get_parent");
    log(INFO, "Loaded: il2cpp_field_get_parent");
    *(void**)(&field_get_offset) = dlsym(imagehandle, "il2cpp_field_get_offset");
    log(INFO, "Loaded: il2cpp_field_get_offset");
    *(void**)(&field_get_type) = dlsym(imagehandle, "il2cpp_field_get_type");
    log(INFO, "Loaded: il2cpp_field_get_type");
    *(void**)(&field_get_value) = dlsym(imagehandle, "il2cpp_field_get_value");
    log(INFO, "Loaded: il2cpp_field_get_value");
    *(void**)(&field_get_value_object) = dlsym(imagehandle, "il2cpp_field_get_value_object");
    log(INFO, "Loaded: il2cpp_field_get_value_object");
    *(void**)(&field_has_attribute) = dlsym(imagehandle, "il2cpp_field_has_attribute");
    log(INFO, "Loaded: il2cpp_field_has_attribute");
    *(void**)(&field_set_value) = dlsym(imagehandle, "il2cpp_field_set_value");
    log(INFO, "Loaded: il2cpp_field_set_value");
    *(void**)(&field_static_get_value) = dlsym(imagehandle, "il2cpp_field_static_get_value");
    log(INFO, "Loaded: il2cpp_field_static_get_value");
    *(void**)(&field_static_set_value) = dlsym(imagehandle, "il2cpp_field_static_set_value");
    log(INFO, "Loaded: il2cpp_field_static_set_value");
    *(void**)(&field_set_value_object) = dlsym(imagehandle, "il2cpp_field_set_value_object");
    log(INFO, "Loaded: il2cpp_field_set_value_object");
    #ifdef UNITY_2019
    *(void**)(&field_is_literal) = dlsym(imagehandle, "il2cpp_field_is_literal");
    log(INFO, "Loaded: il2cpp_field_is_literal");
    #endif
    *(void**)(&gc_collect) = dlsym(imagehandle, "il2cpp_gc_collect");
    log(INFO, "Loaded: il2cpp_gc_collect");
    *(void**)(&gc_collect_a_little) = dlsym(imagehandle, "il2cpp_gc_collect_a_little");
    log(INFO, "Loaded: il2cpp_gc_collect_a_little");
    *(void**)(&gc_disable) = dlsym(imagehandle, "il2cpp_gc_disable");
    log(INFO, "Loaded: il2cpp_gc_disable");
    *(void**)(&gc_enable) = dlsym(imagehandle, "il2cpp_gc_enable");
    log(INFO, "Loaded: il2cpp_gc_enable");
    *(void**)(&gc_is_disabled) = dlsym(imagehandle, "il2cpp_gc_is_disabled");
    log(INFO, "Loaded: il2cpp_gc_is_disabled");
    #ifdef UNITY_2019
    *(void**)(&gc_get_max_time_slice_ns) = dlsym(imagehandle, "il2cpp_gc_get_max_time_slice_ns");
    log(INFO, "Loaded: il2cpp_gc_get_max_time_slice_ns");
    *(void**)(&gc_set_max_time_slice_ns) = dlsym(imagehandle, "il2cpp_gc_set_max_time_slice_ns");
    log(INFO, "Loaded: il2cpp_gc_set_max_time_slice_ns");
    *(void**)(&gc_is_incremental) = dlsym(imagehandle, "il2cpp_gc_is_incremental");
    log(INFO, "Loaded: il2cpp_gc_is_incremental");
    #endif
    *(void**)(&gc_get_used_size) = dlsym(imagehandle, "il2cpp_gc_get_used_size");
    log(INFO, "Loaded: il2cpp_gc_get_used_size");
    *(void**)(&gc_get_heap_size) = dlsym(imagehandle, "il2cpp_gc_get_heap_size");
    log(INFO, "Loaded: il2cpp_gc_get_heap_size");
    *(void**)(&gc_wbarrier_set_field) = dlsym(imagehandle, "il2cpp_gc_wbarrier_set_field");
    log(INFO, "Loaded: il2cpp_gc_wbarrier_set_field");
    #ifdef UNITY_2019
    *(void**)(&gc_has_strict_wbarriers) = dlsym(imagehandle, "il2cpp_gc_has_strict_wbarriers");
    log(INFO, "Loaded: il2cpp_gc_has_strict_wbarriers");
    *(void**)(&gc_set_external_allocation_tracker) = dlsym(imagehandle, "il2cpp_gc_set_external_allocation_tracker");
    log(INFO, "Loaded: il2cpp_gc_set_external_allocation_tracker");
    *(void**)(&gc_set_external_wbarrier_tracker) = dlsym(imagehandle, "il2cpp_gc_set_external_wbarrier_tracker");
    log(INFO, "Loaded: il2cpp_gc_set_external_wbarrier_tracker");
    *(void**)(&gc_foreach_heap) = dlsym(imagehandle, "il2cpp_gc_foreach_heap");
    log(INFO, "Loaded: il2cpp_gc_foreach_heap");
    *(void**)(&stop_gc_world) = dlsym(imagehandle, "il2cpp_stop_gc_world");
    log(INFO, "Loaded: il2cpp_stop_gc_world");
    *(void**)(&start_gc_world) = dlsym(imagehandle, "il2cpp_start_gc_world");
    log(INFO, "Loaded: il2cpp_start_gc_world");
    #endif
    *(void**)(&gchandle_new) = dlsym(imagehandle, "il2cpp_gchandle_new");
    log(INFO, "Loaded: il2cpp_gchandle_new");
    *(void**)(&gchandle_new_weakref) = dlsym(imagehandle, "il2cpp_gchandle_new_weakref");
    log(INFO, "Loaded: il2cpp_gchandle_new_weakref");
    *(void**)(&gchandle_get_target) = dlsym(imagehandle, "il2cpp_gchandle_get_target");
    log(INFO, "Loaded: il2cpp_gchandle_get_target");
    *(void**)(&gchandle_free) = dlsym(imagehandle, "il2cpp_gchandle_free");
    log(INFO, "Loaded: il2cpp_gchandle_free");
    #ifdef UNITY_2019
    *(void**)(&gchandle_foreach_get_target) = dlsym(imagehandle, "il2cpp_gchandle_foreach_get_target");
    log(INFO, "Loaded: il2cpp_gchandle_foreach_get_target");
    *(void**)(&object_header_size) = dlsym(imagehandle, "il2cpp_object_header_size");
    log(INFO, "Loaded: il2cpp_object_header_size");
    *(void**)(&array_object_header_size) = dlsym(imagehandle, "il2cpp_array_object_header_size");
    log(INFO, "Loaded: il2cpp_array_object_header_size");
    *(void**)(&offset_of_array_length_in_array_object_header) = dlsym(imagehandle, "il2cpp_offset_of_array_length_in_array_object_header");
    log(INFO, "Loaded: il2cpp_offset_of_array_length_in_array_object_header");
    *(void**)(&offset_of_array_bounds_in_array_object_header) = dlsym(imagehandle, "il2cpp_offset_of_array_bounds_in_array_object_header");
    log(INFO, "Loaded: il2cpp_offset_of_array_bounds_in_array_object_header");
    *(void**)(&allocation_granularity) = dlsym(imagehandle, "il2cpp_allocation_granularity");
    log(INFO, "Loaded: il2cpp_allocation_granularity");
    #endif
    *(void**)(&unity_liveness_calculation_begin) = dlsym(imagehandle, "il2cpp_unity_liveness_calculation_begin");
    log(INFO, "Loaded: il2cpp_unity_liveness_calculation_begin");
    *(void**)(&unity_liveness_calculation_end) = dlsym(imagehandle, "il2cpp_unity_liveness_calculation_end");
    log(INFO, "Loaded: il2cpp_unity_liveness_calculation_end");
    *(void**)(&unity_liveness_calculation_from_root) = dlsym(imagehandle, "il2cpp_unity_liveness_calculation_from_root");
    log(INFO, "Loaded: il2cpp_unity_liveness_calculation_from_root");
    *(void**)(&unity_liveness_calculation_from_statics) = dlsym(imagehandle, "il2cpp_unity_liveness_calculation_from_statics");
    log(INFO, "Loaded: il2cpp_unity_liveness_calculation_from_statics");
    *(void**)(&method_get_return_type) = dlsym(imagehandle, "il2cpp_method_get_return_type");
    log(INFO, "Loaded: il2cpp_method_get_return_type");
    *(void**)(&method_get_declaring_type) = dlsym(imagehandle, "il2cpp_method_get_declaring_type");
    log(INFO, "Loaded: il2cpp_method_get_declaring_type");
    *(void**)(&method_get_name) = dlsym(imagehandle, "il2cpp_method_get_name");
    log(INFO, "Loaded: il2cpp_method_get_name");
    *(void**)(&method_get_from_reflection) = dlsym(imagehandle, "il2cpp_method_get_from_reflection");
    log(INFO, "Loaded: il2cpp_method_get_from_reflection");
    *(void**)(&method_get_object) = dlsym(imagehandle, "il2cpp_method_get_object");
    log(INFO, "Loaded: il2cpp_method_get_object");
    *(void**)(&method_is_generic) = dlsym(imagehandle, "il2cpp_method_is_generic");
    log(INFO, "Loaded: il2cpp_method_is_generic");
    *(void**)(&method_is_inflated) = dlsym(imagehandle, "il2cpp_method_is_inflated");
    log(INFO, "Loaded: il2cpp_method_is_inflated");
    *(void**)(&method_is_instance) = dlsym(imagehandle, "il2cpp_method_is_instance");
    log(INFO, "Loaded: il2cpp_method_is_instance");
    *(void**)(&method_get_param_count) = dlsym(imagehandle, "il2cpp_method_get_param_count");
    log(INFO, "Loaded: il2cpp_method_get_param_count");
    *(void**)(&method_get_param) = dlsym(imagehandle, "il2cpp_method_get_param");
    log(INFO, "Loaded: il2cpp_method_get_param");
    *(void**)(&method_get_class) = dlsym(imagehandle, "il2cpp_method_get_class");
    log(INFO, "Loaded: il2cpp_method_get_class");
    *(void**)(&method_has_attribute) = dlsym(imagehandle, "il2cpp_method_has_attribute");
    log(INFO, "Loaded: il2cpp_method_has_attribute");
    *(void**)(&method_get_flags) = dlsym(imagehandle, "il2cpp_method_get_flags");
    log(INFO, "Loaded: il2cpp_method_get_flags");
    *(void**)(&method_get_token) = dlsym(imagehandle, "il2cpp_method_get_token");
    log(INFO, "Loaded: il2cpp_method_get_token");
    *(void**)(&method_get_param_name) = dlsym(imagehandle, "il2cpp_method_get_param_name");
    log(INFO, "Loaded: il2cpp_method_get_param_name");
    *(void**)(&profiler_install) = dlsym(imagehandle, "il2cpp_profiler_install");
    log(INFO, "Loaded: il2cpp_profiler_install");
    *(void**)(&profiler_set_events) = dlsym(imagehandle, "il2cpp_profiler_set_events");
    log(INFO, "Loaded: il2cpp_profiler_set_events");
    *(void**)(&profiler_install_enter_leave) = dlsym(imagehandle, "il2cpp_profiler_install_enter_leave");
    log(INFO, "Loaded: il2cpp_profiler_install_enter_leave");
    *(void**)(&profiler_install_allocation) = dlsym(imagehandle, "il2cpp_profiler_install_allocation");
    log(INFO, "Loaded: il2cpp_profiler_install_allocation");
    *(void**)(&profiler_install_gc) = dlsym(imagehandle, "il2cpp_profiler_install_gc");
    log(INFO, "Loaded: il2cpp_profiler_install_gc");
    *(void**)(&profiler_install_fileio) = dlsym(imagehandle, "il2cpp_profiler_install_fileio");
    log(INFO, "Loaded: il2cpp_profiler_install_fileio");
    *(void**)(&profiler_install_thread) = dlsym(imagehandle, "il2cpp_profiler_install_thread");
    log(INFO, "Loaded: il2cpp_profiler_install_thread");
    *(void**)(&property_get_flags) = dlsym(imagehandle, "il2cpp_property_get_flags");
    log(INFO, "Loaded: il2cpp_property_get_flags");
    *(void**)(&property_get_get_method) = dlsym(imagehandle, "il2cpp_property_get_get_method");
    log(INFO, "Loaded: il2cpp_property_get_get_method");
    *(void**)(&property_get_set_method) = dlsym(imagehandle, "il2cpp_property_get_set_method");
    log(INFO, "Loaded: il2cpp_property_get_set_method");
    *(void**)(&property_get_name) = dlsym(imagehandle, "il2cpp_property_get_name");
    log(INFO, "Loaded: il2cpp_property_get_name");
    *(void**)(&property_get_parent) = dlsym(imagehandle, "il2cpp_property_get_parent");
    log(INFO, "Loaded: il2cpp_property_get_parent");
    *(void**)(&object_get_class) = dlsym(imagehandle, "il2cpp_object_get_class");
    log(INFO, "Loaded: il2cpp_object_get_class");
    *(void**)(&object_get_size) = dlsym(imagehandle, "il2cpp_object_get_size");
    log(INFO, "Loaded: il2cpp_object_get_size");
    *(void**)(&object_get_virtual_method) = dlsym(imagehandle, "il2cpp_object_get_virtual_method");
    log(INFO, "Loaded: il2cpp_object_get_virtual_method");
    *(void**)(&object_new) = dlsym(imagehandle, "il2cpp_object_new");
    log(INFO, "Loaded: il2cpp_object_new");
    *(void**)(&object_unbox) = dlsym(imagehandle, "il2cpp_object_unbox");
    log(INFO, "Loaded: il2cpp_object_unbox");
    *(void**)(&value_box) = dlsym(imagehandle, "il2cpp_value_box");
    log(INFO, "Loaded: il2cpp_value_box");
    *(void**)(&monitor_enter) = dlsym(imagehandle, "il2cpp_monitor_enter");
    log(INFO, "Loaded: il2cpp_monitor_enter");
    *(void**)(&monitor_try_enter) = dlsym(imagehandle, "il2cpp_monitor_try_enter");
    log(INFO, "Loaded: il2cpp_monitor_try_enter");
    *(void**)(&monitor_exit) = dlsym(imagehandle, "il2cpp_monitor_exit");
    log(INFO, "Loaded: il2cpp_monitor_exit");
    *(void**)(&monitor_pulse) = dlsym(imagehandle, "il2cpp_monitor_pulse");
    log(INFO, "Loaded: il2cpp_monitor_pulse");
    *(void**)(&monitor_pulse_all) = dlsym(imagehandle, "il2cpp_monitor_pulse_all");
    log(INFO, "Loaded: il2cpp_monitor_pulse_all");
    *(void**)(&monitor_wait) = dlsym(imagehandle, "il2cpp_monitor_wait");
    log(INFO, "Loaded: il2cpp_monitor_wait");
    *(void**)(&monitor_try_wait) = dlsym(imagehandle, "il2cpp_monitor_try_wait");
    log(INFO, "Loaded: il2cpp_monitor_try_wait");
    *(void**)(&runtime_invoke) = dlsym(imagehandle, "il2cpp_runtime_invoke");
    log(INFO, "Loaded: il2cpp_runtime_invoke");
    *(void**)(&runtime_invoke_convert_args) = dlsym(imagehandle, "il2cpp_runtime_invoke_convert_args");
    log(INFO, "Loaded: il2cpp_runtime_invoke_convert_args");
    *(void**)(&runtime_class_init) = dlsym(imagehandle, "il2cpp_runtime_class_init");
    log(INFO, "Loaded: il2cpp_runtime_class_init");
    *(void**)(&runtime_object_init) = dlsym(imagehandle, "il2cpp_runtime_object_init");
    log(INFO, "Loaded: il2cpp_runtime_object_init");
    *(void**)(&runtime_object_init_exception) = dlsym(imagehandle, "il2cpp_runtime_object_init_exception");
    log(INFO, "Loaded: il2cpp_runtime_object_init_exception");
    *(void**)(&runtime_unhandled_exception_policy_set) = dlsym(imagehandle, "il2cpp_runtime_unhandled_exception_policy_set");
    log(INFO, "Loaded: il2cpp_runtime_unhandled_exception_policy_set");
    *(void**)(&string_length) = dlsym(imagehandle, "il2cpp_string_length");
    log(INFO, "Loaded: il2cpp_string_length");
    *(void**)(&string_chars) = dlsym(imagehandle, "il2cpp_string_chars");
    log(INFO, "Loaded: il2cpp_string_chars");
    *(void**)(&string_new) = dlsym(imagehandle, "il2cpp_string_new");
    log(INFO, "Loaded: il2cpp_string_new");
    *(void**)(&string_new_len) = dlsym(imagehandle, "il2cpp_string_new_len");
    log(INFO, "Loaded: il2cpp_string_new_len");
    *(void**)(&string_new_utf16) = dlsym(imagehandle, "il2cpp_string_new_utf16");
    log(INFO, "Loaded: il2cpp_string_new_utf16");
    *(void**)(&string_new_wrapper) = dlsym(imagehandle, "il2cpp_string_new_wrapper");
    log(INFO, "Loaded: il2cpp_string_new_wrapper");
    *(void**)(&string_intern) = dlsym(imagehandle, "il2cpp_string_intern");
    log(INFO, "Loaded: il2cpp_string_intern");
    *(void**)(&string_is_interned) = dlsym(imagehandle, "il2cpp_string_is_interned");
    log(INFO, "Loaded: il2cpp_string_is_interned");
    *(void**)(&thread_current) = dlsym(imagehandle, "il2cpp_thread_current");
    log(INFO, "Loaded: il2cpp_thread_current");
    *(void**)(&thread_attach) = dlsym(imagehandle, "il2cpp_thread_attach");
    log(INFO, "Loaded: il2cpp_thread_attach");
    *(void**)(&thread_detach) = dlsym(imagehandle, "il2cpp_thread_detach");
    log(INFO, "Loaded: il2cpp_thread_detach");
    *(void**)(&thread_get_all_attached_threads) = dlsym(imagehandle, "il2cpp_thread_get_all_attached_threads");
    log(INFO, "Loaded: il2cpp_thread_get_all_attached_threads");
    *(void**)(&is_vm_thread) = dlsym(imagehandle, "il2cpp_is_vm_thread");
    log(INFO, "Loaded: il2cpp_is_vm_thread");
    *(void**)(&current_thread_walk_frame_stack) = dlsym(imagehandle, "il2cpp_current_thread_walk_frame_stack");
    log(INFO, "Loaded: il2cpp_current_thread_walk_frame_stack");
    *(void**)(&thread_walk_frame_stack) = dlsym(imagehandle, "il2cpp_thread_walk_frame_stack");
    log(INFO, "Loaded: il2cpp_thread_walk_frame_stack");
    *(void**)(&current_thread_get_top_frame) = dlsym(imagehandle, "il2cpp_current_thread_get_top_frame");
    log(INFO, "Loaded: il2cpp_current_thread_get_top_frame");
    *(void**)(&thread_get_top_frame) = dlsym(imagehandle, "il2cpp_thread_get_top_frame");
    log(INFO, "Loaded: il2cpp_thread_get_top_frame");
    *(void**)(&current_thread_get_frame_at) = dlsym(imagehandle, "il2cpp_current_thread_get_frame_at");
    log(INFO, "Loaded: il2cpp_current_thread_get_frame_at");
    *(void**)(&thread_get_frame_at) = dlsym(imagehandle, "il2cpp_thread_get_frame_at");
    log(INFO, "Loaded: il2cpp_thread_get_frame_at");
    *(void**)(&current_thread_get_stack_depth) = dlsym(imagehandle, "il2cpp_current_thread_get_stack_depth");
    log(INFO, "Loaded: il2cpp_current_thread_get_stack_depth");
    *(void**)(&thread_get_stack_depth) = dlsym(imagehandle, "il2cpp_thread_get_stack_depth");
    log(INFO, "Loaded: il2cpp_thread_get_stack_depth");
    #ifdef UNITY_2019
    *(void**)(&override_stack_backtrace) = dlsym(imagehandle, "il2cpp_override_stack_backtrace");
    log(INFO, "Loaded: il2cpp_override_stack_backtrace");
    #endif
    *(void**)(&type_get_object) = dlsym(imagehandle, "il2cpp_type_get_object");
    log(INFO, "Loaded: il2cpp_type_get_object");
    *(void**)(&type_get_type) = dlsym(imagehandle, "il2cpp_type_get_type");
    log(INFO, "Loaded: il2cpp_type_get_type");
    *(void**)(&type_get_class_or_element_class) = dlsym(imagehandle, "il2cpp_type_get_class_or_element_class");
    log(INFO, "Loaded: il2cpp_type_get_class_or_element_class");
    *(void**)(&type_get_name) = dlsym(imagehandle, "il2cpp_type_get_name");
    log(INFO, "Loaded: il2cpp_type_get_name");
    *(void**)(&type_is_byref) = dlsym(imagehandle, "il2cpp_type_is_byref");
    log(INFO, "Loaded: il2cpp_type_is_byref");
    *(void**)(&type_get_attrs) = dlsym(imagehandle, "il2cpp_type_get_attrs");
    log(INFO, "Loaded: il2cpp_type_get_attrs");
    *(void**)(&type_equals) = dlsym(imagehandle, "il2cpp_type_equals");
    log(INFO, "Loaded: il2cpp_type_equals");
    *(void**)(&type_get_assembly_qualified_name) = dlsym(imagehandle, "il2cpp_type_get_assembly_qualified_name");
    log(INFO, "Loaded: il2cpp_type_get_assembly_qualified_name");
    #ifdef UNITY_2019
    *(void**)(&type_is_static) = dlsym(imagehandle, "il2cpp_type_is_static");
    log(INFO, "Loaded: il2cpp_type_is_static");
    *(void**)(&type_is_pointer_type) = dlsym(imagehandle, "il2cpp_type_is_pointer_type");
    log(INFO, "Loaded: il2cpp_type_is_pointer_type");
    #endif
    *(void**)(&image_get_assembly) = dlsym(imagehandle, "il2cpp_image_get_assembly");
    log(INFO, "Loaded: il2cpp_image_get_assembly");
    *(void**)(&image_get_name) = dlsym(imagehandle, "il2cpp_image_get_name");
    log(INFO, "Loaded: il2cpp_image_get_name");
    *(void**)(&image_get_filename) = dlsym(imagehandle, "il2cpp_image_get_filename");
    log(INFO, "Loaded: il2cpp_image_get_filename");
    *(void**)(&image_get_entry_point) = dlsym(imagehandle, "il2cpp_image_get_entry_point");
    log(INFO, "Loaded: il2cpp_image_get_entry_point");
    *(void**)(&image_get_class_count) = dlsym(imagehandle, "il2cpp_image_get_class_count");
    log(INFO, "Loaded: il2cpp_image_get_class_count");
    *(void**)(&image_get_class) = dlsym(imagehandle, "il2cpp_image_get_class");
    log(INFO, "Loaded: il2cpp_image_get_class");
    *(void**)(&capture_memory_snapshot) = dlsym(imagehandle, "il2cpp_capture_memory_snapshot");
    log(INFO, "Loaded: il2cpp_capture_memory_snapshot");
    *(void**)(&free_captured_memory_snapshot) = dlsym(imagehandle, "il2cpp_free_captured_memory_snapshot");
    log(INFO, "Loaded: il2cpp_free_captured_memory_snapshot");
    *(void**)(&set_find_plugin_callback) = dlsym(imagehandle, "il2cpp_set_find_plugin_callback");
    log(INFO, "Loaded: il2cpp_set_find_plugin_callback");
    *(void**)(&register_log_callback) = dlsym(imagehandle, "il2cpp_register_log_callback");
    log(INFO, "Loaded: il2cpp_register_log_callback");
    *(void**)(&debugger_set_agent_options) = dlsym(imagehandle, "il2cpp_debugger_set_agent_options");
    log(INFO, "Loaded: il2cpp_debugger_set_agent_options");
    *(void**)(&is_debugger_attached) = dlsym(imagehandle, "il2cpp_is_debugger_attached");
    log(INFO, "Loaded: il2cpp_is_debugger_attached");
    #ifdef UNITY_2019
    *(void**)(&register_debugger_agent_transport) = dlsym(imagehandle, "il2cpp_register_debugger_agent_transport");
    log(INFO, "Loaded: il2cpp_register_debugger_agent_transport");
    *(void**)(&debug_get_method_info) = dlsym(imagehandle, "il2cpp_debug_get_method_info");
    log(INFO, "Loaded: il2cpp_debug_get_method_info");
    #endif
    *(void**)(&unity_install_unitytls_interface) = dlsym(imagehandle, "il2cpp_unity_install_unitytls_interface");
    log(INFO, "Loaded: il2cpp_unity_install_unitytls_interface");
    *(void**)(&custom_attrs_from_class) = dlsym(imagehandle, "il2cpp_custom_attrs_from_class");
    log(INFO, "Loaded: il2cpp_custom_attrs_from_class");
    *(void**)(&custom_attrs_from_method) = dlsym(imagehandle, "il2cpp_custom_attrs_from_method");
    log(INFO, "Loaded: il2cpp_custom_attrs_from_method");
    *(void**)(&custom_attrs_get_attr) = dlsym(imagehandle, "il2cpp_custom_attrs_get_attr");
    log(INFO, "Loaded: il2cpp_custom_attrs_get_attr");
    *(void**)(&custom_attrs_has_attr) = dlsym(imagehandle, "il2cpp_custom_attrs_has_attr");
    log(INFO, "Loaded: il2cpp_custom_attrs_has_attr");
    *(void**)(&custom_attrs_construct) = dlsym(imagehandle, "il2cpp_custom_attrs_construct");
    log(INFO, "Loaded: il2cpp_custom_attrs_construct");
    *(void**)(&custom_attrs_free) = dlsym(imagehandle, "il2cpp_custom_attrs_free");
    log(INFO, "Loaded: il2cpp_custom_attrs_free");
    #ifdef UNITY_2019
    *(void**)(&class_set_userdata) = dlsym(imagehandle, "il2cpp_class_set_userdata");
    log(INFO, "Loaded: il2cpp_class_set_userdata");
    *(void**)(&class_get_userdata_offset) = dlsym(imagehandle, "il2cpp_class_get_userdata_offset");
    log(INFO, "Loaded: il2cpp_class_get_userdata_offset");
    #endif

    // MANUALLY DEFINED CONST DEFINITIONS
    *(void**)(&class_get_type_const) = dlsym(imagehandle, "il2cpp_class_get_type");
    log(INFO, "Loaded: il2cpp_class_get_type CONST VERSION!");
    *(void**)(&il2cpp_functions::class_get_name_const) = dlsym(imagehandle, "il2cpp_class_get_name");
    log(INFO, "Loaded: il2cpp_class_get_name CONST VERSION!");

    const char* err = dlerror();
    if (err) {
        log(CRITICAL, "A dlsym failed! dlerror: %s", err);
    }

    // XREF TRACES
    // Class::Init. 0x846A68 in 1.5, 0x9EC0A4 in 1.7.0, 0xA6D1B8 in 1.8.0b1
    Instruction ans((const int32_t*)array_new_specific);
    Instruction Array_NewSpecific(CRASH_UNLESS(ans.label));
    log(DEBUG, "Array::NewSpecific offset: %lX", ((intptr_t)Array_NewSpecific.addr) - getRealOffset(0));
    auto j2Cl_I = CRASH_UNLESS(Array_NewSpecific.findNthCall(1));  // also the 113th call in Runtime::Init
    Class_Init = (decltype(Class_Init))CRASH_UNLESS(j2Cl_I->label);
    log(DEBUG, "Class::Init found? offset: %lX", ((intptr_t)Class_Init) - getRealOffset(0));
    usleep(1000);  // 0.001s

    // MetadataCache::GetTypeInfoFromTypeIndex. offset 0x84F764 in 1.5, 0x9F5250 in 1.7.0, 0xA7A79C in 1.8.0b1
    Instruction caha((const int32_t*)custom_attrs_has_attr);
    Instruction MetadataCache_HasAttribute(CRASH_UNLESS(caha.label));
    auto j2MC_GTIFTI = CRASH_UNLESS(MetadataCache_HasAttribute.findNthCall(1));
    MetadataCache_GetTypeInfoFromTypeIndex = (decltype(MetadataCache_GetTypeInfoFromTypeIndex))CRASH_UNLESS(j2MC_GTIFTI->label);
    log(DEBUG, "MetadataCache::GetTypeInfoFromTypeIndex found? offset: %lX",
        ((intptr_t)MetadataCache_GetTypeInfoFromTypeIndex) - getRealOffset(0));
    usleep(1000);  // 0.001s

    // MetadataCache::GetTypeInfoFromTypeDefinitionIndex. offset 0x84FBA4 in 1.5, 0x9F5690 in 1.7.0, 0xA75958 in 1.8.0b1
    Instruction tgcoec((const int32_t*)type_get_class_or_element_class);
    Instruction Type_GetClassOrElementClass(CRASH_UNLESS(tgcoec.label));
    auto j2MC_GTIFTDI = CRASH_UNLESS(Type_GetClassOrElementClass.findNthDirectBranchWithoutLink(5));
    MetadataCache_GetTypeInfoFromTypeDefinitionIndex =
        (decltype(MetadataCache_GetTypeInfoFromTypeDefinitionIndex))CRASH_UNLESS(j2MC_GTIFTDI->label);
    log(DEBUG, "MetadataCache::GetTypeInfoFromTypeDefinitionIndex found? offset: %lX",
        ((intptr_t)MetadataCache_GetTypeInfoFromTypeDefinitionIndex) - getRealOffset(0));
    usleep(1000);  // 0.001s

    // Type::GetName. offset 0x8735DC in 1.5, 0xA1A458 in 1.7.0, 0xA7B634 in 1.8.0b1
    Instruction tanq((const int32_t*)type_get_assembly_qualified_name);
    auto j2T_GN = CRASH_UNLESS(tanq.findNthCall(1));
    _Type_GetName_ = (decltype(_Type_GetName_))CRASH_UNLESS(j2T_GN->label);
    log(DEBUG, "Type::GetName found? offset: %lX", ((intptr_t)_Type_GetName_) - getRealOffset(0));
    usleep(1000);  // 0.001s

    // GenericClass::GetClass. offset 0x88DF64 in 1.5, 0xA34F20 in 1.7.0, 0xA6E4EC in 1.8.0b1
    Instruction cfit((const int32_t*)class_from_il2cpp_type);
    Instruction Class_FromIl2CppType_inst(CRASH_UNLESS(cfit.label));
    Class_FromIl2CppType = (decltype(Class_FromIl2CppType))CRASH_UNLESS(Class_FromIl2CppType_inst.addr);
    auto caseStart = CRASH_UNLESS(EvalSwitch(Class_FromIl2CppType_inst.addr, 1, 1, IL2CPP_TYPE_GENERICINST));
    auto j2GC_GC = CRASH_UNLESS(caseStart->findNthDirectBranchWithoutLink(1));
    log(DEBUG, "j2GC_GC: %s", j2GC_GC->toString().c_str());
    GenericClass_GetClass = (decltype(GenericClass_GetClass))CRASH_UNLESS(j2GC_GC->label);
    log(DEBUG, "GenericClass::GetClass found? offset: %lX", ((intptr_t)GenericClass_GetClass) - getRealOffset(0));
    usleep(1000);  // 0.001s

    Instruction iu16((const int32_t*)init_utf16);
    auto j2R_I = CRASH_UNLESS(iu16.findNthCall(3));
    Instruction Runtime_Init(CRASH_UNLESS(j2R_I->label));
    auto ldr = CRASH_UNLESS(Runtime_Init.findNth(1, std::mem_fn(&Instruction::isLoad)));  // the load for the malloc that precedes our adrp
    // alternatively, could just get the 1st ADRP in Runtime::Init with dest reg x20
    il2cpp_functions::defaults = (decltype(il2cpp_functions::defaults))ExtractAddress(ldr->addr, 1, 1);
    log(DEBUG, "il2cpp_defaults found? offset: %lX", ((intptr_t)defaults) - getRealOffset(0));
    usleep(1000);  // 0.001s

    // FIELDS
    // Extract locations of s_GlobalMetadataHeader, s_Il2CppMetadataRegistration, & s_GlobalMetadata
    // TODO: refactor to reduce instruction re-parsing?
    il2cpp_functions::s_GlobalMetadataHeaderPtr = (decltype(il2cpp_functions::s_GlobalMetadataHeaderPtr))CRASH_UNLESS(
        ExtractAddress(il2cpp_functions::MetadataCache_GetTypeInfoFromTypeDefinitionIndex, 3, 1));
    usleep(1000);  // 0.001s
    il2cpp_functions::s_Il2CppMetadataRegistrationPtr = (decltype(il2cpp_functions::s_Il2CppMetadataRegistrationPtr))CRASH_UNLESS(
        ExtractAddress(il2cpp_functions::MetadataCache_GetTypeInfoFromTypeDefinitionIndex, 4, 1));
    usleep(1000);  // 0.001s
    il2cpp_functions::s_GlobalMetadataPtr = (decltype(il2cpp_functions::s_GlobalMetadataPtr))CRASH_UNLESS(
        ExtractAddress(il2cpp_functions::MetadataCache_GetTypeInfoFromTypeDefinitionIndex, 5, 1));
    usleep(1000);  // 0.001s
    log(DEBUG, "All global constants found!");

    #ifdef FILE_LOG
    if (il2cpp_functions::shutdown) {
        Instruction sd((const int32_t*)il2cpp_functions::shutdown);
        if (sd.label) {
            auto Runtime_Shutdown = *(sd.label);
            log(INFO, "hook installing to: %p (offset %lX)", Runtime_Shutdown, ((intptr_t)Runtime_Shutdown) - getRealOffset(0));
            INSTALL_HOOK_DIRECT(shutdown_hook, Runtime_Shutdown);
        } else {
            log(CRITICAL, "Failed to parse il2cpp_shutdown's implementation address! Could not install shutdown hook for closing file logs.");
        }
    } else {
        log(CRITICAL, "Failed to get address of il2cpp_shutdown (see above)! Could not install shutdown hook for closing file logs.");
    }
    #endif

    dlclose(imagehandle);
    initialized = true;
    log(INFO, "il2cpp_functions: Init: Successfully loaded all il2cpp functions!");
    usleep(100);  // 0.0001s
}
