#!/bin/bash

# Export dyanmic libraries and fastdds profile for booster_motion execution
export LD_LIBRARY_PATH=/app/booster_motion/lib:/app/booster_motion/lib-usr-local:/app/booster_motion/lib-x86_64-linux-gnu:$LD_LIBRARY_PATH
export FASTRTPS_DEFAULT_PROFILES_FILE=/app/booster_motion/fastdds_profile.xml


# Start supervisord to manage processes
/usr/bin/supervisord -n -c /etc/supervisor/conf.d/booster.conf
