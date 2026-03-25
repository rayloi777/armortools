# Technology Stack

**Project:** Analysis Engine (C Code Analysis Tool)
**Researched:** 2025-03-14

## Recommended Stack

### Core Parser/AST Engine

| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| **Clang (LLVM)** | 19.x+ | C parser and AST generation | Industry standard for C/C++ analysis. Provides full C11/C17/C23 support, accurate AST, source location tracking, and semantic analysis. Powers Clang Static Analyzer, clang-tidy, and most production C tools. |
| **Tree-sitter** | 0.24.x | Lightweight incremental parsing | Alternative for simpler analysis needs. Faster startup, incremental parsing (re-parses only changed portions), embeddable C library. Good for IDE-like features, syntax highlighting, or when LLVM is too heavy. |

### Analysis Framework

| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| **Clang LibTooling** | (via Clang 19.x) | Build standalone analysis tools | Provides CompilationDatabase support, ASTConsumer/FrontendAction architecture, RecursiveASTVisitor for AST traversal. The standard way to build Clang-based tools. |
| **Clang AST Matchers** | (via Clang 19.x) | Declarative pattern matching | Write concise, readable analysis rules. More maintainable than manual AST traversal for complex patterns. |

### Supporting Libraries

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| **LLVM/Clang libraries** | 19.x | Core analysis infrastructure | Required for Clang-based approach |
| **tree-sitter-c** | 0.24.x | C grammar for tree-sitter | If using tree-sitter approach |
| **nlohmann/json** | 3.11.x | JSON output generation | For machine-readable reports |
| **inja** | 3.4.x | Template engine | For human-readable report generation |
| **fmt** | 11.x | String formatting | Better error messages and logging |

## Alternative Approaches

### Option A: Full Clang-Based (RECOMMENDED)

```
┌─────────────────────────────────────────┐
│         Analysis Engine                 │
├─────────────────────────────────────────┤
│  CLI Frontend                           │
│  (argparse, exit codes)                │
├─────────────────────────────────────────┤
│  LibTooling / CompilationDatabase       │
│  (build system integration)            │
├─────────────────────────────────────────┤
│  ASTConsumer + RecursiveASTVisitor      │
│  (custom analysis logic)                │
├─────────────────────────────────────────┤
│  Clang 19.x AST / Semantic Analysis     │
├─────────────────────────────────────────┤
│  Clang 19.x Parser (C11/C17/C23)       │
└─────────────────────────────────────────┘
```

**Pros:**
- Full semantic analysis (types, symbols, call graph)
- Accurate source locations for reporting
- Production-proven (clang-tidy, scan-build)
- Supports complex analysis (data flow, pointer aliasing)

**Cons:**
- Heavy dependency (~500MB LLVM)
- Complex build system
- Requires compilation database for full analysis

**Best for:** Deep analysis, bug detection, dependency graphs, complexity metrics

### Option B: Tree-sitter Based

```
┌─────────────────────────────────────────┐
│         Analysis Engine                 │
├─────────────────────────────────────────┤
│  CLI Frontend                           │
│  (argparse, exit codes)                │
├─────────────────────────────────────────┤
│  Analysis Logic                         │
│  (tree queries, custom traversal)      │
├─────────────────────────────────────────┤
│  tree-sitter 0.24.x                    │
│  (incremental CST)                      │
├─────────────────────────────────────────┤
│  tree-sitter-c (C grammar)              │
└─────────────────────────────────────────┘
```

**Pros:**
- Lightweight (~2MB)
- Fast incremental parsing
- Easy to embed
- Good for syntax-level analysis

**Cons:**
- Concrete Syntax Tree (not Abstract) - more complex traversal
- No semantic analysis (types, symbols)
- Limited to syntactic patterns

**Best for:** Quick analysis, syntax checking, code formatting, incremental IDE features

### Option C: Hybrid Approach

Start with tree-sitter for initial parsing, use Clang for deep semantic analysis on-demand.

**Not recommended for v1** - adds complexity without clear benefit.

## What NOT to Use and Why

| Technology | Why Avoid | Alternative |
|------------|-----------|-------------|
| **gcc -fanalyzer** | Limited to GCC, less extensible | Clang Static Analyzer |
| **splint** | Outdated, unmaintained | cppcheck or Clang |
| **Regex-based parsing** | Fragile, doesn't handle C preprocessor | AST-based tools |
| **Frama-C** | Academic tool, heavy formal methods | Clang for practical analysis |
| **EDG (商用)** | Proprietary, expensive | Clang (open source) |
| **CPPcheck alone** | Good for bug detection, not for custom analysis | Use cppcheck + custom Clang tool |

## Installation

### Clang-Based Approach (Recommended)

```bash
# Install Clang 19.x (via LLVM)
# macOS
brew install llvm@19

# Linux (apt)
apt-get install clang-19 llvm-19 llvm-19-dev

# Verify
clang-19 --version

# For build integration, generate compile_commands.json
# For CMake projects:
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .

# For other build systems:
# Use bear (https://github.com/rizsotto/Bear) to generate compile_commands.json
bear -- make
```

### Dependencies for Analysis Engine

```bash
# C++ build tools
cmake >= 3.20
ninja or make

# If using JSON output
# nlohmann/json via vcpkg or CMake FetchContent
```

## Key Decisions for This Project

Based on PROJECT.md requirements:

| Requirement | Recommended Approach |
|-------------|---------------------|
| AST-based analysis | Use Clang LibTooling |
| Incremental analysis | Generate and cache Clang AST |
| Dependency graphs | Use Clang's AST + Decl/Ref traversal |
| 30-second performance | Clang is fast enough for ~50 files |
| JSON output | nlohmann/json |
| Multi-platform | Clang supports Windows/Linux/macOS |

## Sources

- [Clang Static Analyzer Documentation](https://clang.llvm.org/docs/ClangStaticAnalyzer.html) - HIGH confidence
- [Clang LibTooling Documentation](https://clang.llvm.org/docs/LibTooling.html) - HIGH confidence  
- [Clang AST Introduction](https://clang.llvm.org/docs/IntroductionToTheClangAST.html) - HIGH confidence
- [Tree-sitter Documentation](https://tree-sitter.github.io/tree-sitter/) - HIGH confidence
- [tree-sitter-c Repository](https://github.com/tree-sitter/tree-sitter-c) - HIGH confidence (v0.24.1, May 2025)
- [Static Analysis Tools List](https://github.com/analysis-tools-dev/static-analysis) - MEDIUM confidence (community-maintained)
- [Clang RecursiveASTVisitor Tutorial](https://clang.llvm.org/docs/RAVFrontendAction.html) - HIGH confidence
