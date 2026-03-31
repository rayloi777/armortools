---
name: debug
description: Launch the IronGame engine under lldb for debugging. Use 'attach' to attach to running process, 'core' to analyze a core dump.
---

Debug the IronGame engine. Arguments: "$ARGUMENTS"

Binary path: /Users/user/Documents/GitHub/armortools/engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame

Steps:

If argument contains "attach":
1. Find the running IronGame process:
   Run: ps aux | grep IronGame | grep -v grep
2. Tell the user the PID and suggest they run in their terminal:
   lldb -p <PID>
   (lldb cannot be used interactively through Claude Code)

Else if argument contains "core":
1. Ask the user for the core dump path if not provided
2. Run: lldb -c <core_path> /Users/user/Documents/GitHub/armortools/engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame

Else (default - launch under debugger):
1. First check the binary exists:
   Run: ls -la /Users/user/Documents/GitHub/armortools/engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame
2. Inform the user that lldb requires an interactive terminal and suggest they run:
   lldb /Users/user/Documents/GitHub/armortools/engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame

   Useful lldb commands to share:
   - break set -n function_name    (set breakpoint)
   - break set -f file.c -l 42     (break at file:line)
   - run                            (start execution)
   - bt                             (backtrace on crash)
   - frame variable                 (show local variables)
   - continue                       (continue execution)
   - expr (type)variable            (evaluate expression)

   For engine-specific debugging:
   - break set -n sys_2d_draw       (break in 2D render system)
   - break set -n sprite_bridge_create_sprite  (break in sprite creation)
   - break set -n minic_printf      (break in script printf)
