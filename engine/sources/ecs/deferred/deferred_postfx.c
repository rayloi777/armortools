#include "deferred_postfx.h"
#include <iron.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static postfx_t g_postfx = {0};

// Vertex structure matching Iron's draw system (pos: float2)
static gpu_vertex_structure_t postfx_vertex_structure;

// Use sys_get_shader() which correctly resolves the data path
// sys_get_shader expects names like "postfx_ssao.vert" / "postfx_ssao.frag"
static gpu_pipeline_t *create_postfx_pipeline_format(const char *name, int format) {
	char vert_name[128];
	char frag_name[128];
	snprintf(vert_name, sizeof(vert_name), "%s.vert", name);
	snprintf(frag_name, sizeof(frag_name), "%s.frag", name);

	gpu_shader_t *vert = sys_get_shader(vert_name);
	gpu_shader_t *frag = sys_get_shader(frag_name);
	if (!vert || !frag) {
		printf("PostFX: failed to load shader %s (vert=%p frag=%p)\n", name, (void *)vert, (void *)frag);
		return NULL;
	}

	gpu_pipeline_t *pipe = (gpu_pipeline_t *)calloc(1, sizeof(gpu_pipeline_t));
	gpu_pipeline_init(pipe);
	pipe->input_layout = &postfx_vertex_structure;
	pipe->color_attachment[0] = format;
	pipe->blend_source = GPU_BLEND_ONE;
	pipe->blend_destination = GPU_BLEND_INV_SOURCE_ALPHA;
	pipe->alpha_blend_source = GPU_BLEND_ONE;
	pipe->alpha_blend_destination = GPU_BLEND_INV_SOURCE_ALPHA;
	pipe->vertex_shader = vert;
	pipe->fragment_shader = frag;
	gpu_pipeline_compile(pipe);

	printf("PostFX: created pipeline for %s\n", name);
	return pipe;
}

static gpu_pipeline_t *create_postfx_pipeline(const char *name) {
	return create_postfx_pipeline_format(name, GPU_TEXTURE_FORMAT_RGBA64);
}

static gpu_pipeline_t *create_postfx_pipeline_framebuffer(const char *name) {
	return create_postfx_pipeline_format(name, GPU_TEXTURE_FORMAT_RGBA32);
}

void postfx_init(int width, int height) {
	if (g_postfx.initialized) postfx_shutdown();

	// Initialize vertex structure (matches Iron's draw system)
	postfx_vertex_structure.size = 0;
	gpu_vertex_structure_add(&postfx_vertex_structure, "pos", GPU_VERTEX_DATA_F32_2X);

	int hw = width / 2;
	int hh = height / 2;

	g_postfx.ao_result = gpu_create_render_target(hw, hh, GPU_TEXTURE_FORMAT_RGBA64);

	// Bloom downsample chain: 1/2, 1/4, 1/8, 1/16
	int bw = width;
	int bh = height;
	for (int i = 0; i < 4; i++) {
		bw = bw > 1 ? bw / 2 : 1;
		bh = bh > 1 ? bh / 2 : 1;
		g_postfx.bloom_down[i] = gpu_create_render_target(bw, bh, GPU_TEXTURE_FORMAT_RGBA64);
		g_postfx.bloom_up[i] = gpu_create_render_target(bw, bh, GPU_TEXTURE_FORMAT_RGBA64);
	}

	g_postfx.scene_copy = gpu_create_render_target(width, height, GPU_TEXTURE_FORMAT_RGBA64);

	// Load shader pipelines
	g_postfx.ssao_pipeline = create_postfx_pipeline("postfx_ssao");
	g_postfx.bloom_down_pipeline = create_postfx_pipeline("postfx_bloom_down");
	g_postfx.bloom_up_pipeline = create_postfx_pipeline("postfx_bloom_up");
	g_postfx.composite_pipeline = create_postfx_pipeline_framebuffer("postfx_compositor");

	g_postfx.width = width;
	g_postfx.height = height;
	g_postfx.ssao_enabled = true;
	g_postfx.bloom_enabled = true;
	g_postfx.ssao_radius = 0.5f;
	g_postfx.ssao_strength = 0.8f;
	g_postfx.bloom_threshold = 0.8f;
	g_postfx.bloom_strength = 0.3f;
	g_postfx.initialized = true;

	printf("PostFX initialized: %dx%d (SSAO half: %dx%d)\n", width, height, hw, hh);
}

void postfx_resize(int width, int height) {
	if (!g_postfx.initialized) return;
	if (g_postfx.width == width && g_postfx.height == height) return;
	gpu_pipeline_t *ssao = g_postfx.ssao_pipeline;
	gpu_pipeline_t *bd = g_postfx.bloom_down_pipeline;
	gpu_pipeline_t *bu = g_postfx.bloom_up_pipeline;
	gpu_pipeline_t *comp = g_postfx.composite_pipeline;

	g_postfx.ao_result = NULL;
	for (int i = 0; i < 4; i++) {
		g_postfx.bloom_down[i] = NULL;
		g_postfx.bloom_up[i] = NULL;
	}
	g_postfx.scene_copy = NULL;
	g_postfx.initialized = false;

	int hw = width / 2;
	int hh = height / 2;
	g_postfx.ao_result = gpu_create_render_target(hw, hh, GPU_TEXTURE_FORMAT_RGBA64);
	int bw = width, bh = height;
	for (int i = 0; i < 4; i++) {
		bw = bw > 1 ? bw / 2 : 1;
		bh = bh > 1 ? bh / 2 : 1;
		g_postfx.bloom_down[i] = gpu_create_render_target(bw, bh, GPU_TEXTURE_FORMAT_RGBA64);
		g_postfx.bloom_up[i] = gpu_create_render_target(bw, bh, GPU_TEXTURE_FORMAT_RGBA64);
	}
	g_postfx.scene_copy = gpu_create_render_target(width, height, GPU_TEXTURE_FORMAT_RGBA64);

	g_postfx.ssao_pipeline = ssao;
	g_postfx.bloom_down_pipeline = bd;
	g_postfx.bloom_up_pipeline = bu;
	g_postfx.composite_pipeline = comp;
	g_postfx.width = width;
	g_postfx.height = height;
	g_postfx.initialized = true;
}

void postfx_shutdown(void) {
	if (!g_postfx.initialized) return;
	g_postfx.ao_result = NULL;
	for (int i = 0; i < 4; i++) {
		g_postfx.bloom_down[i] = NULL;
		g_postfx.bloom_up[i] = NULL;
	}
	g_postfx.scene_copy = NULL;
	g_postfx.ssao_pipeline = NULL;
	g_postfx.bloom_down_pipeline = NULL;
	g_postfx.bloom_up_pipeline = NULL;
	g_postfx.composite_pipeline = NULL;
	g_postfx.initialized = false;
	printf("PostFX shutdown\n");
}

postfx_t *postfx_get(void) {
	return &g_postfx;
}
