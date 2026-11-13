#!/bin/bash
set -euo pipefail
repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="${QVIEW_LOOP_BUILD_DIR:-$repo_dir/build}"
default_destination="$HOME/Applications/qView Loop.app"
destination="${1:-$default_destination}"
desktop_link="${QVIEW_LOOP_DESKTOP_LINK:-}"
if [[ -z "$desktop_link" && "$destination" == "$default_destination" ]]; then
    desktop_link="$HOME/Desktop/qView Loop.app"
fi
if [[ -e "$destination" || -L "$destination" ]]; then
    echo "Destination exists; choose a new output path: $destination" >&2
    exit 1
fi
if [[ -n "$desktop_link" && ( -e "$desktop_link" || -L "$desktop_link" ) ]]; then
    if [[ ! -L "$desktop_link" || "$(readlink "$desktop_link")" != "$destination" ]]; then
        echo "Desktop link path already exists; move it aside before building: $desktop_link" >&2
        exit 1
    fi
fi
qt_prefix="$(brew --prefix)"
cmake -S "$repo_dir" -B "$build_dir" -DCMAKE_PREFIX_PATH="$qt_prefix" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTS=ON -DQV_DISABLE_ONLINE_VERSION_CHECK=ON
cmake --build "$build_dir" --parallel 6
ctest --test-dir "$build_dir" --output-on-failure
stage_dir="$(mktemp -d /private/tmp/qview-loop-macos.XXXXXX)"
trap 'rm -rf "$stage_dir"' EXIT
staged_app="$stage_dir/qView.app"
ditto "$build_dir/qView.app" "$staged_app"
"$qt_prefix/bin/macdeployqt" "$staged_app" -always-overwrite -no-codesign
# Stage outside Desktop's File Provider domain: it restores FinderInfo on bundles there.
# Remove only packaging metadata that codesign rejects; preserve provenance and provider attributes.
xattr -dr com.apple.FinderInfo "$staged_app" 2>/dev/null || true
xattr -dr com.apple.ResourceFork "$staged_app" 2>/dev/null || true
codesign --force --deep --sign - "$staged_app"
codesign --verify --deep --strict "$staged_app"
mkdir -p "$(dirname "$destination")"
ditto "$staged_app" "$destination"
codesign --verify --deep --strict "$destination"
if [[ -n "$desktop_link" && ! -L "$desktop_link" ]]; then
    ln -s "$destination" "$desktop_link"
fi
echo "Built $destination"
