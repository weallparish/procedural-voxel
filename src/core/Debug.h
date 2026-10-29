#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Camera.h"


void renderDebug(int& chunksRendered, int& totalChunks, int& chunksUnloaded, ChunkStreamer* chunkStreamer, Camera* cam, bool* vsync) {
    // Debug Window
    ImGui::Begin("Debug");

    ImGui::Text("%.1f FPS (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

    if (ImGui::CollapsingHeader("Rendering")) {
        ImGui::Text("%d / %d / %d (Rendered / Generated / Unloaded)", chunksRendered, totalChunks, chunksUnloaded);
        ImGui::SliderInt("Render distance", &chunkStreamer->radius, 1, 50);
        ImGui::SliderInt("Renders per frame", &chunkStreamer->chunkLimitPerFrame, 1, 50);
    }
    
    ImGui::Text("Pos: %.1f, %.1f, %.1f", cam->pos.x, cam->pos.y, cam->pos.z);
    ImGui::SliderFloat("FOV", &cam->fov, 40.0f, 110.0f);
    ImGui::SliderFloat("Speed", &cam->speed, 1.0f, 50.0f);

    // This is called when the checkbox is selected.
    if (ImGui::Checkbox("VSync", vsync)) 
    {
        glfwSwapInterval(*vsync ? 1 : 0);
    }
        
    ImGui::Separator();
    ImGui::TextDisabled("WASD move, Space/Ctrl up/down, Shift sprint");
    ImGui::TextDisabled("Esc: toggle mouse capture");
    ImGui::End();
}