#pragma once

void scene_3d_api_register(void);

// Minic material API
minic_val_t minic_material_bind_texture(minic_val_t *args, int argc);
minic_val_t minic_material_set_use_texture(minic_val_t *args, int argc);
