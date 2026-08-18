#!/usr/bin/env bash
set -euo pipefail

# Clean working tree for source upload (no build artifacts).

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"

rm -rf -- \
	build \
	qsi_setup/deb_packages/*.deb \
	qsi_setup/output/*.qsi \
	qsi_setup/qinstaller \
	*.qsi

exit 0
