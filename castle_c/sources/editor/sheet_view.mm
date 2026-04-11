/*
 * CastleDB Editor - Sheet View
 */

#include "editor_app.h"
#include "imgui/imgui.h"
#include <string.h>
#include <stdio.h>

void editor_render_sheet_list(void) {
    ImGui::BeginChild("SheetList", ImVec2(200, 0), true);

    int sheet_count = cdb_get_sheet_count(g_editor.db);
    for (int i = 0; i < sheet_count; i++) {
        CDBSheet *sheet = cdb_get_sheet(g_editor.db, i);
        const char *name = cdb_sheet_name(sheet);

        // Skip hidden sheets (sub-sheets have @)
        if (strchr(name, '@') != NULL) continue;

        ImGuiTreeNodeFlags flags = (g_editor.selected_sheet == i)
            ? ImGuiTreeNodeFlags_Selected : 0;
        flags |= ImGuiTreeNodeFlags_Leaf;

        if (ImGui::TreeNodeEx(name, flags)) {
            ImGui::TreePop();
        }

        if (ImGui::IsItemClicked()) {
            g_editor.selected_sheet = i;
        }
    }

    ImGui::EndChild();
}

void editor_render_sheet_table(void) {
    CDBSheet *sheet = cdb_get_sheet(g_editor.db, g_editor.selected_sheet);
    if (!sheet) return;

    int col_count = cdb_sheet_column_count(sheet);
    int row_count = cdb_sheet_row_count(sheet);

    ImGui::BeginChild("SheetView");

    // Header row
    ImGui::Columns(col_count + 1, "sheet_columns", true);
    ImGui::Text("Row"); ImGui::NextColumn();

    for (int i = 0; i < col_count; i++) {
        CDBColumn *col = cdb_sheet_column(sheet, i);
        ImGui::Text("%s", cdb_column_name(col));
        ImGui::NextColumn();
    }
    ImGui::Separator();

    // Data rows
    for (int r = 0; r < row_count; r++) {
        char row_label[32];
        snprintf(row_label, sizeof(row_label), "%d", r);
        if (ImGui::Selectable(row_label, g_editor.selected_row == r)) {
            g_editor.selected_row = r;
        }
        ImGui::NextColumn();

        for (int c = 0; c < col_count; c++) {
            CDBColumn *col = cdb_sheet_column(sheet, c);
            const char *col_name = cdb_column_name(col);
            editor_render_cell_value(sheet, r, col);
            ImGui::NextColumn();
        }
    }

    ImGui::Columns(1);
    ImGui::EndChild();
}
