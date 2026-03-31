---
name: build
description: Build the IronGame engine (macOS Metal). Use 'run' arg to also launch, 'clean' for clean build.
---

Build the IronGame engine for macOS Metal. Arguments: "$ARGUMENTS"

Steps:
1. Export assets and shaders:
   Run: cd /Users/user/Documents/GitHub/armortools/engine && ../base/make macos metal

2. Compile C code:
   If the argument contains "clean", first run:
   cd /Users/user/Documents/GitHub/armortools/engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release clean
   Then run:
   cd /Users/user/Documents/GitHub/armortools/engine/build && xcodebuild -project IronGame.xcodeproj -configuration Release build

3. If the argument contains "run", launch the binary:
   /Users/user/Documents/GitHub/armortools/engine/build/build/Release/IronGame.app/Contents/MacOS/IronGame

Report build errors if any step fails.
