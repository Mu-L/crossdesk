#include "keyboard_capturer.h"

KeyboardCapturer::KeyboardCapturer() {}

KeyboardCapturer::~KeyboardCapturer() {}

int KeyboardCapturer::Hook(OnKeyAction on_key_action, void *user_ptr) {
  return 0;
}

int KeyboardCapturer::Unhook() { return 0; }

int KeyboardCapturer::SendKeyboardCommand(int key_code, bool is_down) {
  return 0;
}