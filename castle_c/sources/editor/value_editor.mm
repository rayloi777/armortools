/*
 * CastleDB Editor - Value Editors
 */

#include "editor_app.h"
#include "lib/cdb_internal.h"
#include "imgui/imgui.h"
#include <stdio.h>
#include <string.h>

void editor_render_cell_value(CDBSheet *sheet, int row, CDBColumn *col) {
    CDBColumnType type = cdb_column_type(col);
    const char *col_name = cdb_column_name(col);

    switch (type) {
        case CDB_TSTRING:
        case CDB_TID:
        case CDB_TFILE: {
            static char buf[256];
            const char *val = cdb_get_string(sheet, row, col_name);
            strncpy(buf, val ? val : "", sizeof(buf));
            buf[sizeof(buf) - 1] = '\0';
            if (ImGui::InputText(col_name, buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                cdb_set_string(sheet, row, col_name, buf);
                g_editor.modified = true;
            }
            break;
        }
        case CDB_TINT: {
            int val = cdb_get_int(sheet, row, col_name);
            if (ImGui::InputInt(col_name, &val)) {
                cdb_set_int(sheet, row, col_name, val);
                g_editor.modified = true;
            }
            break;
        }
        case CDB_TFLOAT: {
            float val = cdb_get_float(sheet, row, col_name);
            if (ImGui::InputFloat(col_name, &val)) {
                cdb_set_float(sheet, row, col_name, val);
                g_editor.modified = true;
            }
            break;
        }
        case CDB_TBOOL: {
            bool val = cdb_get_bool(sheet, row, col_name) != 0;
            if (ImGui::Checkbox(col_name, &val)) {
                cdb_set_bool(sheet, row, col_name, val ? 1 : 0);
                g_editor.modified = true;
            }
            break;
        }
        case CDB_TENUM: {
            editor_render_enum_editor(sheet, row, col, col_name);
            break;
        }
        case CDB_TCOLOR: {
            editor_render_color_editor(sheet, row, col, col_name);
            break;
        }
        default: {
            // For complex types, show raw string
            const char *val = cdb_get_raw(sheet, row, col_name);
            ImGui::Text("%s", val ? val : "(null)");
            break;
        }
    }
}

void editor_render_enum_editor(CDBSheet *sheet, int row, CDBColumn *col, const char *col_name) {
    int current = cdb_get_int(sheet, row, col_name);
    const char *preview = (col->enum_values && current >= 0 && current < col->enum_count)
        ? col->enum_values[current] : "";

    if (ImGui::BeginCombo(col_name, preview)) {
        for (int i = 0; i < col->enum_count; i++) {
            bool selected = (current == i);
            if (ImGui::Selectable(col->enum_values[i], selected)) {
                cdb_set_int(sheet, row, col_name, i);
                g_editor.modified = true;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

void editor_render_color_editor(CDBSheet *sheet, int row, CDBColumn *col, const char *col_name) {
    int val = cdb_get_int(sheet, row, col_name);
    float col_r = ((val >> 16) & 0xFF) / 255.0f;
    float col_g = ((val >> 8) & 0xFF) / 255.0f;
    float col_b = (val & 0xFF) / 255.0f;

    float color[4] = {col_r, col_g, col_b, 1.0f};
    if (ImGui::ColorEdit4(col_name, color)) {
        int new_val = ((int)(color[0] * 255) << 16) |
                      ((int)(color[1] * 255) << 8) |
                      ((int)(color[2] * 255));
        cdb_set_int(sheet, row, col_name, new_val);
        g_editor.modified = true;
    }
}
