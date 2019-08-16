#ifndef TMPro_TMP_FontAsset_DEFINED
#define TMPro_TMP_FontAsset_DEFINED
// This .h file was compiled via beatsaber-hook/shared/helper.py's Parse Mode.
// Created by Sc2ad.
// Methods may not be valid!
#include <dlfcn.h>
#include <string_view>
#include "../utils/typedefs.h"
#include "../utils/il2cpp-functions.h"
#include "../utils/il2cpp-utils.h"
// Contains MethodInfo/Il2CppClass data for: TMPro.TMP_FontAsset
namespace TMPro_TMP_FontAsset {
    // TMPro.TMP_FontAsset
    typedef struct Class {
        static void* s_defaultFontAsset; // 0x0
        void* fontAssetType; // 0x18
        void* m_fontInfo; // 0x1C
        void* atlas; // 0x20
        void* m_glyphInfoList; // 0x24
        void* m_characterDictionary; // 0x28
        void* m_kerningDictionary; // 0x2C
        void* m_kerningInfo; // 0x30
        void* m_kerningPair; // 0x34
        void* fallbackFontAssets; // 0x38
        void* m_CreationSettings; // 0x3C
        void* fontWeights; // 0x7C
        void* m_characterSet; // 0x80
        float normalStyle; // 0x84
        float normalSpacingOffset; // 0x88
        float boldStyle; // 0x8C
        float boldSpacing; // 0x90
        char italicStyle; // 0x94
        char tabSize; // 0x95
        char m_oldTabSize; // 0x96
    } Class;
    static bool __cached = false;
    static Il2CppClass* klass;
    static const MethodInfo* get_defaultFontAsset;
    static const MethodInfo* get_fontInfo;
    static const MethodInfo* get_characterDictionary;
    static const MethodInfo* get_kerningDictionary;
    static const MethodInfo* get_kerningInfo;
    static const MethodInfo* get_creationSettings;
    static const MethodInfo* set_creationSettings;
    static const MethodInfo* OnEnable;
    static const MethodInfo* OnDisable;
    static const MethodInfo* AddFaceInfo_FaceInfo;
    static const MethodInfo* AddGlyphInfo_TMP_Glyph_arr;
    static const MethodInfo* AddKerningInfo_KerningTable;
    static const MethodInfo* ReadFontDefinition;
    static const MethodInfo* SortGlyphs;
    static const MethodInfo* HasCharacter_int;
    static const MethodInfo* HasCharacter_char;
    static const MethodInfo* HasCharacter_char_bool;
    static const MethodInfo* HasCharacter_Internal_char_bool;
    static const MethodInfo* HasCharacters_string_out_List1;
    static const MethodInfo* HasCharacters_string;
    static const MethodInfo* GetCharacters_TMP_FontAsset;
    static const MethodInfo* GetCharactersArray_TMP_FontAsset;
    // The Initialization function that must be called before using any of these definitions
    static void Init() {
        if (!__cached) {
            klass = il2cpp_utils::GetClassFromName("TMPro", "TMP_FontAsset");
            get_defaultFontAsset = il2cpp_functions::class_get_method_from_name(klass, "get_defaultFontAsset", 0);
            get_fontInfo = il2cpp_functions::class_get_method_from_name(klass, "get_fontInfo", 0);
            get_characterDictionary = il2cpp_functions::class_get_method_from_name(klass, "get_characterDictionary", 0);
            get_kerningDictionary = il2cpp_functions::class_get_method_from_name(klass, "get_kerningDictionary", 0);
            get_kerningInfo = il2cpp_functions::class_get_method_from_name(klass, "get_kerningInfo", 0);
            get_creationSettings = il2cpp_functions::class_get_method_from_name(klass, "get_creationSettings", 0);
            set_creationSettings = il2cpp_functions::class_get_method_from_name(klass, "set_creationSettings", 1);
            OnEnable = il2cpp_functions::class_get_method_from_name(klass, "OnEnable", 0);
            OnDisable = il2cpp_functions::class_get_method_from_name(klass, "OnDisable", 0);
            AddFaceInfo_FaceInfo = il2cpp_functions::class_get_method_from_name(klass, "AddFaceInfo", 1);
            AddGlyphInfo_TMP_Glyph_arr = il2cpp_functions::class_get_method_from_name(klass, "AddGlyphInfo", 1);
            AddKerningInfo_KerningTable = il2cpp_functions::class_get_method_from_name(klass, "AddKerningInfo", 1);
            ReadFontDefinition = il2cpp_functions::class_get_method_from_name(klass, "ReadFontDefinition", 0);
            SortGlyphs = il2cpp_functions::class_get_method_from_name(klass, "SortGlyphs", 0);
            HasCharacter_int = il2cpp_functions::class_get_method_from_name(klass, "HasCharacter", 1);
            HasCharacter_char = il2cpp_functions::class_get_method_from_name(klass, "HasCharacter", 1);
            HasCharacter_char_bool = il2cpp_functions::class_get_method_from_name(klass, "HasCharacter", 2);
            HasCharacter_Internal_char_bool = il2cpp_functions::class_get_method_from_name(klass, "HasCharacter_Internal", 2);
            HasCharacters_string_out_List1 = il2cpp_functions::class_get_method_from_name(klass, "HasCharacters", 3);
            HasCharacters_string = il2cpp_functions::class_get_method_from_name(klass, "HasCharacters", 1);
            GetCharacters_TMP_FontAsset = il2cpp_functions::class_get_method_from_name(klass, "GetCharacters", 1);
            GetCharactersArray_TMP_FontAsset = il2cpp_functions::class_get_method_from_name(klass, "GetCharactersArray", 1);
            __cached = true;
        }
    }
}
#endif /* TMPro_TMP_FontAsset_DEFINED */