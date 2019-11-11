#pragma once

namespace mod
{
    // init_t is called in modloader::preload
    using init_t = void() noexcept;
    // constructor_t is called in modloader::load
    using lib_main_t = void();
    // preload_t is called in modloader::accept_unity_handle
    using preload_t = void() noexcept;
    // load_t is called in il2cpp_init
    using load_t = void() noexcept;
} // namespace mod

#define MOD_FUNCTION_ASSERT(funcname) \
    ::std::is_same_v<std::remove_reference_t<decltype(&funcname)>, ::mod::funcname##_t*>

#define CHECK_MOD_FUNCTION(funcname) \
    static_assert(MOD_FUNCTION_ASSERT(funcname), \
    #funcname " either has the wrong signature, or does not exist!")

#define CHECK_MOD_INIT CHECK_MOD_FUNCTION(init)
#define CHECK_MOD_CTOR CHECK_MOD_FUNCTION(lib_main)
#define CHECK_MOD_PRELOAD CHECK_MOD_FUNCTION(preload)
#define CHECK_MOD_LOAD CHECK_MOD_FUNCTION(load)