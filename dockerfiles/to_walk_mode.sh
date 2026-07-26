#!/bin/bash
# Prepare -> Walking (startup / manual). No GetUp.
set -uo pipefail

IFACE="${1:-127.0.0.1}"
CLIENT="${LOCO_CLIENT:-/opt/booster/sdk/build/b1_loco_example_client}"

export FASTRTPS_DEFAULT_PROFILES_FILE="${FASTRTPS_DEFAULT_PROFILES_FILE:-/app/booster_motion/fastdds_profile.xml}"
export LD_LIBRARY_PATH="/opt/booster/sdk/build:/app/booster_motion/lib:/app/booster_motion/lib-usr-local:/app/booster_motion/lib-x86_64-linux-gnu:${LD_LIBRARY_PATH:-}"

if [[ ! -x "$CLIENT" ]]; then
  echo "loco client not found: $CLIENT" >&2
  exit 1
fi

echo "[to_walk_mode] DDS addr=$IFACE  (Prepare -> Walking)"

(
  echo mp
  sleep 3
  echo mp
  sleep 2
  echo mw
  sleep 2
  echo mw
  sleep 2
  echo l
  sleep 1
  echo mw
  sleep 1
  echo l
  sleep 1
) | "$CLIENT" "$IFACE" || true

echo "[to_walk_mode] done"
