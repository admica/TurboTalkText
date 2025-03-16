#include "gui.h"
#include "logger.h"
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
void init_gui(SDL_Window* window, SDL_Renderer* renderer) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
}
void render_gui(const std::string& status, const std::string& transcribed_text) {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("TurboTalkText");
    ImGui::Text("Status: %s", status.c_str());
    ImGui::Text("Transcribed: %s", transcribed_text.c_str());
    ImGui::BeginChild("Logs", ImVec2(0, 0), true);
    ImGui::TextUnformatted(g_logger->sinks()[0]->to_string().c_str()); // Simplified log display
    ImGui::EndChild();
    ImGui::End();
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
}
void shutdown_gui() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}
