# Release Engineering

## What ships

Every release has four self-contained archives:

| Platform | Architecture | Archive |
| --- | --- | --- |
| macOS | Apple Silicon (`arm64`) | `snake-puzzle-<version>-macos-arm64.tar.gz` |
| macOS | Intel (`x86_64`) | `snake-puzzle-<version>-macos-x86_64.tar.gz` |
| Linux | `x86_64` | `snake-puzzle-<version>-linux-x86_64.tar.gz` |
| Windows | `x86_64` | `snake-puzzle-<version>-windows-x86_64.zip` |

Each archive contains:

```text
snake-puzzle-<version>-<platform>/
├── snake_puzzle(.exe)
├── run.sh / run.command / run.bat
├── assets/atoms/          # runtime puzzle registry
├── docs/
├── README.md
├── LICENSE
├── THIRD_PARTY_NOTICES.md
├── licenses/
└── QUICKSTART.txt
```

The release build compiles raylib 6.0 from source and links it statically.
No raylib installation is needed on the player's machine. raylib and GLFW
licenses are bundled in `licenses/` and `THIRD_PARTY_NOTICES.md`.

## CI matrix

`.github/workflows/release.yml` runs on:

- `macos-15` for Apple Silicon (`arm64`)
- `macos-15-intel` for Intel (`x86_64`)
- `ubuntu-22.04` for Linux `x86_64`
- `windows-2022` with MSYS2 MinGW-w64 UCRT64 for Windows `x86_64`

The Linux build links dynamically to common desktop libraries (X11, OpenGL,
ALSA), so it is built on Ubuntu 22.04 for broad glibc compatibility. Windows
uses `-static -static-libgcc` so GCC runtime DLLs are not required. macOS
builds are unsigned because the repository does not use an Apple Developer
certificate.

## Triggering a release

```bash
git tag -a v1.1.0 -m "Snake Puzzle v1.1.0"
git push origin v1.1.0
```

The tag starts the release workflow. When all four native build jobs pass:

1. packages are downloaded into one release job;
2. `SHA256SUMS` is generated;
3. GitHub Release `v1.1.0` is created or updated with all archives.

A manual `workflow_dispatch` run builds all four packages and keeps them as
Actions artifacts, but does not publish a release.

Check progress:

```bash
gh run list --workflow release.yml
gh run watch
gh release view v1.1.0
```

## Platform support

- macOS: 11+; arm64 and x86_64 separate builds.
- Linux: x86_64, built on Ubuntu 22.04 (glibc 2.35+), common X11/OpenGL/ALSA libs.
- Windows: 10/11 x64; UCRT runtime included by the OS; no MinGW runtime DLLs.

## Reproducing a package locally

macOS (current architecture):

```bash
cmake -S . -B build-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DSP_BUILD_TESTS=OFF \
  -DSP_RELEASE_VERSION=1.1.0
cmake --build build-release --parallel
python3 tools/package_release.py \
  --binary build-release/snake_puzzle \
  --platform macos-arm64 \
  --version 1.1.0 \
  --output-dir dist
```

Linux:

```bash
sudo apt install libasound2-dev libx11-dev libxrandr-dev libxi-dev \
  libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev \
  libwayland-dev libxkbcommon-dev
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DSP_BUILD_TESTS=OFF
cmake --build build-release --parallel
python3 tools/package_release.py \
  --binary build-release/snake_puzzle \
  --platform linux-x86_64 --version 1.1.0 --output-dir dist
```

Windows (MSYS2 UCRT64 shell):

```bash
cmake -S . -B build-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DSP_BUILD_TESTS=OFF \
  -DSP_RELEASE_VERSION=1.1.0
cmake --build build-release --parallel
python tools/package_release.py \
  --binary build-release/snake_puzzle.exe \
  --platform windows-x86_64 --version 1.1.0 --output-dir dist
```

## Asset discovery

The executable resolves its atom registry in this order:

1. `--atoms PATH`
2. `SNAKE_PUZZLE_ATOMS`
3. `assets/atoms` relative to the current directory
4. `assets/atoms` next to the executable
5. `../Resources/assets/atoms` (inside a macOS app bundle)
6. `../share/snake-puzzle/assets/atoms` (system install layout)

This means the archive works even when launched from another directory.

## Security and signing

- Archives are not code-signed or notarized.
- macOS Gatekeeper may require right-click → Open the first time.
- Windows SmartScreen may warn about an unsigned executable.
- Verify downloads with the attached `SHA256SUMS`:

```bash
sha256sum -c SHA256SUMS
```

## Release checklist

- [ ] `make check` passes locally.
- [ ] `ctest` passes via CMake.
- [ ] Version in `CMakeLists.txt` / `SP_RELEASE_VERSION` matches the tag.
- [ ] Tag is pushed only after the release workflow exists on `main`.
- [ ] All four build jobs pass.
- [ ] `SHA256SUMS` is attached.
- [ ] Release notes list platform requirements and unsigned-build caveat.
