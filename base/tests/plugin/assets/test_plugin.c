// Test Minic Plugin - Interactive button test

static float g_button_x = 100.0f;
static float g_button_y = 100.0f;
static float g_button_w = 120.0f;
static float g_button_h = 40.0f;
static int g_clicked = 0;

void on_ui() {
    // Check mouse click on button
    if (mouse_started("left")) {
        float mx = mouse_x;
        float my = mouse_y;
        if (mx >= g_button_x && mx <= g_button_x + g_button_w &&
            my >= g_button_y && my <= g_button_y + g_button_h) {
            g_clicked = 1;
        }
    }

    // Draw button background
    if (g_clicked) {
        draw_set_color(0xff205d9c);
        g_clicked = 0;
    } else {
        draw_set_color(0xff323232);
    }
    draw_filled_rect(g_button_x, g_button_y, g_button_w, g_button_h);

    // Draw button border
    draw_set_color(0xff555555);
    draw_rect(g_button_x, g_button_y, g_button_w, g_button_h, 1.0f);
}

float main() {
    test_plugin_create();
    test_plugin_register_on_ui(on_ui);
    console_info("Plugin loaded!");
    return 0.0;
}
