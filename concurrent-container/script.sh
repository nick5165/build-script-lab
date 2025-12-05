#!/bin/sh

SHARED_DIR="/data"
LOCK_FILE="$SHARED_DIR/.lock"
CONTAINER_ID="$(hostname)-$(cat /proc/sys/kernel/random/uuid | cut -c1-8)"
COUNTER=1

cleanup(){
    echo "Container stopped. Cleaning up..."
    if [ -f "/tmp/current_filename" ]; then
        CURRENT_FILE=$(cat /tmp/current_filename)
        if [ -f "$SHARED_DIR/$CURRENT_FILE" ]; then
            rm "$SHARED_DIR/$CURRENT_FILE"
            echo "Deleted orphan file $CURRENT_FILE"
        fi
    fi
    exit 0
}
trap cleanup SIGTERM SIGINT EXIT

touch "$LOCK_FILE"

echo "Container $CONTAINER_ID started"

while true; do
    MY_FILE=""
    (
        flock -x 200
        i=1
        while true; do
        NAME=$(printf "%03d" "$i")
        FILE_PATH="$SHARED_DIR/$NAME"

        if [ ! -e "$FILE_PATH" ]; then
            echo "$CONTAINER_ID $COUNTER" > "$FILE_PATH"
            echo "$NAME" > /tmp/current_filename
            break
        fi 
        i=$((i + 1))
    done
    ) 200>"$LOCK_FILE"

    MY_FILE=$(cat /tmp/current_filename)

    echo "[Create] $CONTAINER_ID created $MY_FILE (Seq: $COUNTER)"

    sleep 1

    if [ -f "$SHARED_DIR/$MY_FILE" ]; then
        rm "$SHARED_DIR/$MY_FILE"
        echo "[Delete] $CONTAINER_ID deleted $MY_FILE"
    fi

    COUNTER=$((COUNTER + 1))

done
