/*
 * CastleDB Editor - Menu Bar
 */

#include "editor_app.h"
#include "imgui/imgui.h"
#include <stdio.h>

void editor_render_menu_bar(void) {
    if (ImGui::BeginMenuBar()) {
        // File menu
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) {
                editor_new();
            }
            if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                editor_open();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                editor_save();
            }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                editor_save_as();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                g_editor.running = false;
            }
            ImGui::EndMenu();
        }

        // Edit menu
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                editor_undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                editor_redo();
            }
            ImGui::EndMenu();
        }

        // Sheet menu
        if (ImGui::BeginMenu("Sheet")) {
            if (ImGui::MenuItem("Add Sheet")) {
                editor_add_sheet();
            }
            if (ImGui::MenuItem("Delete Sheet")) {
                editor_delete_sheet();
            }
            ImGui::EndMenu();
        }

        // Help menu
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About CastleDB")) {
                g_editor.show_about = true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}
