// To use, simply add customui.cpp to your Android.mk, and #include customui.hpp
// CUSTOM UI

#include "../utils/utils.h"
#include "customui.hpp"
#include "../utils/il2cpp-utils.hpp"

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
        auto *type_tmpugui = GetSystemType("TMPro", "TextMeshProUGUI");

        log(DEBUG, "TextObject::create: Adding component TMPro.TextMeshProUGUI");
        textMesh = RET_0_UNLESS(RunMethod(gameObj, "AddComponent", type_tmpugui));

        // textMesh.font = GameObject.Instantiate(Resources.FindObjectsOfTypeAll<TMP_FontAsset>().First(t => t.name == "Teko-Medium SDF No Glow"));
        log(DEBUG, "TextObject::create: Getting type of TMPro.TMP_FontAsset");
        auto *type_fontasset = GetSystemType("TMPro", "TMP_FontAsset");
        log(DEBUG, "TextObject::create: Gotten the type!");

        // Find Objects of type TMP_fontAsset
        auto* allObjects = RET_0_UNLESS(RunMethod<Array<Il2CppObject*>*>("UnityEngine", "Resources", "FindObjectsOfTypeAll", type_fontasset));
        int match = -1;
        for (int i = 0; i < allObjects->Length(); i++) {
            // Treat it as a UnityEngine.Object (which it is) and call the name getter
            auto* assetName = RET_0_UNLESS(RunMethod<Il2CppString*>(allObjects->values[i], "get_name"));
            if (to_utf8(csstrtostr(assetName)) == "Teko-Medium SDF No Glow") {
                // Found matching asset
                match = i;
                break;
            }
        }
        RET_0_UNLESS(match != -1);

        log(DEBUG, "TextObject::create: Instantiating the font");
        auto* font = RET_0_UNLESS(RunMethod("UnityEngine", "Object", "Instantiate", allObjects->values[match]));

        log(DEBUG, "TextObject::create: Setting the font");
        RET_0_UNLESS(RunMethod(textMesh, "set_font", font));

        // textMesh.rectTransform.SetParent(parent, false);
        log(DEBUG, "TextObject::create: Getting rectTransform");
        auto* rectTransform = RET_0_UNLESS(RunMethod(textMesh, "get_rectTransform"));

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

    bool TextObject::set(std::string_view text) {
        auto* str = il2cpp_utils::createcsstr(text);
        RET_0_UNLESS(il2cpp_utils::SetPropertyValue(textMesh, "text", str));
        return true;
    }

    bool TextObject::destroy() {
        RET_0_UNLESS(RunMethod("UnityEngine", "Object", "Destroy", gameObj));
        gameObj = nullptr;
        textMesh = nullptr;
        return true;
    }

    static il2cpp_array_size_t findDataSize(Il2CppObject* downloadHandler) {
        auto* data = RET_0_UNLESS(il2cpp_utils::RunMethod<Array<uint8_t>*>(
            "UnityEngine.Networking", "DownloadHandler", "InternalGetByteArray", downloadHandler));
        return data->Length();
    }

    void RawImageObject::monitorProgress(RawImageObject* obj) {
        log(INFO, "monitorProgress start");
        Il2CppObject* downloadHandler = RET_V_UNLESS(il2cpp_utils::GetFieldValue(obj->WWW, "m_DownloadHandler"));
        if (!downloadHandler) return;

        bool isDone = false;
        auto prevCap = findDataSize(downloadHandler);
        float prevProg = -1;
        std::vector<decltype(Array<int>::max_length)> recvLens;
        while (!isDone) {
            isDone = il2cpp_utils::RunMethod<bool>(obj->WWW, "get_isDone").value_or(false);
            if (auto progOpt = il2cpp_utils::RunMethod<float>(obj->WWW, "GetDownloadProgress")) {
                float prog = *progOpt;
                if (prog != prevProg) {
                    auto dataLen = findDataSize(downloadHandler);
                    decltype(Array<int>::max_length) cap = std::round(((float)dataLen) / prog);
                    if (cap != prevCap || prog < prevProg) {
                        log(INFO, "prog is %lu / %lu", dataLen, cap);
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
        log(INFO, "real progs (out of %lu): %s", finalLen, ss.str().c_str());

        log(INFO, "webRequest isDone.");
        auto* str = RET_V_UNLESS(RunMethod<Il2CppString*>(obj->WWW, "get_error"));
        if (str) {
            log(ERROR, "webRequest error: %s", to_utf8(csstrtostr(str)).c_str());
        } else {
            log(DEBUG, "webRequest had no error.");
        }
    }

    bool RawImageObject::textureWebRequestComplete(RawImageObject* obj, Il2CppObject* asyncOp) {
        // Notes: The field named "<webRequest>k__BackingField" on asyncOp is a pointer that should match obj->WWW
        log(DEBUG, "Entering textureWebRequestComplete!");

        obj->recievedTexture2D = RET_0_UNLESS(RunMethod("UnityEngine.Networking", "DownloadHandlerTexture", "GetContent", obj->WWW));

        obj->gameObj = RET_0_UNLESS(New(GetClassFromName("UnityEngine", "GameObject"), createcsstr("RandomImage")));
        RET_0_UNLESS(RunMethod(obj->gameObj, "SetActive", false));
        obj->rawImage = RET_0_UNLESS(RunMethod(obj->gameObj, "AddComponent", GetSystemType("UnityEngine.UI", "RawImage")));
        RET_0_UNLESS(RunMethod(obj->rawImage, "set_texture", obj->recievedTexture2D));

        auto* rawImageRectTransform = RET_0_UNLESS(RunMethod(obj->rawImage, "get_rectTransform"));
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
        WWW = RET_0_UNLESS(RunMethod("UnityEngine.Networking", "UnityWebRequestTexture", "GetTexture", createcsstr(url)));

        // If only we could use UnityEngine.WWW and its WaitUntilDoneIfPossible() :(
        auto* method = RET_0_UNLESS(FindMethodUnsafe("UnityEngine.Networking", "UnityWebRequestAsyncOperation", "add_completed", 1));

        sendWebRequestObj = RET_0_UNLESS(RunMethod(WWW, "SendWebRequest"));

        auto* action = RET_0_UNLESS(MakeAction(method, 0, this, textureWebRequestComplete));
        RET_0_UNLESS(RunMethod(sendWebRequestObj, method, action));

        // Uncomment this to watch the progress for debugging purposes (backtracks a lot):
        // StartCoroutine(monitorProgress, this);

        return true;
    }
} // namespace CustomUI
