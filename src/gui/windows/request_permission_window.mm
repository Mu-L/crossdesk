#include "layout.h"
#include "localization.h"
#include "rd_log.h"
#include "render.h"

#include <ApplicationServices/ApplicationServices.h>
#include <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#include <unistd.h>
#include <cstdlib>

namespace crossdesk {

static bool DrawToggleSwitch(const char* id, bool active, bool enabled) {
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  float height = ImGui::GetFrameHeight();
  float width = height * 1.8f;
  float radius = height * 0.5f;

  ImVec2 p = ImGui::GetCursorScreenPos();

  ImGui::InvisibleButton(id, ImVec2(width, height));
  bool hovered = ImGui::IsItemHovered();
  bool clicked = ImGui::IsItemClicked() && enabled;

  ImVec4 col_bg_vec;
  if (active) {
    col_bg_vec =
        hovered && enabled ? ImVec4(0.26f, 0.59f, 0.98f, 1.0f) : ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
  } else {
    col_bg_vec =
        hovered && enabled ? ImVec4(0.70f, 0.70f, 0.70f, 1.0f) : ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
  }
  if (!enabled) {
    col_bg_vec.w *= 0.6f;
  }
  ImU32 col_bg = ImGui::GetColorU32(col_bg_vec);

  draw_list->AddRectFilled(ImVec2(p.x, p.y + 0.5f), ImVec2(p.x + width, p.y + height - 0.5f),
                           col_bg, height * 0.5f);

  float t = active ? 1.0f : 0.0f;
  float knob_height = height - 4.0f;
  float knob_width = knob_height * 1.2f;
  float knob_radius = knob_height * 0.5f;

  float knob_min_x = p.x + 2.0f;
  float knob_max_x = p.x + width - knob_width - 2.0f;
  float knob_x = knob_min_x + t * (knob_max_x - knob_min_x);
  float knob_y = p.y + (height - knob_height) * 0.5f;

  ImU32 col_knob = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, enabled ? 1.0f : 0.9f));
  draw_list->AddRectFilled(ImVec2(knob_x, knob_y),
                           ImVec2(knob_x + knob_width, knob_y + knob_height), col_knob,
                           knob_radius);

  return clicked;
}

bool Render::CheckScreenRecordingPermission() {
  // CGPreflightScreenCaptureAccess is available on macOS 10.15+
  if (@available(macOS 10.15, *)) {
    bool granted = CGPreflightScreenCaptureAccess();
    return granted;
  }
  // for older macOS versions, assume permission is granted
  return true;
}

bool Render::CheckAccessibilityPermission() {
  NSDictionary* options = @{(__bridge id)kAXTrustedCheckOptionPrompt : @NO};
  bool trusted = AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
  return trusted;
}

void Render::OpenAccessibilityPreferences() {
  NSDictionary* options = @{(__bridge id)kAXTrustedCheckOptionPrompt : @YES};
  AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);

  system("open "
         "\"x-apple.systempreferences:com.apple.preference.security?Privacy_"
         "Accessibility\"");
}

void Render::OpenScreenRecordingPreferences() {
  if (@available(macOS 10.15, *)) {
    CGRequestScreenCaptureAccess();
  }

  system("open "
         "\"x-apple.systempreferences:com.apple.preference.security?Privacy_"
         "ScreenCapture\"");
}

int Render::RequestPermissionWindow() {
  bool screen_recording_granted = CheckScreenRecordingPermission();
  bool accessibility_granted = CheckAccessibilityPermission();

  show_request_permission_window_ = !screen_recording_granted || !accessibility_granted;

  if (!show_request_permission_window_) {
    return 0;
  }

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  float window_width = localization_language_index_ == 0 ? REQUEST_PERMISSION_WINDOW_WIDTH_CN
                                                         : REQUEST_PERMISSION_WINDOW_WIDTH_EN;
  float window_height = localization_language_index_ == 0 ? REQUEST_PERMISSION_WINDOW_HEIGHT_CN
                                                          : REQUEST_PERMISSION_WINDOW_HEIGHT_EN;

  // center the window on screen
  ImVec2 center_pos = ImVec2((viewport->WorkSize.x - window_width) * 0.5f + viewport->WorkPos.x,
                             (viewport->WorkSize.y - window_height) * 0.5f + viewport->WorkPos.y);
  ImGui::SetNextWindowPos(center_pos, ImGuiCond_Always);

  ImGui::SetNextWindowSize(ImVec2(window_width, window_height), ImGuiCond_Always);

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 15.0f));

  ImGui::Begin(localization::request_permissions[localization_language_index_].c_str(), nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoSavedSettings);

  ImGui::SetWindowFontScale(0.3f);

  // use system font
  if (system_chinese_font_ != nullptr) {
    ImGui::PushFont(system_chinese_font_);
  }

  ImGui::SetCursorPosX(10.0f);
  ImGui::TextWrapped(
      "%s", localization::permission_required_message[localization_language_index_].c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();

  // accessibility permission
  ImGui::SetCursorPosX(10.0f);
  ImGui::AlignTextToFramePadding();
  ImGui::Text("1. %s:",
              localization::accessibility_permission[localization_language_index_].c_str());
  ImGui::SameLine();
  ImGui::AlignTextToFramePadding();
  if (accessibility_granted) {
    DrawToggleSwitch("accessibility_toggle_on", true, false);
  } else {
    if (DrawToggleSwitch("accessibility_toggle", accessibility_granted, !accessibility_granted)) {
      OpenAccessibilityPreferences();
    }
  }

  ImGui::Spacing();

  // screen recording permission
  ImGui::SetCursorPosX(10.0f);
  ImGui::AlignTextToFramePadding();
  ImGui::Text("2. %s:",
              localization::screen_recording_permission[localization_language_index_].c_str());
  ImGui::SameLine();
  ImGui::AlignTextToFramePadding();
  if (screen_recording_granted) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    DrawToggleSwitch("screen_recording_toggle_on", true, false);
  } else {
    if (DrawToggleSwitch("screen_recording_toggle", screen_recording_granted,
                         !screen_recording_granted)) {
      OpenScreenRecordingPreferences();
    }
  }

  ImGui::SetWindowFontScale(1.0f);
  ImGui::SetWindowFontScale(0.45f);

  // pop system font
  if (system_chinese_font_ != nullptr) {
    ImGui::PopFont();
  }

  ImGui::End();
  ImGui::SetWindowFontScale(1.0f);
  ImGui::PopStyleVar(4);
  ImGui::PopStyleColor();

  return 0;
}
}  // namespace crossdesk