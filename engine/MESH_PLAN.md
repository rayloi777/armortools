# Mesh Rendering Test Plan (Option A)

## Goal

Create a programmatic cube mesh and verify the ECS→Iron bridge works correctly.

## Implementation Steps

### Phase 1: Add programmatic mesh creation (engine layer)

**1.1 Create `engine/sources/core/mesh_api.h`** - Header for mesh creation functions
```c
#pragma once
#include <stdint.h>

typedef struct mesh_data mesh_data_t;

mesh_data_t *mesh_create_cube(float size);
```

**1.2 Create `engine/sources/core/mesh_api.c`** - Implement cube mesh creation
- Define 8 cube vertices (corners) with positions, normals, and UVs
- Define 36 indices (12 triangles, 2 per face)
- Convert float positions to int16 (short4norm format)
- Build vertex_array_t structures for pos, nor, uv
- Build index_array_t (u32)
- Call `mesh_data_create()` from base to build GPU buffers

**1.3 Modify `engine/sources/ecs/ecs_bridge.c`** - Handle built-in meshes
- In `bridge_system()`, check if `mesh[i].mesh_path` starts with `:`
- If `:cube`, call `mesh_create_cube()` instead of `data_get_mesh()`
- If `mesh[i].material_path` starts with `:`, use default material

**1.4 Register mesh_api in `engine/sources/game_engine.c`**
- Include mesh_api.h
- Call mesh_api_init() during game_engine_init()

### Phase 2: Create mesh test Minic script

**2.1 Create `engine/assets/systems/mesh_system.minic`**
```c
// MeshSystem - Creates and animates a cube entity

int init() {
    printf("MeshSystem: Creating cube entity\n");
    
    // Register built-in components if not already registered
    let pos_type = component_register("TransformPosition", 12);
    component_add_field(pos_type, "x", TYPE_FLOAT, 0);
    component_add_field(pos_type, "y", TYPE_FLOAT, 4);
    component_add_field(pos_type, "z", TYPE_FLOAT, 8);
    
    let rot_type = component_register("TransformRotation", 16);
    component_add_field(rot_type, "x", TYPE_FLOAT, 0);
    component_add_field(rot_type, "y", TYPE_FLOAT, 4);
    component_add_field(rot_type, "z", TYPE_FLOAT, 8);
    component_add_field(rot_type, "w", TYPE_FLOAT, 12);
    
    let scale_type = component_register("TransformScale", 12);
    component_add_field(scale_type, "x", TYPE_FLOAT, 0);
    component_add_field(scale_type, "y", TYPE_FLOAT, 4);
    component_add_field(scale_type, "z", TYPE_FLOAT, 8);
    
    let render_obj_type = component_register("RenderObject", 24);
    // field: object (ptr), transform (ptr), dirty (int/bool)
    
    // Create cube entity
    let cube = entity_create_named("Cube");
    
    // Add transform components
    let pos = entity_get(cube, "TransformPosition");
    pos.x = 0.0; pos.y = 0.0; pos.z = -5.0;
    
    let rot = entity_get(cube, "TransformRotation");
    rot.x = 0.0; rot.y = 0.0; rot.z = 0.0; rot.w = 1.0;
    
    let scale = entity_get(cube, "TransformScale");
    scale.x = 1.0; scale.y = 1.0; scale.z = 1.0;
    
    // Add RenderMesh with built-in cube mesh (":cube" path triggers programmatic creation)
    // This requires adding RenderMesh support in entity API
    
    printf("MeshSystem: Cube entity created\n");
    return 0;
}

int step() {
    // Rotate cube over time (if we had direct transform access)
    return 0;
}
```

**2.2 Modify `engine/assets/scripts/game.minic`**
- Include mesh_system initialization

**2.3 Modify `game_engine.c`** to load mesh_system.minic
- Add: `minic_system_load("MeshSystem", "data/systems/mesh_system.minic");`

### Phase 3: Build and test

**3.1 Build base**
```bash
cd base && ./make macos metal
```

**3.2 Build engine**
```bash
cd engine && ../base/make macos metal
```

**3.3 Run and verify**
- Should see a rotating/visible cube
- Check console for MeshSystem initialization messages

## Key Files to Modify/Create

| File | Action |
|------|--------|
| `engine/sources/core/mesh_api.h` | Create |
| `engine/sources/core/mesh_api.c` | Create |
| `engine/sources/ecs/ecs_bridge.c` | Modify |
| `engine/sources/game_engine.c` | Modify |
| `engine/sources/core/runtime_api.c` | Modify (add RenderMesh support) |
| `engine/assets/systems/mesh_system.minic` | Create |
| `engine/assets/scripts/game.minic` | Modify |

## Risks and Mitigations

1. **Risk**: `mesh_data_create()` from base might have assumptions about data format
   - **Mitigation**: Start with simple cube (just positions), add normals/UVs later

2. **Risk**: Bridge might not sync transforms correctly
   - **Mitigation**: Add debug printf statements to verify transform values

3. **Risk**: Default material might not render anything visible
   - **Mitigation**: Use a simple unlit material or add basic material creation

## Technical Notes

### mesh_data_t structure (from base/engine.h)
```c
typedef struct mesh_data {
    char                   *name;
    float                   scale_pos;
    float                   scale_tex;
    vertex_array_t_array_t *vertex_arrays;  // array of vertex_array_t*
    u32_array_t            *index_array;
    mesh_data_runtime_t    *_;  // runtime data (filled by mesh_data_create)
} mesh_data_t;

typedef struct vertex_array_t_array {
    struct vertex_array **buffer;
    int                   length;
    int                   capacity;
} vertex_array_t_array_t;

typedef struct vertex_array {
    char     *attrib;  // "pos", "nor", "uv"
    char     *data;    // "short4norm" or "short2norm"
    i16_array_t *values;
} vertex_array_t;
```

### Array creation functions (from base/iron_array.h)
```c
i16_array_t  *i16_array_create(uint32_t length);
i16_array_t  *i16_array_create_from_raw(int16_t *raw, uint32_t length);
u32_array_t  *u32_array_create(uint32_t length);
u32_array_t  *u32_array_create_from_raw(uint32_t *raw, uint32_t length);
any_array_t  *any_array_create(uint32_t length);
```

### mesh_data functions (from base/engine.h/engine.c)
```c
mesh_data_t *mesh_data_create(mesh_data_t *raw);  // builds GPU buffers
void         mesh_data_delete(mesh_data_t *raw);  // frees GPU buffers
```

### Vertex format: short4norm
- `short4norm` = int16 normalized to [-1, 1]
- `short2norm` = int16 normalized to [0, 1] (for UVs)
- GPU automatically unpacks these to float in Metal
