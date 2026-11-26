# Step 1.2 — Project Initialization

Project: The Hollow Japan | GDD baseline: v1.0.0 | Status: implementation pending verification.

Goal: compile a minimal C++23 executable with CMake and Ninja before integrating SDL3 and vcpkg in Step 1.3. Step 1.1 completion is user-reported.

## Workspace

```bash
cd ~/Personal/project/cpp/the-hollow-japan
git status
mkdir -p src/client
```

## CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.28)
project(TheHollowJapan VERSION 0.1.0 DESCRIPTION "A Japanese fantasy open-world RPG" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(thj_client src/client/main.cpp)
target_compile_features(thj_client PRIVATE cxx_std_23)
if(MSVC)
    target_compile_options(thj_client PRIVATE /W4 /permissive-)
else()
    target_compile_options(thj_client PRIVATE -Wall -Wextra -Wpedantic)
endif()
```

## CMakePresets.json

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "linux-debug",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/linux-debug",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" }
    },
    {
      "name": "linux-release",
      "inherits": "linux-debug",
      "binaryDir": "${sourceDir}/build/linux-release",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Release" }
    }
  ],
  "buildPresets": [
    { "name": "linux-debug", "configurePreset": "linux-debug" },
    { "name": "linux-release", "configurePreset": "linux-release" }
  ]
}
```

## src/client/main.cpp

```cpp
#include <iostream>

int main() {
    std::cout << "The Hollow Japan\n"
              << "Beyond the living world, something waits.\n";
    return 0;
}
```

## .gitignore (merge with existing rules)

```gitignore
/build/
/compile_commands.json
/.cache/
/CMakeUserPresets.json
```

## .clang-format

```yaml
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 100
```

## Build and run

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
./build/linux-debug/thj_client
cmake --preset linux-release
cmake --build --preset linux-release
./build/linux-release/thj_client
```

Expected: title and tagline; exit status zero. No SDL window yet. Optional clangd setup: `ln -s build/linux-debug/compile_commands.json compile_commands.json` if no file or symlink exists at that path.

## Acceptance criteria

- [ ] Debug and Release configure and build successfully.
- [ ] Both executables print expected output and exit zero.
- [ ] Build artifacts are ignored by Git.
- [ ] Source and build configuration reviewed.
- [ ] Git checkpoint committed.

## Git checkpoint

```bash
git add CMakeLists.txt CMakePresets.json .gitignore .clang-format src/client/main.cpp docs/development/01-foundation/02-project-initialization.md
git diff --cached --stat
git commit -m "build: initialize C++23 project"
```

Push with `git push origin HEAD` if the remote is configured. Next: Step 1.3 Dependency Management. No dependency installation or actual local build is claimed by this document.
