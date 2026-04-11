/*
 * CastleDB Editor - Main Entry Point
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "editor_app.h"
#include <stdio.h>
#include <stdlib.h>

// External UI functions (implemented in editor_ui.mm with C++)
void editor_ui_init(void);
void editor_ui_shutdown(void);
void editor_run(void);

int main(int argc, char **argv) {
    printf("CastleDB Editor starting...\n");
    printf("Version: %s\n", CDB_VERSION);

    // Initialize editor state
    editor_init();

    // Initialize Dear ImGui UI (creates window and starts event loop on macOS)
    editor_ui_init();

    // Run the application (on macOS this starts NSApp's event loop)
    // This function doesn't return until the application quits
    editor_run();

    // Shutdown (only reached when the app terminates)
    editor_ui_shutdown();
    editor_shutdown();

    printf("CastleDB Editor closed.\n");
    return 0;
}
