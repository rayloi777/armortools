// Test Minic Plugin

void *plugin;
ui_handle_t *h0;
ui_handle_t *h1;
ui_handle_t *h2;
ui_handle_t *h3;
ui_handle_t *h4;
ui_handle_t *h5;

void on_ui() {
    if (ui_panel(h0, "Test Plugin", false, false, false)) {
        ui_text("Hello from Minic Plugin!", 0, 0);

        if (ui_button("Click Me", UI_ALIGN_CENTER, "")) {
            console_info("Button clicked!");
        }

        ui_slider(h1, "Slider", 0, 1, true, 100, true, UI_ALIGN_LEFT, true);

        ui_check(h2, "Check Me", "");

        {
            string_array_t *items = string_array_create(3);
            items->buffer[0] = "Item 1";
            items->buffer[1] = "Item 2";
            items->buffer[2] = "Item 3";
            ui_combo(h3, items, "Combo", true, UI_ALIGN_LEFT, true);
        }

        ui_radio(h4, 0, "Radio 1", "");
        ui_radio(h4, 1, "Radio 2", "");
        ui_radio(h4, 2, "Radio 3", "");
    }
}

void main() {
    plugin = test_plugin_create();
    h0 = ui_handle_create();
    h1 = ui_handle_create();
    h2 = ui_handle_create();
    h3 = ui_handle_create();
    h4 = ui_handle_create();
    gc_root(plugin);
    gc_root(h0);
    gc_root(h1);
    gc_root(h2);
    gc_root(h3);
    gc_root(h4);
    test_plugin_register_on_ui(on_ui);
    console_info("Test plugin loaded!");
}
