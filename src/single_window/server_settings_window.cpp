#include <filesystem>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#include "layout_style.h"
#include "localization.h"
#include "rd_log.h"
#include "render.h"

std::vector<std::string> GetRootEntries() {
  std::vector<std::string> roots;
#ifdef _WIN32
  DWORD mask = GetLogicalDrives();
  for (char letter = 'A'; letter <= 'Z'; ++letter) {
    if (mask & 1) {
      roots.push_back(std::string(1, letter) + ":\\");
    }
    mask >>= 1;
  }
#else
  roots.push_back("/");
#endif
  return roots;
}

int Render::ShowSimpleFileBrowser() {
  std::string display_text;
  if (!selected_file_.empty()) {
    display_text = std::filesystem::path(selected_file_).filename().string();
  } else if (selected_current_file_path_ != "Root") {
    display_text =
        std::filesystem::path(selected_current_file_path_).filename().string();
    if (display_text.empty()) {
      display_text = selected_current_file_path_;
    }
  }

  if (display_text.empty()) {
    display_text =
        localization::select_a_file[localization_language_index_].c_str();
  }

  if (ImGui::BeginCombo("##select_a_file", display_text.c_str())) {
    if (selected_current_file_path_ == "Root" ||
        !std::filesystem::exists(selected_current_file_path_) ||
        !std::filesystem::is_directory(selected_current_file_path_)) {
      auto roots = GetRootEntries();
      for (const auto& root : roots) {
        if (ImGui::Selectable(root.c_str())) {
          selected_current_file_path_ = root;
          selected_file_.clear();
        }
      }
    } else {
      std::filesystem::path p(selected_current_file_path_);

      if (ImGui::Selectable("..")) {
        if (p.has_parent_path() && p != p.root_path())
          selected_current_file_path_ = p.parent_path().string();
        else
          selected_current_file_path_ = "Root";
        selected_file_.clear();
      }

      try {
        for (const auto& entry :
             std::filesystem::directory_iterator(selected_current_file_path_)) {
          std::string name = entry.path().filename().string();
          if (entry.is_directory()) {
            if (ImGui::Selectable(name.c_str())) {
              selected_current_file_path_ = entry.path().string();
              selected_file_.clear();
            }
          } else {
            if (ImGui::Selectable(name.c_str())) {
              selected_file_ = entry.path().string();
            }
          }
        }
      } catch (const std::exception& e) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", e.what());
      }
    }

    ImGui::EndCombo();
  }

  return 0;
}

int Render::SelfHostedServerWindow() {
  if (show_self_hosted_server_config_window_) {
    if (self_hosted_server_config_window_pos_reset_) {
      const ImGuiViewport* viewport = ImGui::GetMainViewport();
      if (ConfigCenter::LANGUAGE::CHINESE == localization_language_) {
        ImGui::SetNextWindowPos(
            ImVec2((viewport->WorkSize.x - viewport->WorkPos.x -
                    SELF_HOSTED_SERVER_CONFIG_WINDOW_WIDTH_CN) /
                       2,
                   (viewport->WorkSize.y - viewport->WorkPos.y -
                    SELF_HOSTED_SERVER_CONFIG_WINDOW_HEIGHT_CN) /
                       2));

        ImGui::SetNextWindowSize(
            ImVec2(SELF_HOSTED_SERVER_CONFIG_WINDOW_WIDTH_CN,
                   SELF_HOSTED_SERVER_CONFIG_WINDOW_HEIGHT_CN));
      } else {
        ImGui::SetNextWindowPos(
            ImVec2((viewport->WorkSize.x - viewport->WorkPos.x -
                    SELF_HOSTED_SERVER_CONFIG_WINDOW_WIDTH_EN) /
                       2,
                   (viewport->WorkSize.y - viewport->WorkPos.y -
                    SELF_HOSTED_SERVER_CONFIG_WINDOW_HEIGHT_EN) /
                       2));

        ImGui::SetNextWindowSize(
            ImVec2(SELF_HOSTED_SERVER_CONFIG_WINDOW_WIDTH_EN,
                   SELF_HOSTED_SERVER_CONFIG_WINDOW_HEIGHT_EN));
      }

      self_hosted_server_config_window_pos_reset_ = false;
    }

    // Settings
    {
      static int settings_items_padding = 30;
      int settings_items_offset = 0;

      ImGui::SetWindowFontScale(0.5f);
      ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

      ImGui::Begin(localization::self_hosted_server_settings
                       [localization_language_index_]
                           .c_str(),
                   nullptr,
                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                       ImGuiWindowFlags_NoSavedSettings);
      ImGui::SetWindowFontScale(1.0f);
      ImGui::SetWindowFontScale(0.5f);
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
      {
        settings_items_offset += settings_items_padding;
        ImGui::SetCursorPosY(settings_items_offset + 2);
        ImGui::Text("%s", localization::self_hosted_server_address
                              [localization_language_index_]
                                  .c_str());
        if (ConfigCenter::LANGUAGE::CHINESE == localization_language_) {
          ImGui::SetCursorPosX(SELF_HOSTED_SERVER_HOST_INPUT_BOX_PADDING_CN);
        } else {
          ImGui::SetCursorPosX(SELF_HOSTED_SERVER_HOST_INPUT_BOX_PADDING_EN);
        }
        ImGui::SetCursorPosY(settings_items_offset);
        ImGui::SetNextItemWidth(SELF_HOSTED_SERVER_INPUT_WINDOW_WIDTH);

        ImGui::InputText("##self_hosted_server_host_", self_hosted_server_host_,
                         IM_ARRAYSIZE(self_hosted_server_host_));
      }

      ImGui::Separator();

      {
        settings_items_offset += settings_items_padding;
        ImGui::SetCursorPosY(settings_items_offset + 2);
        ImGui::Text(
            "%s",
            localization::self_hosted_server_port[localization_language_index_]
                .c_str());

        if (ConfigCenter::LANGUAGE::CHINESE == localization_language_) {
          ImGui::SetCursorPosX(SELF_HOSTED_SERVER_PORT_INPUT_BOX_PADDING_CN);
        } else {
          ImGui::SetCursorPosX(SELF_HOSTED_SERVER_PORT_INPUT_BOX_PADDING_EN);
        }
        ImGui::SetCursorPosY(settings_items_offset);
        ImGui::SetNextItemWidth(SELF_HOSTED_SERVER_INPUT_WINDOW_WIDTH);

        ImGui::InputText("##self_hosted_server_port_", self_hosted_server_port_,
                         IM_ARRAYSIZE(self_hosted_server_port_));
      }

      ImGui::Separator();

      {
        settings_items_offset += settings_items_padding;
        ImGui::SetCursorPosY(settings_items_offset + 2);
        ImGui::Text("%s", localization::self_hosted_server_certificate_path
                              [localization_language_index_]
                                  .c_str());

        if (ConfigCenter::LANGUAGE::CHINESE == localization_language_) {
          ImGui::SetCursorPosX(SELF_HOSTED_SERVER_PORT_INPUT_BOX_PADDING_CN);
        } else {
          ImGui::SetCursorPosX(SELF_HOSTED_SERVER_PORT_INPUT_BOX_PADDING_EN);
        }
        ImGui::SetCursorPosY(settings_items_offset);
        ImGui::SetNextItemWidth(SELF_HOSTED_SERVER_INPUT_WINDOW_WIDTH);

        ShowSimpleFileBrowser();
      }

      if (stream_window_inited_) {
        ImGui::EndDisabled();
      }

      if (ConfigCenter::LANGUAGE::CHINESE == localization_language_) {
        ImGui::SetCursorPosX(SELF_HOSTED_SERVER_CONFIG_OK_BUTTON_PADDING_CN);
      } else {
        ImGui::SetCursorPosX(SELF_HOSTED_SERVER_CONFIG_OK_BUTTON_PADDING_EN);
      }

      settings_items_offset += settings_items_padding + 10;
      ImGui::SetCursorPosY(settings_items_offset);
      ImGui::PopStyleVar();

      // OK
      if (ImGui::Button(
              localization::ok[localization_language_index_].c_str())) {
        show_self_hosted_server_config_window_ = false;

        // Language
        if (language_button_value_ == 0) {
          config_center_.SetLanguage(ConfigCenter::LANGUAGE::CHINESE);
        } else {
          config_center_.SetLanguage(ConfigCenter::LANGUAGE::ENGLISH);
        }
        language_button_value_last_ = language_button_value_;
        localization_language_ = (ConfigCenter::LANGUAGE)language_button_value_;
        localization_language_index_ = language_button_value_;
        LOG_INFO("Set localization language: {}",
                 localization_language_index_ == 0 ? "zh" : "en");

        // Video quality
        if (video_quality_button_value_ == 0) {
          config_center_.SetVideoQuality(ConfigCenter::VIDEO_QUALITY::HIGH);
        } else if (video_quality_button_value_ == 1) {
          config_center_.SetVideoQuality(ConfigCenter::VIDEO_QUALITY::MEDIUM);
        } else {
          config_center_.SetVideoQuality(ConfigCenter::VIDEO_QUALITY::LOW);
        }
        video_quality_button_value_last_ = video_quality_button_value_;

        // Video encode format
        if (video_encode_format_button_value_ == 0) {
          config_center_.SetVideoEncodeFormat(
              ConfigCenter::VIDEO_ENCODE_FORMAT::AV1);
        } else if (video_encode_format_button_value_ == 1) {
          config_center_.SetVideoEncodeFormat(
              ConfigCenter::VIDEO_ENCODE_FORMAT::H264);
        }
        video_encode_format_button_value_last_ =
            video_encode_format_button_value_;

        // Hardware video codec
        if (enable_hardware_video_codec_) {
          config_center_.SetHardwareVideoCodec(true);
        } else {
          config_center_.SetHardwareVideoCodec(false);
        }
        enable_hardware_video_codec_last_ = enable_hardware_video_codec_;

        // TURN mode
        if (enable_turn_) {
          config_center_.SetTurn(true);
        } else {
          config_center_.SetTurn(false);
        }
        enable_turn_last_ = enable_turn_;

        // SRTP
        if (enable_srtp_) {
          config_center_.SetSrtp(true);
        } else {
          config_center_.SetSrtp(false);
        }
        enable_srtp_last_ = enable_srtp_;

        SaveSettingsIntoCacheFile();
        self_hosted_server_config_window_pos_reset_ = true;

        // Recreate peer instance
        LoadSettingsFromCacheFile();

        // Recreate peer instance
        if (!stream_window_inited_) {
          LOG_INFO("Recreate peer instance");
          CleanupPeers();
          CreateConnectionPeer();
        }
      }

      ImGui::SameLine();
      // Cancel
      if (ImGui::Button(
              localization::cancel[localization_language_index_].c_str())) {
        show_self_hosted_server_config_window_ = false;
        if (language_button_value_ != language_button_value_last_) {
          language_button_value_ = language_button_value_last_;
        }

        if (video_quality_button_value_ != video_quality_button_value_last_) {
          video_quality_button_value_ = video_quality_button_value_last_;
        }

        if (video_encode_format_button_value_ !=
            video_encode_format_button_value_last_) {
          video_encode_format_button_value_ =
              video_encode_format_button_value_last_;
        }

        if (enable_hardware_video_codec_ != enable_hardware_video_codec_last_) {
          enable_hardware_video_codec_ = enable_hardware_video_codec_last_;
        }

        if (enable_turn_ != enable_turn_last_) {
          enable_turn_ = enable_turn_last_;
        }

        self_hosted_server_config_window_pos_reset_ = true;
      }
      ImGui::SetWindowFontScale(1.0f);
      ImGui::SetWindowFontScale(0.5f);
      ImGui::End();
      ImGui::PopStyleVar(2);
      ImGui::PopStyleColor();
      ImGui::SetWindowFontScale(1.0f);
    }
  }

  return 0;
}