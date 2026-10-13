#!/usr/bin/env bash
set -euo pipefail
if [ "$#" -ne 2 ]; then
  echo 'Usage: tools/package.sh /local/packing.keys /local/hacbrewpack' >&2
  exit 1
fi
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
key_path="$(realpath "$1")"
pack_tool="$(realpath "$2")"
package_dir="$(mktemp -d)"
trap 'rm -rf "$package_dir"' EXIT
mkdir -p "$package_dir/exefs" "$package_dir/control" "$project_dir/dist"
elf2nso "$project_dir/DouyinNativeSwitch.elf" "$package_dir/exefs/main"
npdmtool "$project_dir/npdm.json" "$package_dir/exefs/main.npdm"
cp "$project_dir/DouyinNativeSwitch.nacp" "$package_dir/control/control.nacp"
cp "$project_dir/icon.jpg" "$package_dir/control/icon_AmericanEnglish.dat"
"$pack_tool" -k "$key_path" --exefsdir "$package_dir/exefs" --controldir "$package_dir/control" --romfsdir "$project_dir/romfs" --nologo --tempdir "$package_dir/temp" --ncadir "$package_dir/nca" --nspdir "$package_dir/nsp"
cp "$package_dir/nsp/0100d0a11a020000.nsp" "$project_dir/dist/DouyinNativeSwitch-0.3.0-test.nsp"
