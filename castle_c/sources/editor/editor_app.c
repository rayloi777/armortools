/*
 * CastleDB Editor - App State
 */

#include "editor_app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WITH_NFD
#include <nfd.h>
#endif

EditorState g_editor = {0};

void editor_init(void) {
    memset(&g_editor, 0, sizeof(g_editor));
    g_editor.db = cdb_create();
    g_editor.selected_sheet = 0;
    g_editor.modified = false;
    g_editor.running = true;
}

void editor_shutdown(void) {
    if (g_editor.db) cdb_close(g_editor.db);
    free(g_editor.filepath);
    memset(&g_editor, 0, sizeof(g_editor));
}

bool editor_should_close(void) {
    return !g_editor.running;
}

void editor_update(void) {
    // Placeholder - will be filled with actual update logic
}

void editor_render(void) {
    // Placeholder - will be filled with actual render logic
}

// Placeholder implementations
void editor_undo(void) {
    // TODO: Implement undo
}

void editor_redo(void) {
    // TODO: Implement redo
}

void editor_new(void) {
    if (g_editor.db) cdb_close(g_editor.db);
    g_editor.db = cdb_create();
    cdb_create_sheet(g_editor.db, "Sheet1");
    free(g_editor.filepath);
    g_editor.filepath = NULL;
    g_editor.modified = false;
    g_editor.selected_sheet = 0;
}

void editor_open(void) {
#ifdef WITH_NFD
    nfdchar_t *out_path = NULL;
    nfdresult_t result = NFD_OpenDialog("cdb", NULL, &out_path);

    if (result == NFD_OKAY) {
        if (g_editor.db) cdb_close(g_editor.db);
        g_editor.db = cdb_create();
        if (cdb_load(g_editor.db, out_path) != 0) {
            printf("Error loading file: %s\n", cdb_error_string(cdb_get_error(g_editor.db)));
        }
        free(g_editor.filepath);
        g_editor.filepath = strdup(out_path);
        g_editor.modified = false;
        free(out_path);
    }
#else
    printf("NFD not available - cannot open file\n");
#endif
}

void editor_save(void) {
    if (!g_editor.filepath) {
        editor_save_as();
        return;
    }
    if (cdb_save(g_editor.db, g_editor.filepath) != 0) {
        printf("Error saving file: %s\n", cdb_error_string(cdb_get_error(g_editor.db)));
    }
    g_editor.modified = false;
}

void editor_save_as(void) {
#ifdef WITH_NFD
    nfdchar_t *out_path = NULL;
    nfdresult_t result = NFD_SaveDialog("cdb", NULL, &out_path);

    if (result == NFD_OKAY) {
        if (cdb_save(g_editor.db, out_path) != 0) {
            printf("Error saving file: %s\n", cdb_error_string(cdb_get_error(g_editor.db)));
        }
        free(g_editor.filepath);
        g_editor.filepath = strdup(out_path);
        g_editor.modified = false;
        free(out_path);
    }
#else
    printf("NFD not available - cannot save file\n");
#endif
}

void editor_add_sheet(void) {
    // TODO: Add sheet dialog
}

void editor_delete_sheet(void) {
    // TODO: Delete sheet
}
