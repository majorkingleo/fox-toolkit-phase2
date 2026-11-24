#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source_root="${1:-}"
destination="${2:-$script_dir}"

if [[ -z "$source_root" ]]; then
  for candidate in "${XDG_DATA_HOME:-$HOME/.local/share}/themes" "$HOME/.themes" /usr/share/themes; do
    if [[ -f "$candidate/Breeze/gtk-3.0/gtk.css" && -f "$candidate/Breeze-Dark/gtk-3.0/gtk.css" ]]; then
      source_root="$candidate"
      break
    fi
  done
fi

if [[ -z "$source_root" ]]; then
  echo "Breeze and Breeze-Dark were not found. Pass the Linux themes directory as the first argument." >&2
  exit 1
fi

copy_theme() {
  local source="$1"
  local target="$2"
  local name="$3"

  if [[ ! -f "$source/gtk-3.0/gtk.css" ]]; then
    echo "Missing GTK 3 theme stylesheet: $source/gtk-3.0/gtk.css" >&2
    exit 1
  fi

  mkdir -p "$target"
  for part in assets gtk-2.0 gtk-3.0 gtk-4.0; do
    if [[ -d "$source/$part" ]]; then
      rm -rf -- "$target/$part"
      cp -a -- "$source/$part" "$target/"
    fi
  done
  echo "Imported $name from $source"
}

copy_theme "$source_root/Breeze" "$destination/light" Breeze
copy_theme "$source_root/Breeze-Dark" "$destination/dark" Breeze-Dark
