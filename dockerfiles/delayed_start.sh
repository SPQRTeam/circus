#!/bin/bash
# Wait for booster-motion to settle, then start bridge + TF publisher.
# (Matches working origin/rviz2tf flow; motion is autostarted by supervisord.)
set +u
set -o pipefail

CONF=/etc/supervisor/conf.d/booster.conf
CLIENT=/opt/booster/sdk/build/b1_loco_example_client

echo "[delayed_start] begin — wait for booster-motion"
for i in $(seq 1 30); do
  st=$(supervisorctl -c "$CONF" status booster-motion 2>/dev/null | awk '{print $2}')
  echo "[delayed_start] booster-motion=$st ($i)"
  if [ "$st" = "RUNNING" ]; then
    break
  fi
  if [ "$st" = "FATAL" ] || [ "$st" = "BACKOFF" ]; then
    echo "[delayed_start] motion failed — clear IPC and kick"
    ipcrm -a 2>/dev/null || true
    supervisorctl -c "$CONF" stop booster-motion 2>/dev/null || true
    sleep 1
    supervisorctl -c "$CONF" start booster-motion || true
  fi
  sleep 2
done

sleep 3
echo "[delayed_start] starting simbridge + robot_state_publisher"
supervisorctl -c "$CONF" start simbridge || echo "[delayed_start] simbridge start failed"
supervisorctl -c "$CONF" start robot_state_publisher || true

send_loco() {
  (
    for cmd in "$@"; do
      echo "$cmd"
      sleep 1
    done
    sleep 1
  ) | "$CLIENT" 127.0.0.1 || true
}

(
  sleep 5
  echo "[delayed_start] Prepare -> Walking"
  send_loco mp mp mw mw l mw
  while true; do
    sleep 4
    send_loco mw l
  done
) &

echo "[delayed_start] done (stand loop in background)"
