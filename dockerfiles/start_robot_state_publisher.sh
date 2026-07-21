#!/bin/bash
# Publish /tf from T1 URDF + Circus joint states.
set -eo pipefail

# Avoid nounset issues in ROS setup scripts.
set +u
source /opt/ros/humble/setup.bash
set -u

export FASTRTPS_DEFAULT_PROFILES_FILE=/app/booster_motion/fastdds_profile.xml
export AMENT_PREFIX_PATH=/app/bridge:${AMENT_PREFIX_PATH:-}

URDF_PATH=/app/booster_motion/configs/t1.urdf
if [ ! -f "$URDF_PATH" ]; then
  echo "[robot_state_publisher] URDF not found: $URDF_PATH" >&2
  exit 1
fi

# Wait until joint states are available (simbridge up).
for _ in $(seq 1 30); do
  if ros2 topic list 2>/dev/null | grep -qx /booster/ros2_k2_joint_states; then
    break
  fi
  sleep 1
done

PARAMS=/tmp/robot_state_publisher_params.yaml
python3 - <<PY
from pathlib import Path
urdf = Path("$URDF_PATH").read_text()
# YAML literal block keeps multiline URDF intact for ros2 params.
Path("$PARAMS").write_text(
    "robot_state_publisher:\n"
    "  ros__parameters:\n"
    "    robot_description: |\n"
    + "".join("      " + line + "\n" for line in urdf.splitlines())
)
print("[robot_state_publisher] wrote", "$PARAMS", "urdf_bytes=", len(urdf))
PY

exec ros2 run robot_state_publisher robot_state_publisher --ros-args \
  --params-file "$PARAMS" \
  -r /joint_states:=/booster/ros2_k2_joint_states
