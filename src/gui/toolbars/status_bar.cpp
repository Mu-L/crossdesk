#include "localization.h"
#include "render.h"

namespace crossdesk {

int Render::StatusBar() {
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
  static bool a, b, c, d, e;
  ImGui::SetNextWindowPos(
      ImVec2(0, main_window_height_ - status_bar_height_ - 1),
      ImGuiCond_Always);

  ImGui::BeginChild(
      "StatusBar", ImVec2(main_window_width_, status_bar_height_ + 1),
      ImGuiChildFlags_Border,
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);

  ImVec2 dot_pos = ImVec2(13.0f * dpi_scale_,
                          (main_window_height_ - status_bar_height_) * 1.025f);
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  draw_list->AddCircleFilled(dot_pos, 5.0f * dpi_scale_,
                             ImColor(signal_connected_ ? 0.0f : 1.0f,
                                     signal_connected_ ? 1.0f : 0.0f, 0.0f),
                             100);
  draw_list->AddCircle(dot_pos, 6.0f * dpi_scale_, ImColor(1.0f, 1.0f, 1.0f),
                       100);

  ImGui::SetWindowFontScale(0.6f);
  draw_list->AddText(
      ImVec2(25.0f * dpi_scale_,
             (main_window_height_ - status_bar_height_) * 1.003f),
      ImColor(0.0f, 0.0f, 0.0f),
      signal_connected_
          ? localization::signal_connected[localization_language_index_].c_str()
          : localization::signal_disconnected[localization_language_index_]
                .c_str());
  ImGui::SetWindowFontScale(1.0f);

  ImGui::PopStyleColor();
  ImGui::EndChild();
  return 0;
}
}  // namespace crossdesk