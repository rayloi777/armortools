#pragma once

typedef struct {
    float metallic;
    float roughness;
    float albedo_r, albedo_g, albedo_b;
    float emissive_r, emissive_g, emissive_b;
    float ao;
} comp_3d_material;

void material_init(void);
void material_shutdown(void);