#pragma once

// LOD component — auto-switches mesh based on distance to camera
// Each level has a distance threshold and mesh path.
typedef struct {
    float dist0;     // Use lod0_mesh when distance <= dist0
    float dist1;     // Use lod1_mesh when distance <= dist1
    float dist2;     // Use lod2_mesh when distance <= dist2
    char *lod0_mesh; // Closest (highest detail) mesh path
    char *lod1_mesh; // Medium detail mesh path
    char *lod2_mesh; // Lowest detail mesh path (optional, can be NULL)
    int active_lod;  // Current LOD level (0, 1, 2) — set by system
} comp_3d_lod;
