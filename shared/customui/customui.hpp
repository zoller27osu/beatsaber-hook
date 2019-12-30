#ifndef CUSTOMUI_DEFINED
#define CUSTOMUI_DEFINED

#include <string.h>
#include "../utils/typedefs.h"

namespace CustomUI { 
    class TextObject {
        private:
            // This counter increments every time a CustomUIText object is created by the same mod.
            static int counter;
            // This is the name prefix of the CustomUIText object. It has the counter appended to it.
            std::string name = "CustomUIText";
        public:
            // The TextMeshProUGUI Il2CppObject* of the created object.
            Il2CppObject *textMesh = nullptr;
            // The GameObject Il2CppObject* of the created object.
            Il2CppObject *gameObj = nullptr;
            // The desired Color of the text object.
            Color color = {1.0, 1.0, 1.0, 1.0};
            // The desired text of the text object.
            std::string text = "Default";
            // The desired font size of the text object.
            float fontSize = 40.0f;
            // The desired parent transform of the text object.
            Il2CppObject *parentTransform = nullptr;
            // The desired anchorMin position of the text object.
            Vector2 anchorMin = {0.0, 1.0};
            // The desired anchorMax position of the text object.
            Vector2 anchorMax = {0.0, 1.0};
            // The desired sizeDelta position of the text object.
            Vector2 sizeDelta = {0.0, 1.0};
            // The desired anchoredPosition of the text object.
            Vector2 anchoredPosition = {0.0, 0.0};
            // Creates the text object in game.
            // Returns true on success, false otherwise.
            bool create();
    };
} // namespace CustomUI

#endif /* CUSTOMUI_DEFINED */