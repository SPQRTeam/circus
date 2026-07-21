#!/bin/bash
# Wait for booster-motion to settle, then start bridge + TF publisher.
sleep 3

# Start simbridge (maximus disabled here: needs TensorRT libs not present on this host tree).
# SimBridge alone is enough for Circus to accept the robot connection and run the sim loop.
supervisorctl -c /etc/supervisor/conf.d/booster.conf start simbridge

# Publish /tf from t1.urdf + /booster/ros2_k2_joint_states
supervisorctl -c /etc/supervisor/conf.d/booster.conf start robot_state_publisher
