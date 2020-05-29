#ifndef ENGINE_INPUTMANAGER_H
#define ENGINE_INPUTMANAGER_H

#define PUBLIC
#define PRIVATE
#define PROTECTED
#define STATIC
#define VIRTUAL
#define EXPOSED


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

    static void  Init();
    static void  Poll();
    static bool  GetControllerAttached(int controller_index);
    static float GetControllerAxis(int controller_index, int index);
    static bool  GetControllerButton(int controller_index, int index);
    static int   GetControllerHat(int controller_index, int index);
    static char* GetControllerName(int controller_index);
    static void  SetControllerVibrate(int controller_index, double strength, int duration);
    static float TouchGetX(int touch_index);
    static float TouchGetY(int touch_index);
    static bool  TouchIsDown(int touch_index);
    static bool  TouchIsPressed(int touch_index);
    static bool  TouchIsReleased(int touch_index);
    static void  Dispose();
};

#endif /* ENGINE_INPUTMANAGER_H */
