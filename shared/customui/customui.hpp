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

#include "customui.hpp"

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
    class TextUI {
        private:
            std::string  name             = "CustomUIText";
        public:
            Il2CppObject *textMesh        = nullptr;
            Il2CppObject *gameObj         = nullptr;
            Color        color            = {1.0, 1.0, 1.0, 1.0};
            std::string  text             = "Default";
            float        fontSize         = 40.0f;
            Il2CppObject *parentTransform = nullptr;
            bool create();
    };
} // namespace CustomUI

#endif /* CUSTOMUI_DEFINED */