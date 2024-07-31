#include "IconsFontAwesome6.h"
#include "localization.h"
#include "render.h"

int Render::TitleBar() {
  ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0, 0, 0, 0.05f));
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
  ImGui::BeginChild("TitleBar", ImVec2(main_window_width_, title_bar_height_),
                    ImGuiChildFlags_Border,
                    ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDecoration |
                        ImGuiWindowFlags_NoBringToFrontOnFocus);

  ImGui::SetWindowFontScale(1.0f);
  if (ImGui::BeginMenuBar()) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

    ImGui::SetCursorPosX(60);
    if (streaming_) {
      std::string mouse = ICON_FA_COMPUTER_MOUSE;
      if (ImGui::Button(mouse.c_str(), ImVec2(30, 30))) {
        if (mouse_control_button_label_ ==
                localization::control_mouse[localization_language_index_] &&
            connection_established_) {
          mouse_control_button_pressed_ = true;
          control_mouse_ = true;
          mouse_control_button_label_ =
              localization::release_mouse[localization_language_index_];
        } else {
          control_mouse_ = false;
          mouse_control_button_label_ =
              localization::control_mouse[localization_language_index_];
        }
        mouse_control_button_pressed_ = !mouse_control_button_pressed_;
      }

      ImGui::SameLine();
      // Fullscreen
      std::string fullscreen =
          fullscreen_button_pressed_ ? ICON_FA_COMPRESS : ICON_FA_EXPAND;
      if (ImGui::Button(fullscreen.c_str(), ImVec2(30, 30))) {
        fullscreen_button_pressed_ = !fullscreen_button_pressed_;
        if (fullscreen_button_pressed_) {
          main_window_width_before_fullscreen_ = main_window_width_;
          main_window_height_before_fullscreen_ = main_window_height_;
          SDL_SetWindowFullscreen(main_window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
        } else {
          SDL_SetWindowFullscreen(main_window_, SDL_FALSE);
          SDL_SetWindowSize(main_window_, main_window_width_before_fullscreen_,
                            main_window_height_before_fullscreen_);
          main_window_width_ = main_window_width_before_fullscreen_;
          main_window_height_ = main_window_height_before_fullscreen_;
        }
      }
    }

    ImGui::SetCursorPosX(main_window_width_ - (streaming_ ? 90 : 60));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    std::string window_minimize_button = ICON_FA_MINUS;
    if (ImGui::Button(window_minimize_button.c_str(), ImVec2(30, 30))) {
      SDL_MinimizeWindow(main_window_);
    }
    ImGui::PopStyleColor(2);

    if (streaming_) {
      ImGui::SetCursorPosX(main_window_width_ - 60);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0.1f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                            ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

      if (window_maximized_) {
        std::string window_restore_button = ICON_FA_WINDOW_RESTORE;
        if (ImGui::Button(window_restore_button.c_str(), ImVec2(30, 30))) {
          SDL_RestoreWindow(main_window_);
          window_maximized_ = !window_maximized_;
        }
      } else {
        std::string window_maximize_button = ICON_FA_SQUARE;
        if (ImGui::Button(window_maximize_button.c_str(), ImVec2(30, 30))) {
          SDL_GetWindowSize(main_window_, &main_window_width_before_maximized_,
                            &main_window_height_before_maximized_);
          SDL_MaximizeWindow(main_window_);
          window_maximized_ = !window_maximized_;
        }
      }
      ImGui::PopStyleColor(2);
    }

    ImGui::SetCursorPosX(main_window_width_ - 30);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0, 0, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0, 0, 0.5f));
    std::string close_button = ICON_FA_XMARK;
    if (ImGui::Button(close_button.c_str(), ImVec2(30, 30))) {
      SDL_Event event;
      event.type = SDL_QUIT;
      SDL_PushEvent(&event);
    }
    ImGui::PopStyleColor(2);

    ImGui::PopStyleColor(1);
  }
  ImGui::SetWindowFontScale(1.0f);

  ImGui::EndChild();
  ImGui::PopStyleColor(2);
  return 0;
}