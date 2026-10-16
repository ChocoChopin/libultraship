#pragma once

#include <SDL2/SDL.h>

namespace Ship {

// Native Windows input uses Win32 messages and polls SDL controller state.
// Unlike the SDL window backend, it has no consumer for joystick/controller
// motion notifications. Leaving them queued makes every hotplug search scan
// an ever-growing list. Removing notifications does not change the polled
// state, run another input sample, or remove controller add/remove events.
// Use only with a native window backend: SDL windows still dispatch their
// events to ImGui and the window handler.
inline void DiscardNativeWindowControllerNotifications() {
    SDL_FlushEvents(SDL_JOYAXISMOTION, SDL_CONTROLLERDEVICEADDED - 1);
    SDL_FlushEvents(SDL_CONTROLLERDEVICEREMOVED + 1, SDL_FINGERDOWN - 1);
}

} // namespace Ship
