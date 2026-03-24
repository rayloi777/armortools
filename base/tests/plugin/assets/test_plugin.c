// Test Minic Plugin - Uses only value-returning functions

static int g_counter = 0;
static float g_sum = 0.0f;

void on_update() {
    g_counter++;
    g_sum += 0.1f;
    if (g_counter > 100) {
        g_counter = 0;
        g_sum = 0.0f;
    }
}

float on_ui() {
    float x = 50.0f;
    float y = 50.0f;

    int panel_open = 1;
    int text_result = 1;
    int button_clicked = 0;
    float slider_value = g_sum;

    return 1.0;
}

float main() {
    test_plugin_create();
    test_plugin_register_on_update(on_update);
    test_plugin_register_on_ui(on_ui);
    console_info("Test plugin loaded!");
    return 0.0;
}
