#!/bin/bash

# Export dynamic libraries and fastdds profile for booster_motion execution
export LD_LIBRARY_PATH=/app/booster_motion/lib:/app/booster_motion/lib-usr-local:/app/booster_motion/lib-x86_64-linux-gnu:$LD_LIBRARY_PATH
export FASTRTPS_DEFAULT_PROFILES_FILE=/app/booster_motion/fastdds_profile.xml

# Export environment variables for MAXIMUS framework
export SPQR_CONFIG_ROOT=/app/maximus/config
export SPQR_BEHAVIOR_TREE_PATH=/app/maximus/behaviors

if [ -f /app/maximus/tools/vision/scripts/detect-tensorrt.sh ]; then
  # shellcheck disable=SC1091
  source /app/maximus/tools/vision/scripts/detect-tensorrt.sh
  if [ -n "${TENSORRT_DIR:-}" ]; then
    TENSORRT_LINKNAME=$(echo "$TENSORRT_DIR" | sed 's:[^/]*$:TensorRT:')
    ln -sfn "$TENSORRT_DIR" "$TENSORRT_LINKNAME" || true
  fi
else
  echo "[entrypoint] warn: detect-tensorrt.sh missing, continuing without TensorRT setup"
fi

# Start supervisord to manage processes
/usr/bin/supervisord -n -c /etc/supervisor/conf.d/booster.conf
