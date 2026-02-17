# https://stackoverflow.com/questions/56998486/stop-docker-containers-with-name-matching-a-pattern
# ...and some help from Gemini for the ifs

RUNNING_IDS=$(docker ps --filter name="CIRCUS_*" --filter status=running -aq)
if [ -n "$RUNNING_IDS" ]; then
    echo "Stopping circus containers"
    docker stop $RUNNING_IDS
else
    echo "No containers to stop"
fi

EXITED_IDS=$(docker ps --filter name="CIRCUS_*" -aq)
if [ -n "$EXITED_IDS" ]; then
    echo "Removing circus containers"
    docker rm $EXITED_IDS
else
    echo "No containers to remove"
fi

NETWORK_IDS=$(docker network list --filter name="CIRCUS_*" -q)
if [ -n "$NETWORK_IDS" ]; then
    echo "Removing circus networks"
    docker network rm $NETWORK_IDS
else
    echo "No networks to remove"
fi
