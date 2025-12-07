#!/bin/bash
set -e  # Exit on any error

# Compile the bridge program
echo "Compiling bridge..."
cd /app/bridge
mkdir -p build
cd build
cmake ..
make

# Start supervisord to run both programs
echo "Starting programs with supervisord..."
exec /usr/bin/supervisord -n -c /etc/supervisor/conf.d/booster.conf
