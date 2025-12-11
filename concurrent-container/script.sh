#!/bin/sh

SHARED_DIR="/data"
LOCK_FILE="$SHARED_DIR/.lock"
CONTAINER_ID="$(hostname)-$(cat /proc/sys/kernel/random/uuid | cut -c1-8)"
COUNTER=1
TEMP_STATE_FILE="/tmp/current_filename"

cleanup(){
    if [ -f "$TEMP_STATE_FILE" ]; then
        CURRENT_FILE=$(cat "$TEMP_STATE_FILE")
        if [ -n "$CURRENT_FILE" ] && [ -f "$SHARED_DIR/$CURRENT_FILE" ]; then
            rm "$SHARED_DIR/$CURRENT_FILE"
        fi
        rm -f "$TEMP_STATE_FILE"
    fi
    exit 0
}
trap cleanup SIGTERM SIGINT EXIT

touch "$LOCK_FILE"

echo "Container $CONTAINER_ID started"

while true; do
    rm -f "$TEMP_STATE_FILE"

    (
        flock -x 200
        i=1
        while true; do
            NAME=$(printf "%03d" "$i")
            FILE_PATH="$SHARED_DIR/$NAME"

            if [ ! -e "$FILE_PATH" ]; then
                echo "$CONTAINER_ID $COUNTER" > "$FILE_PATH"
                echo "$NAME" > "$TEMP_STATE_FILE"
                break
            fi 
            i=$((i + 1))
        done
    ) 200>"$LOCK_FILE"

    if [ -f "$TEMP_STATE_FILE" ]; then
        MY_FILE=$(cat "$TEMP_STATE_FILE")
        
        echo "[Create] $CONTAINER_ID created $MY_FILE (Seq: $COUNTER)"

        sleep 1

        if [ -f "$SHARED_DIR/$MY_FILE" ]; then
            rm "$SHARED_DIR/$MY_FILE"
            echo "[Delete] $CONTAINER_ID deleted $MY_FILE"
        fi

        rm -f "$TEMP_STATE_FILE"
        COUNTER=$((COUNTER + 1))
    fi

done