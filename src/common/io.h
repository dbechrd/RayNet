#pragma once
#include "common.h"

struct IO;

extern IO io;

struct IO {
    // In order of least to greatest I/O precedence
    enum Scope {
        IO_None,
        IO_Game,
        IO_GameNPC,
        IO_GameNPCDialog,
        IO_HUDSpinner,
        IO_HUDMenu,
        IO_HUDSignEditor,

        // Server-side, prolly shouldn't exist client-side
        IO_Editor,
        IO_EditorGroundOverlay,
        IO_EditorEntityOverlay,
        IO_EditorUI,
        IO_ScrollPanelOuter,
        IO_ScrollPanelInner,
        IO_EditorDrag,

        IO_F3Menu,
        IO_Count
    };

    struct Scoped {
        Scope scope{};

        Scoped(Scope scope) : scope(scope)
        {
            io.PushScope(scope);
        }

        ~Scoped(void)
        {
            io.PopScope();
        }
    };

    void EndFrame(void)
    {
        assert(scopeStack.empty());  // forgot to close a scope?

        prevKeyboardCaptureScope = keyboardCaptureScope;
        prevMouseCaptureScope = mouseCaptureScope;
        keyboardCaptureScope = IO_None;
        mouseCaptureScope = IO_None;
    }

    void PushScope(Scope scope)
    {
        scopeStack.push(scope);
    }

    void PopScope(void)
    {
        scopeStack.pop();
    }

    // TODO(dlb): Maybe these can just be bools in PushScope?
    void CaptureKeyboard(void)
    {
        keyboardCaptureScope = MAX(keyboardCaptureScope, scopeStack.top());
    }

    void CaptureMouse(void)
    {
        mouseCaptureScope = MAX(mouseCaptureScope, scopeStack.top());
    }

    // returns true if keyboard is captured by a higher precedence scope
    bool KeyboardCaptured(void)
    {
        int captureScope = MAX(prevKeyboardCaptureScope, keyboardCaptureScope);
        return captureScope > scopeStack.top();
    }

    // returns true if mouse is captured by a higher precedence scope
    bool MouseCaptured(void)
    {
        int captureScope = MAX(prevMouseCaptureScope, mouseCaptureScope);
        return captureScope > scopeStack.top();
    }

    bool KeyPressed(int key, bool include_repeats = false) {
        if (KeyboardCaptured()) {
            return false;
        }
        return IsKeyPressed(key) || (include_repeats && IsKeyPressedRepeat(key));
    }
    bool KeyDown(int key) {
        if (KeyboardCaptured()) {
            return false;
        }
        return IsKeyDown(key);
    }
    bool KeyReleased(int key) {
        if (KeyboardCaptured()) {
            return false;
        }
        return IsKeyReleased(key);
    }
    bool KeyUp(int key) {
        if (KeyboardCaptured()) {
            return false;
        }
        return IsKeyUp(key);
    }

    bool MouseButtonPressed(int button) {
        if (MouseCaptured()) {
            return false;
        }
        return IsMouseButtonPressed(button);
    }
    bool MouseButtonDown(int button) {
        if (MouseCaptured()) {
            return false;
        }
        return IsMouseButtonDown(button);
    }
    bool MouseButtonReleased(int button) {
        if (MouseCaptured()) {
            return false;
        }
        return IsMouseButtonReleased(button);
    }
    bool MouseButtonUp(int button) {
        if (MouseCaptured()) {
            return false;
        }
        return IsMouseButtonUp(button);
    }
    float MouseWheelMove(void) {
        if (MouseCaptured()) {
            return 0;
        }
        return GetMouseWheelMove();
    }
    Vector2 MouseWheelMoveV(void) {
        if (MouseCaptured()) {
            return {};
        }
        return GetMouseWheelMoveV();
    }

private:
    // NOTE: This default value prevents all input on the first frame (as opposed
    // to letting multiple things handle the input at once) but it probably doesn't
    // matter either way.
    Scope prevKeyboardCaptureScope = IO_Count;
    Scope prevMouseCaptureScope = IO_Count;
    Scope keyboardCaptureScope{};
    Scope mouseCaptureScope{};

    std::stack<Scope> scopeStack;
};
