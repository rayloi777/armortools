// Test Minic Plugin - Simple draw based

static int g_counter = 0;
static float g_sum = 0.0f;

void on_update() {
    g_counter++;
    g_sum += 0.01f;
    if (g_counter > 100) {
        g_counter = 0;
        g_sum = 0.0f;
    }
}

float on_ui() {
    // Simple calculation - just return a value
    // The actual drawing is done in C code (main.c)
    return g_sum;
}

float main() {
    test_plugin_create();
    test_plugin_register_on_update(on_update);
    test_plugin_register_on_ui(on_ui);
    console_info("Draw Plugin loaded!");
    return g_sum;
}
