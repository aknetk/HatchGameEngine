#if INTERFACE
#include <Engine/Includes/Standard.h>
#include <Engine/Includes/StandardSDL2.h>
#include <Engine/Application.h>

class InputManager {
public:
    static float MouseX;
    static float MouseY;
    static int   MouseDown;
    static int   MousePressed;
    static int   MouseReleased;

    static Uint8 KeyboardState[0x120];
    static Uint8 KeyboardStateLast[0x120];

    static void* Controllers[8];
    static void* ControllerHaptics[8];

    static SDL_TouchID TouchDevice;
    static void*       TouchStates;
};
#endif

#include <Engine/InputManager.h>

float       InputManager::MouseX = 0;
float       InputManager::MouseY = 0;
int         InputManager::MouseDown = 0;
int         InputManager::MousePressed = 0;
int         InputManager::MouseReleased = 0;

Uint8       InputManager::KeyboardState[0x120];
Uint8       InputManager::KeyboardStateLast[0x120];

void*       InputManager::Controllers[8];
void*       InputManager::ControllerHaptics[8];

SDL_TouchID InputManager::TouchDevice;
void*       InputManager::TouchStates;

struct TouchState {
    float X;
    float Y;
    bool Down;
    bool Pressed;
    bool Released;
};

PUBLIC STATIC void  InputManager::Init() {
    memset(KeyboardState, 0, 0x120);
    memset(KeyboardStateLast, 0, 0x120);

    Log::Print(Log::LOG_VERBOSE, "Opening joysticks... (%d count)", SDL_NumJoysticks());
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        InputManager::Controllers[i] = SDL_JoystickOpen(i);

        if (InputManager::Controllers[i])
            Log::Print(Log::LOG_VERBOSE, "InputManager::Controllers[%d] opened. (%p)", i, InputManager::Controllers[i]);
        else
            Log::Print(Log::LOG_ERROR, "InputManager::Controllers[%d] failed: %s", i, SDL_GetError());

        if (InputManager::Controllers[i])
            InputManager::ControllerHaptics[i] = SDL_HapticOpenFromJoystick((SDL_Joystick*)InputManager::Controllers[i]);

        if (InputManager::ControllerHaptics[i] && SDL_HapticRumbleInit((SDL_Haptic*)InputManager::ControllerHaptics[i]))
            InputManager::ControllerHaptics[i] = NULL;
    }

    InputManager::TouchStates = Memory::TrackedCalloc("InputManager::TouchStates", 8, sizeof(TouchState));
    for (int t = 0; t < 8; t++) {
        TouchState* current = &((TouchState*)TouchStates)[t];
        current->X = 0.0f;
        current->Y = 0.0f;
        current->Down = false;
        current->Pressed = false;
        current->Released = false;
    }
}

PUBLIC STATIC void  InputManager::Poll() {
    if (Application::Platform == Platforms::iOS ||
        Application::Platform == Platforms::Android ||
        Application::Platform == Platforms::Switch) {
        int w, h;
        TouchState* states = (TouchState*)TouchStates;
        SDL_GetWindowSize(Application::Window, &w, &h);

        bool OneDown = false;
        for (int d = 0; d < SDL_GetNumTouchDevices(); d++) {
            TouchDevice = SDL_GetTouchDevice(d);
            if (TouchDevice) {
                OneDown = false;
                for (int t = 0; t < 8 && SDL_GetNumTouchFingers(TouchDevice); t++) {
                    TouchState* current = &states[t];

                    bool previouslyDown = current->Down;

                    current->Down = false;
                    if (t < SDL_GetNumTouchFingers(TouchDevice)) {
                        SDL_Finger* finger = SDL_GetTouchFinger(TouchDevice, t);
                        float tx = finger->x * w;
                        float ty = finger->y * h;

                        current->X = tx;
                        current->Y = ty;
                        current->Down = true;

                        OneDown = true;
                    }

                    current->Pressed = !previouslyDown && current->Down;
                    current->Released = previouslyDown && !current->Down;
                }

                if (OneDown)
                    break;
            }
        }

        // NOTE: If no touches were down, set all their
        //   states to !Down and Released appropriately.
        if (!OneDown) {
            for (int t = 0; t < 8; t++) {
                TouchState* current = &states[t];

                bool previouslyDown = current->Down;

                current->Down = false;
                current->Pressed = !previouslyDown && current->Down;
                current->Released = previouslyDown && !current->Down;
            }
        }
    }

    const Uint8* state = SDL_GetKeyboardState(NULL);
    memcpy(KeyboardStateLast, KeyboardState, 0x11C + 1);
    memcpy(KeyboardState, state, 0x11C + 1);

    int mx, my, buttons;
    buttons = SDL_GetMouseState(&mx, &my);
    MouseX = mx;
    MouseY = my;

    int lastDown = MouseDown;
    MouseDown = 0;
    MousePressed = 0;
    MouseReleased = 0;
    for (int i = 0; i < 8; i++) {
        int lD, mD, mP, mR;
        lD = (lastDown >> i) & 1;
        mD = (buttons >> i) & 1;
        mP = !lD && mD;
        mR = lD && !mD;

        MouseDown |= mD << i;
        MousePressed |= mP << i;
        MouseReleased |= mR << i;
    }
}

PUBLIC STATIC bool  InputManager::GetControllerAttached(int controller_index) {
    SDL_Joystick* joy = (SDL_Joystick*)InputManager::Controllers[controller_index];
    if (!joy) return false;
    return SDL_JoystickGetAttached(joy);
}
PUBLIC STATIC float InputManager::GetControllerAxis(int controller_index, int index) {
    SDL_Joystick* joy = (SDL_Joystick*)InputManager::Controllers[controller_index];
    return SDL_JoystickGetAxis(joy, index) / (double)0x8000;
}
PUBLIC STATIC bool  InputManager::GetControllerButton(int controller_index, int index) {
    SDL_Joystick* joy = (SDL_Joystick*)InputManager::Controllers[controller_index];
    return SDL_JoystickGetButton(joy, index);
}
PUBLIC STATIC int   InputManager::GetControllerHat(int controller_index, int index) {
    SDL_Joystick* joy = (SDL_Joystick*)InputManager::Controllers[controller_index];
    return SDL_JoystickGetHat(joy, index);
}
PUBLIC STATIC char* InputManager::GetControllerName(int controller_index) {
    SDL_Joystick* joy = (SDL_Joystick*)InputManager::Controllers[controller_index];
    return (char*)SDL_JoystickName(joy);
}
PUBLIC STATIC void  InputManager::SetControllerVibrate(int controller_index, double strength, int duration) {
    if (InputManager::ControllerHaptics[controller_index])
        SDL_HapticRumblePlay((SDL_Haptic*)InputManager::ControllerHaptics[controller_index], strength, duration);
}

PUBLIC STATIC float InputManager::TouchGetX(int touch_index) {
    TouchState* states = (TouchState*)TouchStates;
    return states[touch_index].X;
}
PUBLIC STATIC float InputManager::TouchGetY(int touch_index) {
    TouchState* states = (TouchState*)TouchStates;
    return states[touch_index].Y;
}
PUBLIC STATIC bool  InputManager::TouchIsDown(int touch_index) {
    TouchState* states = (TouchState*)TouchStates;
    return states[touch_index].Down;
}
PUBLIC STATIC bool  InputManager::TouchIsPressed(int touch_index) {
    TouchState* states = (TouchState*)TouchStates;
    return states[touch_index].Pressed;
}
PUBLIC STATIC bool  InputManager::TouchIsReleased(int touch_index) {
    TouchState* states = (TouchState*)TouchStates;
    return states[touch_index].Released;
}

PUBLIC STATIC void  InputManager::Dispose() {
    // Close joysticks
    for (int i = 0; i < 8; i++) {
        SDL_Joystick* joy = (SDL_Joystick*)InputManager::Controllers[i];
        if (!joy) continue;

        if (InputManager::ControllerHaptics[i])
            SDL_HapticClose((SDL_Haptic*)InputManager::ControllerHaptics[i]);

        if (SDL_JoystickGetAttached(joy)) {
            SDL_JoystickClose(joy);
        }
    }

    // Dispose of touch states
    Memory::Free(InputManager::TouchStates);
}
