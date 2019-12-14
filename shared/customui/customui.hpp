#ifndef CUSTOMUI_DEFINED
#define CUSTOMUI_DEFINED

#include <string.h>
#include <string_view>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include "../utils/typedefs.h"

#include "../dumps/UnityEngine_GameObject.hpp"
#include "../dumps/UnityEngine_RectTransform.hpp"
#include "../dumps/UnityEngine_Transform.hpp"
#include "../dumps/TMPro_TextMeshProUGUI.hpp"
#include "../dumps/UnityEngine_Resources.hpp"
#include "../dumps/UnityEngine_Object.hpp"
#include "../dumps/System_Type.hpp"
#include "../dumps/TMPro_TMP_FontAsset.hpp"
#include "../dumps/TMPro_TMP_Text.hpp"

namespace CustomUI {
    // Creates a custom TMP_TextMeshProUGUI object and returns it
    Il2CppObject* createtext(Il2CppObject* parent_transform, std::string_view text, Vector2 anchoredPosition, Vector2 sizeDelta);
}

#endif /* CUSTOMUI_DEFINED */