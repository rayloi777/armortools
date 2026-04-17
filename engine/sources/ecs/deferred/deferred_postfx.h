#pragma once

#include <stdbool.h>

struct gpu_texture_t;
struct gpu_pipeline_t;

typedef struct {
	struct gpu_texture_t *ao_result;       // RGBA64: SSAO output (half-res)
	struct gpu_texture_t *ssao_upsampled;  // RGBA64: SSAO upsampled to full-res
	struct gpu_texture_t *bloom_down[4];   // RGBA64: 1/2, 1/4, 1/8, 1/16
	struct gpu_texture_t *bloom_up[4];     // RGBA64: upsample chain
	struct gpu_texture_t *scene_copy;      // RGBA64: lit scene before bloom
	// Pipelines for postfx shaders
	struct gpu_pipeline_t *ssao_pipeline;
	struct gpu_pipeline_t *bloom_down_pipeline;
	struct gpu_pipeline_t *bloom_up_pipeline;
	struct gpu_pipeline_t *composite_pipeline;
	int width;
	int height;
	bool ssao_enabled;
	bool bloom_enabled;
	float ssao_radius;
	float ssao_strength;
	float bloom_threshold;
	float bloom_strength;
	bool initialized;
} postfx_t;

void postfx_init(int width, int height);
void postfx_resize(int width, int height);
void postfx_shutdown(void);
postfx_t *postfx_get(void);
