#!/bin/bash
# ota-upload.sh: Upload ESP32 sketch over WiFi using Arduino OTA
# Usage: ./ota-upload.sh <board> [sketch_path]
# Example: ./ota-upload.sh sleipnir
#
# This script reads WiFi credentials and hostname from secrets.h,
# finds the ESP32 on the network, and uploads the sketch via OTA.

set -euo pipefail

BOARD="${1:-}"
SKETCH_PATH="${2:-$REPO_ROOT/parking-sensor.ino}"

SECRETS_FILE="$REPO_ROOT/secrets-$BOARD.h"

# Check that the secrets file exists.
if [[ ! -f "$SECRETS_FILE" ]]; then
    echo "[ERROR] Secrets file not found: $SECRETS_FILE" >&2
    exit 1
fi

# --- Configuration ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
FQBN="${FQBN:-esp32:esp32:esp32doit-devkit-v1}"

# --- Parse secrets.h ---
get_secret() {
  local key="$1"
  grep "^#define[[:space:]]\+$key" "$SECRETS_FILE" | sed -E 's/^#define[[:space:]]+'"$key"'[[:space:]]+"([^"]*)".*/\1/'
}

WIFI_SSID="$(get_secret WIFI_SSID)"
WIFI_PASSWORD="$(get_secret WIFI_PASSWORD)"
WIFI_HOSTNAME="$(get_secret WIFI_HOSTNAME)"

if [[ -z "$WIFI_HOSTNAME" ]]; then
  echo "[ERROR] WIFI_HOSTNAME not found in $SECRETS_FILE" >&2
  exit 1
fi

# Check that avahi-resolve-host-name is available.
if ! command -v avahi-resolve-host-name >/dev/null 2>&1; then
  echo "[ERROR] avahi-resolve-host-name not found. Please install avahi-utils:" >&2
  echo "  Ubuntu/Debian: sudo apt install avahi-utils" >&2
  echo "  macOS: brew install avahi" >&2
  echo "  Arch: sudo pacman -S avahi" >&2
  exit 1
fi

# Check that arduino-cli is available.
if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "[ERROR] arduino-cli not found. Please install Arduino CLI:" >&2
  echo "  https://arduino.github.io/arduino-cli/latest/installation/" >&2
  exit 1
fi

# --- Find ESP32 on the network ---
# Try mDNS first, fallback to ping scan
PORT="3232" # Arduino OTA default port
ESP32_ADDR="$WIFI_HOSTNAME"

# Check if mDNS resolves
if ! avahi-resolve-host-name "$ESP32_ADDR" >/dev/null 2>&1; then
  echo "[WARN] mDNS lookup failed for $ESP32_ADDR. Trying to find IP via ping scan..."
  # Try to find by pinging all devices on the subnet
  SUBNET=$(ip route | awk '/default/ {print $3}' | sed 's/\.[0-9]*$/./')
  for i in {2..254}; do
    IP="${SUBNET}${i}"
    if ping -c 1 -W 1 "$IP" >/dev/null 2>&1; then
      # Try to connect to OTA port
      if nc -z -w 1 "$IP" "$PORT"; then
        echo "[INFO] Found ESP32 at $IP:$PORT"
        ESP32_ADDR="$IP"
        break
      fi
    fi
  done
fi

# --- Compile the sketch (with secrets) ---
echo "[INFO] Compiling sketch..."
arduino-cli compile \
  --fqbn "$FQBN" \
  --build-property build.extra_flags="-DWIFI_SSID=\"$WIFI_SSID\" -DWIFI_PASSWORD=\"$WIFI_PASSWORD\" -DWIFI_HOSTNAME=\"$WIFI_HOSTNAME\"" \
  "$SKETCH_PATH"

# --- Upload over OTA ---
echo "[INFO] Uploading over OTA to $ESP32_ADDR..."
arduino-cli upload \
  -p "$ESP32_ADDR" \
  --fqbn "$FQBN" \
  "$SKETCH_PATH"

echo "[SUCCESS] OTA upload complete!" 