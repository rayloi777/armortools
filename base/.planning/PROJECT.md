# Analysis Engine

## What This Is

A code analysis and static analysis tool for the iron 3D engine codebase. Provides insights into code quality, complexity metrics, dependency analysis, and potential issues in the C/JavaScript codebase.

## Core Value

Enable developers to understand, maintain, and improve the iron 3D engine codebase through automated static analysis and code intelligence.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] Analyze C source files for complexity and quality metrics
- [ ] Generate dependency graphs between modules
- [ ] Detect common C programming issues and anti-patterns
- [ ] Provide actionable reports for code improvements

### Out of Scope

- [Real-time editing] — Analysis only, not a live editor
- [Language servers] — Not implementing LSP protocol for v1

## Context

The iron engine is a C-based 3D engine with JavaScript scripting. Currently lacks any automated code analysis tooling. This project aims to fill that gap.

Technical environment:
- C codebase with ~50+ source files in /sources
- JavaScript runtime (QuickJS) integration
- Multi-platform (Windows, Linux, macOS)
- Multiple graphics backends (Direct3D12, Vulkan, Metal, WebGPU)

## Constraints

- **Performance**: Analysis should complete in under 30 seconds for full codebase
- **Language**: Focus on C code analysis initially, JS as v2
- **Output**: Machine-readable reports (JSON) + human readable summaries

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| CLI-first approach | Simple integration into existing workflows | — Pending |
| AST-based analysis | More accurate than regex-based tools | — Pending |
| Incremental analysis | Avoid re-analyzing unchanged files | — Pending |

---
*Last updated: 2025-03-14 after initialization*