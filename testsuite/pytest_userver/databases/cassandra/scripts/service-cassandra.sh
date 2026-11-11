#!/bin/sh

. "$TESTSUITE_LIB_UTILS"
. "$(dirname "$0")/find-cassandra.sh"

# use ramdisk for cassandra if available
RAMDISK="/mnt/ramdisk/$USER"
if mkdir -p "$RAMDISK" 2> /dev/null; then
    CASSANDRA_TMPDIR=$RAMDISK/_cassandra${WORKER_SUFFIX_PATH}
fi

if [ "x$CASSANDRA_TMPDIR" = "x" ]; then
    die "CASSANDRA_TMPDIR must be set or RAM-disk must be enabled"
fi
if [ "x$CASSANDRA_HOST" = "x" ]; then
    die "CASSANDRA_HOST must be set"
fi
if [ "x$CASSANDRA_TCP_PORT" = "x" ]; then
    die "CASSANDRA_TCP_PORT must be set"
fi
if [ "x$CASSANDRA_STORAGE_PORT" = "x" ]; then
    die "CASSANDRA_STORAGE_PORT must be set"
fi

CASSANDRA_PID_FILE="$CASSANDRA_TMPDIR/cassandra.pid"
CASSANDRA_LOG_FILE="$CASSANDRA_TMPDIR/cassandra.log"
CASSANDRA_DATA_DIR="$CASSANDRA_TMPDIR/data"
CASSANDRA_COMMITLOG_DIR="$CASSANDRA_TMPDIR/commitlog"
CASSANDRA_SAVED_CACHES_DIR="$CASSANDRA_TMPDIR/saved_caches"
CASSANDRA_HINTS_DIR="$CASSANDRA_TMPDIR/hints"

start_cassandra() {
    mkdir -p "$CASSANDRA_DATA_DIR"
    mkdir -p "$CASSANDRA_COMMITLOG_DIR"
    mkdir -p "$CASSANDRA_SAVED_CACHES_DIR"
    mkdir -p "$CASSANDRA_HINTS_DIR"

    "$CASSANDRA_BIN" \
        -R \
        -p "$CASSANDRA_PID_FILE" \
        > "$CASSANDRA_LOG_FILE" 2>&1
}

start() {
    # The daemon picks up these values from JVM system properties.
    export JVM_EXTRA_OPTS="-Dcassandra.listen_address=$CASSANDRA_HOST \
        -Dcassandra.broadcast_address=$CASSANDRA_HOST \
        -Dcassandra.native_transport_address=$CASSANDRA_HOST \
        -Dcassandra.rpc_address=$CASSANDRA_HOST \
        -Dcassandra.native_transport_port=$CASSANDRA_TCP_PORT \
        -Dcassandra.storage_port=$CASSANDRA_STORAGE_PORT \
        -Dcassandra.saved_caches_directory=$CASSANDRA_SAVED_CACHES_DIR \
        -Dcassandra.commitlog_directory=$CASSANDRA_COMMITLOG_DIR \
        -Dcassandra.hints_directory=$CASSANDRA_HINTS_DIR \
        -Dcassandra.data_file_directories=$CASSANDRA_DATA_DIR"
    (ulimit_files && start_cassandra)
}

stop() {
    stop_daemon "$CASSANDRA_BIN" "$CASSANDRA_PID_FILE"
    rm -rf "$CASSANDRA_TMPDIR"
}

script_main "$@"
