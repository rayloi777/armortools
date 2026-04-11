#pragma once

#include <lib/cdb.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    CDB *db;
    char *filepath;
    int selected_sheet;
    int selected_row;
    int selected_col;
    bool modified;
    bool show_column_editor;
    bool show_about;
    bool running;
} EditorState;

extern EditorState g_editor;

void editor_init(void);
void editor_shutdown(void);
bool editor_should_close(void);
void editor_update(void);
void editor_render(void);

// File operations
void editor_new(void);
void editor_open(void);
void editor_save(void);
void editor_save_as(void);

// Undo/Redo (placeholder)
void editor_undo(void);
void editor_redo(void);

// Sheet operations
void editor_add_sheet(void);
void editor_delete_sheet(void);

// Sheet view
void editor_render_menu_bar(void);
void editor_render_sheet_list(void);
void editor_render_sheet_table(void);

// Value editors
void editor_render_cell_value(CDBSheet *sheet, int row, CDBColumn *col);
void editor_render_enum_editor(CDBSheet *sheet, int row, CDBColumn *col, const char *col_name);
void editor_render_color_editor(CDBSheet *sheet, int row, CDBColumn *col, const char *col_name);

// Column editor
void editor_show_column_editor(void);

#ifdef __cplusplus
}
#endif
