// To use, simply add customui.cpp to your Android.mk, and #include customui.hpp
// CUSTOM UI

#include "../utils/utils.h"
#include "customui.hpp"

#include <iostream>
#include <sstream>
using namespace il2cpp_utils;

int CustomUI::TextObject::counter = 0;
namespace CustomUI {
    bool TextObject::create() {
        // gameobj = new GameObject("CustomTextUI");
        log(DEBUG, "TextObject::create: Creating gameObject: %s (object number: %i)", name.data(), counter);
        gameObj = New(GetClassFromName("UnityEngine", "GameObject"), createcsstr(name));

        // gameObj.SetActive(false);
        log(DEBUG, "TextObject::create: Setting gameObject.active to false");
        RET_0_UNLESS(RunMethod(gameObj, "SetActive", false));

        // gameObj.AddComponent<TextMeshProUGUI>();
        log(DEBUG, "TextObject::create: Getting type of TMPro.TextMeshProUGUI");
        Il2CppObject *type_tmpugui = GetSystemType("TMPro", "TextMeshProUGUI");

        log(DEBUG, "TextObject::create: Adding component TMPro.TextMeshProUGUI");
        RET_0_UNLESS(RunMethod(&textMesh, gameObj, "AddComponent", type_tmpugui));

        // textMesh.font = GameObject.Instantiate(Resources.FindObjectsOfTypeAll<TMP_FontAsset>().First(t => t.name == "Teko-Medium SDF No Glow"));
        log(DEBUG, "TextObject::create: Getting type of TMPro.TMP_FontAsset");
        Il2CppObject *type_fontasset = GetSystemType("TMPro", "TMP_FontAsset");
        log(DEBUG, "TextObject::create: Gotten the type!");
        Array<Il2CppObject *> *allObjects;

        // Find Objects of type TMP_fontAsset
        RET_0_UNLESS(RunMethod(&allObjects, "UnityEngine", "Resources", "FindObjectsOfTypeAll", type_fontasset));
        int match = -1;
        for (int i = 0; i < allObjects->Length(); i++) {
            // Treat it as a UnityEngine.Object (which it is) and call the name getter
            Il2CppString *assetName;
            RET_0_UNLESS(RunMethod(&assetName, allObjects->values[i], "get_name"));
            if (to_utf8(csstrtostr(assetName)) == "Teko-Medium SDF No Glow") {
                // Found matching asset
                match = i;
                break;
            }
        }
        RET_0_UNLESS(match != -1);

        log(DEBUG, "TextObject::create: Instantiating the font");
        Il2CppObject *font;
        RET_0_UNLESS(RunMethod(&font, "UnityEngine", "Object", "Instantiate", allObjects->values[match]));

        log(DEBUG, "TextObject::create: Setting the font");
        RET_0_UNLESS(RunMethod(textMesh, "set_font", font));

        // textMesh.rectTransform.SetParent(parent, false);
        log(DEBUG, "TextObject::create: Getting rectTransform");
        Il2CppObject *rectTransform;
        RET_0_UNLESS(RunMethod(&rectTransform, textMesh, "get_rectTransform"));

        log(DEBUG, "TextObject::create: Setting Parent");
        if (!parentTransform) {
            log(ERROR, "TextObject::create: parentTransform is null! Fix it!");
        }
        RET_0_UNLESS(RunMethodUnsafe(rectTransform, "SetParent", parentTransform, false));

        // textMesh.text = text;
        log(DEBUG, "TextObject::create: Setting Text");
        RET_0_UNLESS(RunMethod(textMesh, "set_text", createcsstr(text.data())));

        // textmesh.fontSize = fontSize;
        log(DEBUG, "TextObject::create: Setting fontSize");
        RET_0_UNLESS(RunMethod(textMesh, "set_fontSize", fontSize));

        // textMesh.color = Color.white;
        log(DEBUG, "TextObject::create: Setting color");
        RET_0_UNLESS(RunMethod(textMesh, "set_color", color));

        // textMesh.rectTransform.anchorMin = anchorMin
        log(DEBUG, "TextObject::create: Setting anchorMin");
        RET_0_UNLESS(RunMethod(rectTransform, "set_anchorMin", anchorMin));

        // textMesh.rectTransform.anchorMax = anchorMax
        log(DEBUG, "TextObject::create: Setting anchorMax");
        RET_0_UNLESS(RunMethod(rectTransform, "set_anchorMax", anchorMax));

        // textMesh.rectTransform.sizeDelta = sizeDelta
        log(DEBUG, "TextObject::create: Setting sizeDelta");
        RET_0_UNLESS(RunMethod(rectTransform, "set_sizeDelta", sizeDelta));

        // textMesh.rectTransform.anchoredPosition = anchoredPosition
        log(DEBUG, "TextObject::create: Setting anchoredPosition");
        RET_0_UNLESS(RunMethod(rectTransform, "set_anchoredPosition", anchoredPosition));

        // gameObj.SetActive(true);
        log(DEBUG, "TextObject::create: Setting gameObject active to true");
        RET_0_UNLESS(RunMethod(gameObj, "SetActive", true));

        log(DEBUG, "TextObject::create: Succesfully created gameObj: %s", name.c_str());
        counter++;
        return true;
    }

    static auto findDataSize(Il2CppObject* downloadHandler) {
        Array<uint8_t>* data;
        if (il2cpp_utils::RunMethod(&data, "UnityEngine.Networking", "DownloadHandler", "InternalGetByteArray", downloadHandler)) {
            return data->Length();
        }
        return 0;
    }

    void RawImageObject::monitorProgress(RawImageObject* obj) {
        log(INFO, "monitorProgress start");
        Il2CppObject* downloadHandler = il2cpp_utils::GetFieldValue(obj->WWW, "m_DownloadHandler");
        if (!downloadHandler) return;

        bool isDone = false;
        auto prevCap = findDataSize(downloadHandler);
        float prevProg = -1;
        std::vector<decltype(Array<int>::max_length)> recvLens;
        while (!isDone) {
            il2cpp_utils::RunMethod(&isDone, obj->WWW, "get_isDone");
            float prog;
            if (il2cpp_utils::RunMethod(&prog, obj->WWW, "GetDownloadProgress")) {
                if (prog != prevProg) {
                    auto dataLen = findDataSize(downloadHandler);
                    decltype(Array<int>::max_length) cap = std::round(((float)dataLen) / prog);
                    if (cap != prevCap || prog < prevProg) {
                        log(INFO, "prog is %i / %i", dataLen, cap);
                    }
                    log(INFO, "wrAsyncOp progress: %f (raw = %p)", prog, *reinterpret_cast<void**>(&prog));
                    prevProg = prog;
                    recvLens.push_back(dataLen);
                    prevCap = cap;
                }
            }

            std::chrono::nanoseconds nanoTime((long)round(1000000L));  // 0.001s
            std::this_thread::sleep_for(nanoTime);
        }
        std::stringstream ss;
        auto finalLen = findDataSize(downloadHandler);
        for (auto r : recvLens) {
            ss << ((float) r) / finalLen << " ";
        }
        ss << std::endl;
        log(INFO, "real progs (out of %i): %s", finalLen, ss.str().c_str());

        log(INFO, "webRequest isDone.");
        Il2CppString* str;
        if (RunMethod(&str, obj->WWW, "get_error")) {
            if (str) {
                log(ERROR, "webRequest error: %s", to_utf8(csstrtostr(str)).c_str());
            } else {
                log(DEBUG, "webRequest had no error.");
            }
        }
    }

    bool RawImageObject::textureWebRequestComplete(RawImageObject* obj, Il2CppObject* asyncOp) {
        // Notes: The field named "<webRequest>k__BackingField" on asyncOp is a pointer that should match obj->WWW
        log(DEBUG, "Entering textureWebRequestComplete!");

        RET_0_UNLESS(RunMethod(&obj->recievedTexture2D, "UnityEngine.Networking", "DownloadHandlerTexture", "GetContent", obj->WWW));

        obj->gameObj = New(GetClassFromName("UnityEngine", "GameObject"), createcsstr("RandomImage"));
        RET_0_UNLESS(RunMethod(obj->gameObj, "SetActive", false));
        RET_0_UNLESS(RunMethod(&obj->rawImage, obj->gameObj, "AddComponent", GetSystemType("UnityEngine.UI", "RawImage")));
        RET_0_UNLESS(RunMethod(obj->rawImage, "set_texture", obj->recievedTexture2D));

        Il2CppObject *rawImageRectTransform;
        RET_0_UNLESS(RunMethod(&rawImageRectTransform, obj->rawImage, "get_rectTransform"));
        if (!obj->parentTransform) {
            log(ERROR, "RawImageObject::textureWebRequestComplete: obj->parentTransform is null! Fix it!");
        }
        RET_0_UNLESS(RunMethodUnsafe(rawImageRectTransform, "SetParent", obj->parentTransform, false));

        // textMesh.rectTransform.anchorMin = anchorMin
        log(DEBUG, "RawImageObject::textureWebRequestComplete: Setting anchorMin");
        RET_0_UNLESS(RunMethod(rawImageRectTransform, "set_anchorMin", obj->anchorMin));

        // textMesh.rectTransform.anchorMax = anchorMax
        log(DEBUG, "RawImageObject::textureWebRequestComplete: Setting anchorMax");
        RET_0_UNLESS(RunMethod(rawImageRectTransform, "set_anchorMax", obj->anchorMax));

        // textMesh.rectTransform.sizeDelta = sizeDelta
        log(DEBUG, "RawImageObject::textureWebRequestComplete: Setting sizeDelta");
        RET_0_UNLESS(RunMethod(rawImageRectTransform, "set_sizeDelta", obj->sizeDelta));

        // textMesh.rectTransform.anchoredPosition = anchoredPosition
        log(DEBUG, "RawImageObject::textureWebRequestComplete: Setting anchoredPosition");
        RET_0_UNLESS(RunMethod(rawImageRectTransform, "set_anchoredPosition", obj->anchoredPosition));
        RET_0_UNLESS(RunMethod(obj->gameObj, "SetActive", true));

        log(DEBUG, "Callback success!");
        // TODO: if debug, play a sound?
        return true;
    }

    bool RawImageObject::create() {
        RET_0_UNLESS(RunMethod(&WWW, "UnityEngine.Networking", "UnityWebRequestTexture", "GetTexture", createcsstr(url)));

        // If only we could use UnityEngine.WWW and its WaitUntilDoneIfPossible() :(
        auto method = RET_0_UNLESS(FindMethodUnsafe("UnityEngine.Networking", "UnityWebRequestAsyncOperation", "add_completed", 1));
        auto fieldType = il2cpp_functions::method_get_param(method, 0);

        RET_0_UNLESS(RunMethod(&sendWebRequestObj, WWW, "SendWebRequest"));

        auto action = RET_0_UNLESS(MakeAction(this, textureWebRequestComplete, fieldType));
        RET_0_UNLESS(RunMethod(sendWebRequestObj, method, action));

        // Uncomment this to watch the progress for debugging purposes (backtracks a lot):
        // StartCoroutine(monitorProgress, this);

        return true;
    }
} // namespace CustomUI
