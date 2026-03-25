---
status: awaiting_human_verify
trigger: "Metal backend depth testing: cube disappears with compare_mode='less', appears with compare_mode='always'"
created: 2026-03-15T00:00:00Z
updated: 2026-03-15T00:00:00Z
---

## Current Focus

hypothesis: "Matrix multiplication order is reversed - code computes View * Projection * World instead of Projection * View * World"
test: "Verify the multiplication order in engine.c lines 1143 and 1660"
expecting: "Confirming reversed order causes incorrect depth values"
next_action: "Apply fix to engine.c and verify cube renders with depth testing"

## Symptoms

expected: "Cube should render with depth testing enabled (compare_mode='less')"
actual: "Cube disappears when compare_mode='less', renders when compare_mode='always'"
errors: "None"
reproduction: 
- "Build cube test with Metal backend on macOS"
- "Set compare_mode='less' in shader context"
- "Cube disappears"
- "Set compare_mode='always' - cube appears"
started: "During depth testing investigation"

## Eliminated

## Evidence

- timestamp: "2026-03-15"
  checked: "Context provided by user"
  found: "Depth buffer IS being created (depth_bits=32)"
  implication: "Depth testing infrastructure exists"

- timestamp: "2026-03-15"
  checked: "Context provided by user"
  found: "Metal depth test WORKS when depth value is manually set to < 1.0 in shader (pos.z=0.0)"
  implication: "Metal depth testing itself works correctly"

- timestamp: "2026-03-15"
  checked: "Context provided by user"
  found: "WVP matrix produces depth=1.0 (outputting depth to color gives white = 1.0)"
  implication: "WVP is producing maximum depth (1.0), which fails 'less' test"

- timestamp: "2026-03-15"
  checked: "Context provided by user"
  found: "Manual z < 1.0 in shader causes depth test to pass and cube shows"
  implication: "Root cause is WVP not producing correct NDC depth values"

- timestamp: "2026-03-15"
  checked: "engine.c matrix multiplication"
  found: "Line 1143: vp = mat4_mult_mat(v, p) computes View * Projection. Line 1660: WVP = mat4_mult_mat(vp, world) computes (View * Projection) * World"
  implication: "MULTIPLICATION ORDER IS WRONG - should be Projection * View * World, not View * Projection * World"

## Resolution

root_cause: "Matrix multiplication order reversed in engine.c line 1143 - computing View * Projection instead of Projection * View"
fix: "Changed engine.c line 1143 from mat4_mult_mat(raw->v, raw->p) to mat4_mult_mat(raw->p, raw->v)"
verification: "Build succeeded, need human to run test and verify cube renders with compare_mode='less'"
files_changed: ["sources/engine.c"]
