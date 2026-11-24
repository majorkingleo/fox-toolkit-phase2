# Breeze GTK theme sources

`light/` and `dark/` contain the installed GTK theme payloads for Breeze GTK
6.8.3. They are retained as source/reference data and can be refreshed from a
Linux installation with `./import-breeze.sh [themes-directory] [destination]`.
The script looks in the XDG user theme directory, `~/.themes`, and then
`/usr/share/themes` when no source is supplied. It copies `Breeze` and
`Breeze-Dark` into the `light/` and `dark/` subdirectories.

FOX's compiled-in presets are generated from the same Breeze GTK theme data,
but the running application does not read these GTK files or require a GTK
installation. The FOX preset contains the semantic colors and metrics used by
FOX widgets; it does not embed GTK's widget assets.

## Licensing and attribution

These files are generated from KDE's Breeze GTK theme, version 6.8.3. The theme
stylesheets and GTK 2 theme are distributed under the BSD 3-Clause license;
rendered artwork in `assets/` is CC0 1.0. The corresponding license texts are
provided in `LICENSES/`. Breeze GTK source and project information:
<https://invent.kde.org/plasma/breeze-gtk>.
