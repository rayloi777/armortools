/*
 * CastleDB Editor - Column Editor
 */

#include "editor_app.h"
#include "imgui/imgui.h"
#include <string.h>

void editor_show_column_editor(void) {
    if (!g_editor.show_column_editor) return;

    ImGui::Begin("Column Editor", &g_editor.show_column_editor);

    static char name_buf[64] = "";
    static int type_selected = 1; // TString default
    static bool optional = false;

    ImGui::InputText("Name", name_buf, sizeof(name_buf));

    const char *type_names[] = {
        "TId", "TString", "TBool", "TInt", "TFloat",
        "TEnum", "TRef", "TImage", "TList", "TCustom",
        "TFlags", "TColor", "TLayer", "TFile", "TTilePos",
        "TTileLayer", "TDynamic", "TProperties", "TGradient", "TCurve",
        "TGuid", "TPolymorph"
    };

    ImGui::Combo("Type", &type_selected, type_names, 22);
    ImGui::Checkbox("Optional", &optional);

    // Type-specific options
    if (type_selected == CDB_TENUM || type_selected == CDB_TFLAGS) {
        static char enum_buf[256] = "value1,value2,value3";
        ImGui::InputText("Values (comma separated)", enum_buf, sizeof(enum_buf));
    }
    else if (type_selected == CDB_TREF) {
        static char ref_buf[64] = "";
        ImGui::InputText("Referenced Sheet", ref_buf, sizeof(ref_buf));
    }

    if (ImGui::Button("Add Column")) {
        CDBColumnType type = (CDBColumnType)type_selected;
        CDBColumn *col = cdb_column_create(name_buf, type);
        cdb_column_set_optional(col, optional);

        // Set type-specific options based on selected type
        // ...

        CDBSheet *sheet = cdb_get_sheet(g_editor.db, g_editor.selected_sheet);
        if (sheet) {
            cdb_sheet_add_column(sheet, col, -1);
            g_editor.modified = true;
        }
        memset(name_buf, 0, sizeof(name_buf));
    }

    ImGui::End();
}
