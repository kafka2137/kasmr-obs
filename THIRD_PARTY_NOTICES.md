# Third-party notices

## kbsim recordings

The sound recordings in `data/sounds` are redistributed from
https://github.com/tplai/kbsim at commit
`ba103f3b0afa9dab80447aa2e7e2ed80b6bd80e4`.
Copyright (c) Thomas Lai. MIT license; full text is in
`data/sounds/LICENSE.kbsim`. These are the original repository recordings,
not independent measurements of every possible switch/keyboard combination.

## dr_libs

`third_party/dr_mp3.h` and `third_party/dr_wav.h` are from
https://github.com/mackron/dr_libs at commit
`8159355353a9baeea04416df9626ac2b9afbf97b`.
Copyright David Reid. Used under the MIT No Attribution alternative in
`third_party/LICENSE.dr_libs`. License notices also appear inside the headers.

## OBS Studio

The plugin links to libobs and builds against OBS Studio's official headers.
https://github.com/obsproject/obs-studio, version 32.0.4,
commit `dcdbd2e9048ae66237fdab32dbba72b745b1b9c5`.
OBS Studio is GPL-2.0-or-later. This plugin's source is also distributed under
GPL-2.0-or-later; see `LICENSE`. OBS itself is not bundled in the release ZIP.

## Tuna installer workflow

`cmake/windows/installer-Windows.iss.in` follows Tuna's Inno Setup packaging
workflow, including adapted OBS registry detection and version validation.
https://github.com/univrsal/tuna at commit
`ee695dff57c0ec6d9280f86ce14dbd7c20399456`.
Tuna is copyright its contributors, licensed under GPL-2.0.
