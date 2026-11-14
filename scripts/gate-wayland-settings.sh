#!/usr/bin/env bash
#
# Desktop settings gate:  the light/dark preference of the desktop, and the
# chain that finds it, are checked without a display.
#
# On Wayland the answer is asked from the org.freedesktop.portal.Settings
# portal first, because that is the only source a sandboxed application can
# reach, and from gsettings when no portal runs.  Neither is linked into the
# library: both are asked with a command, so the gate can stand in for them with
# two small scripts and drive every step of the chain.
#
set -uo pipefail

ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD="$ROOT/tmp/build-fox2-wayland"
RUN="$ROOT/tmp/wayland-settings-gate"
BIN="$BUILD/bin/desktop"

[ -x "$BIN" ] || { echo "error: no binary at $BIN" >&2; exit 2; }

rm -rf -- "$RUN"
mkdir -p -- "$RUN/home" "$RUN/runtime" "$RUN/bin"
chmod 700 "$RUN" "$RUN/runtime"
RUN=$(cd -- "$RUN" && pwd)

# Stand in for the two commands the library asks; both answer only when the gate
# says so, which is what makes every step of the chain reachable
cat >"$RUN/bin/gdbus" <<'EOF'
#!/bin/sh
case "$*" in
  *org.freedesktop.portal.Settings.Read*color-scheme*)
    if [ -n "${FAKE_PORTAL:-}" ]; then echo "(<uint32 ${FAKE_PORTAL}>,)"; exit 0; fi
    ;;
esac
exit 1
EOF
cat >"$RUN/bin/gsettings" <<'EOF'
#!/bin/sh
if [ -n "${FAKE_GSETTINGS:-}" ]; then echo "${FAKE_GSETTINGS}"; exit 0; fi
exit 1
EOF
chmod +x "$RUN/bin/gdbus" "$RUN/bin/gsettings"

run_case(){
  local name="$1" config="$2" gtktheme="$3"
  mkdir -p -- "$config"
  env -u DBUS_SESSION_BUS_ADDRESS -u DBUS_SYSTEM_BUS_ADDRESS -u GTK_THEME \
      HOME="$RUN/home" XDG_CONFIG_HOME="$config" XDG_RUNTIME_DIR="$RUN/runtime" \
      PATH="$RUN/bin:/usr/bin:/bin" ${gtktheme:+GTK_THEME="$gtktheme"} \
      "$BIN" >"$RUN/$name.out" 2>&1
  }

expect(){
  local name="$1" want="$2"
  local got
  got=$(sed -n 's/^scheme: //p' "$RUN/$name.out")
  printf "%-10s %-8s %s\n" "$name" "$got" "$(sed -n 's/^setting: //p' "$RUN/$name.out")"
  if [ "$got" != "$want" ]; then
    echo "FAIL: $name reported \"$got\" instead of \"$want\""
    exit 1
    fi
  }

echo "==> an empty desktop has no preference"
run_case empty "$RUN/config-empty" ""
expect empty unknown

echo "==> GTK_THEME names the preference"
run_case envvar "$RUN/config-envvar" "Adwaita:dark"
expect envvar dark

echo "==> the GTK 4 settings file names the preference"
mkdir -p -- "$RUN/config-gtk/gtk-4.0"
printf '[Settings]\ngtk-application-prefer-dark-theme=0\n' >"$RUN/config-gtk/gtk-4.0/settings.ini"
run_case gtk "$RUN/config-gtk" ""
expect gtk light

echo "==> the KDE color scheme names the preference"
mkdir -p -- "$RUN/config-kde"
printf '[General]\nColorScheme=BreezeDark\n' >"$RUN/config-kde/kdeglobals"
run_case kde "$RUN/config-kde" ""
expect kde dark

echo "==> the portal is asked before gsettings"
FAKE_PORTAL=2 FAKE_GSETTINGS="'prefer-dark'" run_case portal "$RUN/config-portal" ""
expect portal light
grep -q "portal:" "$RUN/portal.out" || { echo "FAIL: the portal did not decide the answer"; exit 1; }

echo "==> gsettings answers when there is no portal"
FAKE_GSETTINGS="'prefer-dark'" run_case gsettings "$RUN/config-gsettings" ""
expect gsettings dark
grep -q "gsettings:" "$RUN/gsettings.out" || { echo "FAIL: gsettings did not decide the answer"; exit 1; }

echo "PASS: the desktop preference is found, and no preference is invented"
exit 0
