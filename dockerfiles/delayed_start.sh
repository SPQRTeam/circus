#!/bin/bash
# Wait for 5 seconds
sleep 6

# Start simbridge and maximus in parallel
# This substitute the timer of 5 seconds inside the bridge.
# In this way the bridge (or maximus) does not need to have the
# check on the timer if the code is run on the robot or on simulator 
supervisorctl -c /etc/supervisor/conf.d/booster.conf start simbridge maximus