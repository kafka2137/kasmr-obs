# Kafka's Keyboard ASMR for OBS

By **[kafka2137](https://github.com/kafka2137)**.
Repository: **[kafka2137/kasmr-obs](https://github.com/kafka2137/kasmr-obs)**.

A native Windows x64 OBS audio source that plays mechanical keyboard sounds
as you type in any application. Inspired by [tplai/kbsim](https://github.com/tplai/kbsim),
using its original recordings with attribution, plus Gateron G Pro Brown
recordings by kafka2137. No browser source or desktop
audio loopback is needed.

## Install and use

For a setup wizard, run **`dist/kasmr-obs-1.1.0-windows-x64-Installer.exe`**.
Like [Tuna's installer](https://github.com/univrsal/tuna/blob/master/cmake/windows/resources/installer-Windows.iss.in),
it detects the OBS installation folder, lets you change it, checks the OBS version,
and adds an uninstaller to Windows Installed Apps. Close OBS first; the installer
will request administrator permission. Select the OBS root directory, typically
`C:\Program Files\obs-studio`.

The EXE installs the DLL into `obs-plugins/64bit` and its sounds into
`data/obs-plugins/kasmr-obs` beneath that OBS directory. If upgrading from our
earlier ProgramData ZIP installation, it removes that old plugin DLL to prevent
duplicate loading, preserving the old data folder. Remove the EXE installation
using **Windows Settings → Apps → Installed apps → Kafka's Keyboard ASMR for OBS**.
The uninstaller leaves OBS itself, other plugins, and OBS settings alone.

The ZIP remains available for the ProgramData/portable installation method:

1. Extract `dist/kasmr-obs-1.1.0-windows-x64.zip` and close OBS Studio.
2. Copy the extracted `kasmr-obs` folder into
   `C:\ProgramData\obs-studio\plugins`. Alternatively, run the extracted
   `install.ps1` in PowerShell (elevation may be needed for that folder).
3. Open OBS. Under **Sources → +**, choose **Kafka's Keyboard ASMR for OBS**.
4. Choose a switch and keyboard layout, then click **Preview sounds**.
5. Its audio goes into OBS's mixer for recording and streaming. To also hear it
   through headphones, set **Advanced Audio Properties → Audio Monitoring →
   Monitor and Output**, and select your monitoring device in OBS audio settings.
   Avoid capturing that monitoring device again as desktop audio, which can double the sound.

For portable OBS, copy the plugin folder into the portable installation's
`data/plugins` directory. Do not place just the DLL there; keep `bin/64bit` and
`data` together. OBS's [official plugin guide](https://obsproject.com/kb/plugins-guide)
documents these directory conventions.

## Configuration

| Setting | Behavior |
| --- | --- |
| Switch sound | 14 sound sets: Creams, Holy Pandas, Alpacas, Tealios, Black/Red Inks, MX Black/Brown/Blue, Gateron G Pro Browns, Box Navy, Buckling Spring, Blue Alps, Topre |
| Keyboard layout | Full size, TKL, 75%, 65%, or HHKB/60%; keys absent from the chosen layout are silent |
| Keyboard case sound | Original recording, or simulated plastic, aluminum, and wood/dampened tonal profiles |
| Volume | Source gain, also independently adjustable in OBS's mixer |
| Stereo width | Left/right placement based on physical key position; zero centers all keys |
| Pitch variation | Small randomized pitch changes to reduce repetition |
| Key release sounds | Separate upstroke recordings |
| Active scene only | Stop sounds when the source is absent from the active scene; on by default |
| Ignore software events | Ignore injected/synthetic keystrokes; on by default |
| Custom pack | Load a local directory of WAV/MP3 samples instead of a bundled switch |

Space, Enter, and Backspace use dedicated samples where available. Other keys
use row-specific recordings. MX Blue's missing special-key recordings fall back
to its generic sounds. Holding a key does not trigger OS auto-repeat sounds.

The keyboard selection describes the layout, not a selection of a specific USB
device. Case profiles apply pitch/filter changes; they are not recordings of
each named case. There is no visual keyboard overlay or typing test.

## Custom sound packs

Select a folder containing these relative filenames. `.wav` takes priority over
`.mp3`; files should be short, at most three seconds and 8 MB each. Audio is
downmixed and resampled to 48 kHz internally.

```text
press/GENERIC_R0.wav
press/GENERIC_R1.wav
press/GENERIC_R2.wav
press/GENERIC_R3.wav
press/GENERIC_R4.wav
release/GENERIC.wav
press/SPACE.wav          # optional; falls back to GENERIC_R2
press/ENTER.wav          # optional
press/BACKSPACE.wav      # optional
release/SPACE.wav        # optional; falls back to GENERIC
release/ENTER.wav        # optional
release/BACKSPACE.wav    # optional
```

Invalid packs leave the previous sounds loaded and report a warning in the OBS
log. Clear the folder setting to return to bundled sounds. Re-select the folder
or recreate the source after changing files in an already loaded pack.

## Build from source

Requires Windows x64, Visual Studio 2022 with Desktop development with C++,
CMake 3.28+, Git, and OBS Studio 32.x installed. The plugin has no Qt dependency.
Bundled decoder headers and sounds are already included in this repository.

```powershell
./scripts/build.ps1 -Bootstrap
```

To also build the Tuna-style Windows EXE, install
[Inno Setup 6.7 or newer](https://jrsoftware.org/isdl.php), then run:

```powershell
./scripts/build.ps1 -Bootstrap -Installer
# Or package an already staged build:
./scripts/package-windows.ps1 -IsccPath 'C:/Program Files (x86)/Inno Setup 6/ISCC.exe'
```

The template is `cmake/windows/installer-Windows.iss.in`; CMake generates
`build/installer-Windows.generated.iss` using the project version and staging
paths. Run `./tests/installer-tests.ps1` to check invalid-folder and running-OBS rejection,
installation, reinstallation, and safe uninstallation in a temporary workspace
directory. The test compiler mode disables elevation, uninstall registration,
and migration of the real ProgramData installation.

The locally built installer is unsigned.

This downloads official OBS 32.0.4 source headers, builds a Release DLL, runs
core tests and a real libobs smoke test, and creates the installable ZIP under
`dist`, along with a corresponding source ZIP. It does not install or alter your OBS configuration. The fallback build
generates an import library from the installed `obs.dll` exports with Microsoft's
tools. Use a fresh build directory if changing OBS runtime versions.

An official libobs SDK can be supplied with `-SdkPath`; an existing source checkout
with `-ObsSource`; a custom OBS runtime with `-ObsRuntime`. The runtime smoke test
needs that runtime's DLL dependencies available in its bin folder.

Core-only development (does not require OBS):

```powershell
cmake -S . -B build-core -DKASMR_BUILD_PLUGIN=OFF
cmake --build build-core --config Release
ctest --test-dir build-core -C Release --output-on-failure
```

## GitHub Actions releases

The **Windows build and release** workflow builds on pushes to `main`/`master`,
pull requests, and manual runs. Each run downloads pinned, SHA256-verified OBS
and Inno Setup packages, builds the Windows x64 plugin, runs the audio tests at
48/44.1 kHz plus installer tests, and uploads the ZIPs, installer EXE, and
`SHA256SUMS.txt` as workflow artifacts.

To publish a release, update `project(... VERSION ...)` in `CMakeLists.txt`,
commit the changes, then push a matching version tag:

```powershell
git tag v1.1.0
git push origin v1.1.0
```

A successful tag build automatically creates a GitHub Release with the binary
ZIP, source ZIP, Windows installer, and checksums. Re-running the tag workflow
updates those assets. The tag must match the CMake project version exactly.
Manual runs and branch/PR builds only upload workflow artifacts. The workflow
uses the built-in `GITHUB_TOKEN`; no personal access token or signing secret is
required. Packages remain unsigned.

## Design and validation

Windows low-level keyboard hooks run on a dedicated message thread. The hook
only tracks physical scan-code transitions and pushes into a bounded queue;
audio decoding and mixing happen elsewhere. Events always pass through to
Windows. No text, passwords, key history, or network traffic is recorded or sent.
Typing in sensitive fields still produces audible key events while enabled;
disable or mute the source when appropriate. Secure desktops and other Windows
sessions are outside its capture scope.

The audio worker mixes up to 64 overlapping voices into timestamped 10 ms stereo
float blocks. OBS handles routing, monitoring, volume, and sync offsets. Multiple
independent source instances each generate their own sound; use **Add Existing**
when reusing one source across scenes to avoid duplicates.

Verified locally against installed OBS **32.2.2** on Windows x64 at both 48 kHz
and 44.1 kHz: all 14 bundled
sound sets decode and render; physical key mapping, repeat suppression, gain,
voice limits, missing packs, module loading, live preset changes, source audio
delivery, disabled silence, and teardown. The smoke test installs a real Windows
hook but triggers audio through Preview; interactive typing and subjective
listening still need a manual check in OBS. Other OBS major versions and Windows
ARM64 are not verified.

License: GPL-2.0-or-later. See `THIRD_PARTY_NOTICES.md` for sound and decoder licenses.
