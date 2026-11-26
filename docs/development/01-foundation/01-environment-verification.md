# Step 1.1 — Development Environment Verification

Status: Pending local verification. No engine code or package installation in this step.

## Goal

Verify Arch Linux x86-64, toolchain, storage and Git readiness before initializing the C++23 project. Development baseline: GCC >=14 or Clang >=18, CMake >=3.28, Ninja, Git. Exact dependency pins will be established in Step 1.3. Recommended free space >=20 GiB on the intended workspace/build/cache filesystem (planning allowance, not hard library requirement); maintain >=5 GiB root headroom where feasible. Stop before installation if space is insufficient.

## Read-only checks

```bash
cat /etc/os-release
uname -m
uname -r
lscpu | grep -E 'Model name|CPU\(s\)|Architecture'
free -h
df -h / "$HOME"
for tool in git gcc g++ clang++ cmake ninja pkg-config curl unzip zip tar python3; do
  if command -v "$tool" >/dev/null 2>&1; then
    printf '\n--- %s ---\n' "$tool"
    "$tool" --version 2>&1 | head -n 1
  else
    printf '%s: MISSING\n' "$tool"
  fi
done
pacman -Q base-devel 2>/dev/null || true
```

## Repository

Use the existing project directory if present; otherwise create one in a filesystem with sufficient space. Run `git status --short --branch`, `git remote -v` (redact embedded credentials), and `df -h .`. For a new repository only, `git init -b main`. Do not overwrite existing work or reset a branch. Verify Git identity using `git config user.name` and `git config user.email` locally; no need to share personal email.

## Compiler smoke test

```bash
tmpdir=$(mktemp -d)
cat > "$tmpdir/check.cpp" <<'EOF'
#include <expected>
#include <iostream>
int main() {
    std::expected<int, const char*> result = 23;
    std::cout << "C++23 smoke test: " << result.value() << '\n';
}
EOF
"${CXX:-g++}" -std=c++23 -Wall -Wextra -pedantic "$tmpdir/check.cpp" -o "$tmpdir/check" && "$tmpdir/check"
rm -f "$tmpdir/check.cpp" "$tmpdir/check"
rmdir "$tmpdir"
```

Use `CXX=clang++` in the shell (export it) if Clang is the selected compiler. This tests one required library feature, not all C++23 support.

## Acceptance criteria

- Arch Linux x86-64 verified; supported compiler and CMake baseline verified.
- Git, Ninja and required utilities identified; missing tools resolved before next step.
- Workspace and root storage reviewed; no destructive cleanup or partial Arch upgrade.
- C++23 smoke test passes.
- Repository state and identity reviewed; actual outputs recorded without credentials.

## Checkpoint

After successful verification record actual results in this document, then `git add docs/development/01-foundation/01-environment-verification.md` and `git commit -m "docs(dev): record verified development environment"`. Do not claim verification before local execution. Next: Step 1.2 minimal CMake project, not all dependencies.
