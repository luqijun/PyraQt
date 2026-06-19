# Release Guide

## Scope

Phase 4 provides the first release pipeline for PyraQt. It supports local CPack packages, CI artifact upload, update-check placeholders, and basic crash recovery markers.

## Local Packages

Configure and build a release tree:

```bash
cmake --preset linux-release-qt-auto
cmake --build --preset linux-release-qt-auto
```

Generate packages:

```bash
cmake --build --preset linux-release-qt-auto --target package
```

On Linux, CPack generates:

- `PyraQt-<version>-Linux.deb`
- `PyraQt-<version>-Linux.tar.gz`
- `PyraQt-<version>-Linux.zip`

## CI Artifacts

The GitHub Actions workflow builds and tests Linux, macOS, and Windows. The Linux Qt5 job also runs the `package` target and uploads the generated DEB, TGZ, and ZIP files as `pyraqt-linux-qt5-packages`.

## Update Placeholder

`UpdateManager` currently records the last check time and returns `NotConfigured`. It does not contact a server, download updates, or install packages. A production release should add a signed update feed, transport policy, channel handling, and user confirmation flow.

## Crash Recovery

`CrashRecoveryManager` stores a clean-exit marker in the user config. If the previous run did not mark a normal exit, the next launch writes a line to `crash.log` and shows a recovery prompt. It is not a crash dump collector.

## Pre-Release Checklist

- Run `cmake --preset linux-release-qt-auto`.
- Run `cmake --build --preset linux-release-qt-auto`.
- Run `ctest --preset linux-debug-qt-auto` or equivalent release tests.
- Run `cmake --build --preset linux-release-qt-auto --target package`.
- Install or unpack each package and smoke-test launch, scripts, plugins, Command Palette, update check, and recovery prompt.
- Add a formal license file before public distribution.
