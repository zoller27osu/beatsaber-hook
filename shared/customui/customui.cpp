// To use, simply add customui.cpp to your Android.mk, and #include customui.hpp
// CUSTOM UI

#include "../utils/utils.h"
#include "customui.hpp"
using namespace il2cpp_utils;

int CustomUI::TextObject::counter = 0;
namespace CustomUI
{
    bool CustomUI::TextObject::create()
    {
        // gameobj = new GameObject("CustomTextUI");
        log(DEBUG, "TextObject::create: Creating gameObject: %s (object number: %i)", name.data(), counter);
        gameObj = NewUnsafe(GetClassFromName("UnityEngine", "GameObject"), createcsstr(name.data()));

        // gameObj.SetActive(false);
        bool active = false;
        log(DEBUG, "TextObject::create: Setting gameObject.active to false");
        if (!RunMethod(gameObj, "SetActive", &active))
        {
            log(DEBUG, "TextObject::create: Failed to set active to false");
            return false;
        }

        // gameObj.AddComponent<TextMeshProUGUI>();
        log(DEBUG, "TextObject::create: Getting type of TMPro.TextMeshProUGUI");
        Il2CppObject *type_tmpugui = il2cpp_functions::type_get_object(il2cpp_functions::class_get_type(GetClassFromName("TMPro", "TextMeshProUGUI")));
        
        log(DEBUG, "TextObject::create: Adding component TMPro.TextMeshProUGUI");
        if (!RunMethod(&textMesh, gameObj, "AddComponent", type_tmpugui))
        {
            log(DEBUG, "TextObject::create: Failed to add Component TMPro.TextMeshProUGUI");
            return false;
        }
        // textMesh.font = GameObject.Instantiate(Resources.FindObjectsOfTypeAll<TMP_FontAsset>().First(t => t.name == "Teko-Medium SDF No Glow"));
        log(DEBUG, "TextObject::create: Getting type of TMPro.TMP_FontAsset");
        Il2CppObject *type_fontasset = il2cpp_functions::type_get_object(il2cpp_functions::class_get_type(GetClassFromName("TMPro", "TMP_FontAsset")));
        log(DEBUG, "TextObject::create: Gotten the type!");
        Array<Il2CppObject *> *allObjects;

        // Find Objects of type TMP_fontAsset
        if (!RunMethod(&allObjects, nullptr, FindMethod("UnityEngine", "Resources", "FindObjectsOfTypeAll", 1), type_fontasset))
        {
            // EXCEPTION
            log(DEBUG, "TextObject::create: Failed to Find Objects of type TMP_fontAsset");
            return false;
        }
        int match = -1;
        for (int i = 0; i < allObjects->Length(); i++)
        {
            // Treat it as a UnityEngine.Object (which it is) and call the name getter
            Il2CppString *assetName;
            if (!RunMethod(&assetName, allObjects->values[i], "get_name"))
            {
                // EXCEPTION
                log(DEBUG, "TextObject::create: Failed to run get_name of assetName");
                return false;
            }
            if (strcmp(to_utf8(csstrtostr(assetName)).c_str(), "Teko-Medium SDF No Glow") == 0)
            {
                // Found matching asset
                match = i;
                break;
            }
        }
        if (match == -1)
        {
            log(DEBUG, "TextObject::create: Could not find matching TMP_FontAsset!");
            return false;
        }

        // Instantiating the font
        log(DEBUG, "TextObject::create: Instantiating the font");
        Il2CppObject *font;
        if (!RunMethod(&font, nullptr, FindMethod("UnityEngine", "Object", "Instantiate", 1), allObjects->values[match]))
        {
            log(DEBUG, "TextObject::create: Failed to Instantiate font!");
            return false;
        }

        // Setting the font
        log(DEBUG, "TextObject::create: Setting the font");
        if (!RunMethod(textMesh, "set_font", font))
        {
            log(DEBUG, "TextObject::create: Failed to set font!");
            return false;
        }

        // textMesh.rectTransform.SetParent(parent, false);
        log(DEBUG, "TextObject::create: Getting rectTransform");
        Il2CppObject *rectTransform;
        if (!RunMethod(&rectTransform, textMesh, "get_rectTransform"))
        {
            log(DEBUG, "TextObject::create: Failed to get rectTransform");
            return false;
        }
        
        log(DEBUG, "TextObject::create: Setting Parent");
        bool value = false;
        if (!RunMethod(rectTransform, "SetParent", parentTransform, &value))
        {
            log(DEBUG, "TextObject::create: Failed to set parent!");
            if (parentTransform == nullptr)
            {
                log(DEBUG, "TextObject::create: parentTransform is null! Fix it!");
            }
            return false;
        }
        // textMesh.text = text;
        log(DEBUG, "TextObject::create: Setting Text");
        if (!RunMethod(textMesh, "set_text", createcsstr(text.data())))
        {
            log(DEBUG, "TextObject::create: Failed to set text!");
            return false;
        }

        // textmesh.fontSize = fontSize;
        log(DEBUG, "TextObject::create: Setting fontSize");
        if (!RunMethod(textMesh, "set_fontSize", &fontSize))
        {
            log(DEBUG, "TextObject::create: Failed to set fontSize!");
            return false;
        }

        // textMesh.color = Color.white;
        log(DEBUG, "TextObject::create: Setting color");
        if (!RunMethod(textMesh, "set_color", &color))
        {
            log(DEBUG, "TextObject::create: Failed to set color!");
            return false;
        }
    
        // textMesh.rectTransform.anchorMin = anchorMin
        log(DEBUG, "TextObject::create: Setting anchorMin");
        if (!RunMethod(rectTransform, "set_anchorMin", &anchorMin))
        {
            log(DEBUG, "TextObject::create: Failed to set anchorMin");
            return false;
        }

        // textMesh.rectTransform.anchorMax = anchorMax
        log(DEBUG, "TextObject::create: Setting anchorMax");
        if (!RunMethod(rectTransform, "set_anchorMax", &anchorMax))
        {
            log(DEBUG, "TextObject::create: Failed to set anchorMax");
            return false;
        }
        
        // textMesh.rectTransform.sizeDelta = sizeDelta
        log(DEBUG, "TextObject::create: Setting sizeDelta");
        if (!RunMethod(rectTransform, "set_sizeDelta", &sizeDelta))
        {
            log(DEBUG, "TextObject::create: Failed to set sizeDelta");
            return false;
        }

        // textMesh.rectTransform.anchoredPosition = anchoredPosition
        log(DEBUG, "TextObject::create: Setting anchoredPosition");
        if (!RunMethod(rectTransform, "set_anchoredPosition", &anchoredPosition))
        {
            log(DEBUG, "TextObject::create: failed to set anchoredPosition");
            return false;
        }

        // gameObj.SetActive(true);
        log(DEBUG, "TextObject::create: Setting gameObject active to true");
        active = true;
        if (!RunMethod(gameObj, "SetActive", &active))
        {
            log(DEBUG, "TextObject::create: Failed to set active to true");
            return false;
        }
        log(DEBUG, "TextObject::create: Succesfully created gameObj: %s", name.c_str());
        counter++;
        return true;
    }

} // namespace CustomUI