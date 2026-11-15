#!/bin/bash
set -e

die() {
    echo "$*" >&2
    exit 1
}

. "$(dirname "$0")/find-cassandra.sh"

: "${CASSANDRA_JMX_PORT:=7199}"
export CASSANDRA_JMX_PORT

# Use ramdisk if it is actually mounted, preserving worker isolation
RAMDISK_BASE="/mnt/ramdisk"
if mountpoint -q "$RAMDISK_BASE" 2>/dev/null && mkdir -p "$RAMDISK_BASE/$USER" 2>/dev/null; then
    WORKER_DIR=$(basename "${CASSANDRA_TMPDIR:-_cassandra${WORKER_SUFFIX_PATH}}")
    CASSANDRA_TMPDIR="$RAMDISK_BASE/$USER/$WORKER_DIR"
fi

[ -n "$CASSANDRA_BIN" ] || die "CASSANDRA_BIN must be set"
[ -n "$CASSANDRA_TMPDIR" ] || die "CASSANDRA_TMPDIR must be set or RAM-disk must be enabled"
[ -n "$CASSANDRA_HOST" ] || die "CASSANDRA_HOST must be set"
[ -n "$CASSANDRA_TCP_PORT" ] || die "CASSANDRA_TCP_PORT must be set"
[ -n "$CASSANDRA_STORAGE_PORT" ] || die "CASSANDRA_STORAGE_PORT must be set"

CASSANDRA_BIND_HOST="$CASSANDRA_HOST"
if [ "$CASSANDRA_BIND_HOST" = "localhost" ]; then
    CASSANDRA_BIND_HOST="127.0.0.1"
fi


CASSANDRA_PID_FILE="$CASSANDRA_TMPDIR/cassandra.pid"
CASSANDRA_LOG_FILE="$CASSANDRA_TMPDIR/cassandra.log"
CASSANDRA_LOG_DIR="$CASSANDRA_TMPDIR/logs"
CASSANDRA_CONF_DIR="$CASSANDRA_TMPDIR/conf"
CASSANDRA_CONFIG_FILE="$CASSANDRA_CONF_DIR/cassandra.yaml"
CASSANDRA_INCLUDE_FILE="$CASSANDRA_TMPDIR/cassandra.in.sh"
CASSANDRA_DATA_DIR="$CASSANDRA_TMPDIR/data"
CASSANDRA_COMMITLOG_DIR="$CASSANDRA_TMPDIR/commitlog"
CASSANDRA_SAVED_CACHES_DIR="$CASSANDRA_TMPDIR/saved_caches"
CASSANDRA_HINTS_DIR="$CASSANDRA_TMPDIR/hints"

write_cassandra_config() {
    mkdir -p "$CASSANDRA_CONF_DIR"

    SYS_CONF_DIR="/etc/cassandra"
    if [ ! -d "$SYS_CONF_DIR" ]; then
        SYS_CONF_DIR="$(dirname "$(dirname "$CASSANDRA_BIN")")/conf"
    fi
    if [ -d "$SYS_CONF_DIR" ]; then
        cp -a "$SYS_CONF_DIR"/. "$CASSANDRA_CONF_DIR"/ 2>/dev/null || true
        sed -i "s|/var/log/cassandra|$CASSANDRA_LOG_DIR|g" \
            "$CASSANDRA_CONF_DIR"/jvm*.options \
            "$CASSANDRA_CONF_DIR"/logback*.xml 2>/dev/null || true
    fi

    cat > "$CASSANDRA_CONFIG_FILE" <<YAML
cluster_name: 'TestsuiteCassandraCluster'
num_tokens: 16
authenticator: AllowAllAuthenticator
authorizer: AllowAllAuthorizer
partitioner: org.apache.cassandra.dht.Murmur3Partitioner
data_file_directories:
  - $CASSANDRA_DATA_DIR
commitlog_directory: $CASSANDRA_COMMITLOG_DIR
saved_caches_directory: $CASSANDRA_SAVED_CACHES_DIR
hints_directory: $CASSANDRA_HINTS_DIR
seed_provider:
  - class_name: org.apache.cassandra.locator.SimpleSeedProvider
    parameters:
      - seeds: "$CASSANDRA_BIND_HOST:$CASSANDRA_STORAGE_PORT"
listen_address: $CASSANDRA_BIND_HOST
broadcast_address: $CASSANDRA_BIND_HOST
rpc_address: 0.0.0.0
broadcast_rpc_address: $CASSANDRA_BIND_HOST
native_transport_port: $CASSANDRA_TCP_PORT
storage_port: $CASSANDRA_STORAGE_PORT
endpoint_snitch: SimpleSnitch
commitlog_sync: periodic
commitlog_sync_period: 10s
disk_failure_policy: stop
commit_failure_policy: stop
YAML

    cat > "$CASSANDRA_INCLUDE_FILE" <<SH
if [ -r /usr/share/cassandra/cassandra.in.sh ]; then
    . /usr/share/cassandra/cassandra.in.sh
fi
CASSANDRA_CONF="$CASSANDRA_CONF_DIR"
CASSANDRA_LOG_DIR="$CASSANDRA_LOG_DIR"
JMX_PORT="$CASSANDRA_JMX_PORT"
export CASSANDRA_CONF CASSANDRA_LOG_DIR JMX_PORT
SH
}

is_cassandra_ready() {
    timeout 0.5 bash -c "exec 3<>/dev/tcp/$CASSANDRA_BIND_HOST/$CASSANDRA_TCP_PORT" 2>/dev/null
}
wait_for_cassandra() {
    pid="$1"
    retries=160 # up to 80 seconds
    i=0
    while [ "$i" -lt "$retries" ]; do
        if ! kill -0 "$pid" 2>/dev/null; then
            echo "Cassandra process (PID $pid) exited prematurely. Log output:" >&2
            cat "$CASSANDRA_LOG_FILE" >&2 || true
            return 1
        fi
        if is_cassandra_ready; then
            return 0
        fi
        sleep 0.5
        i=$((i + 1))
    done

    echo "Timed out waiting for Cassandra on $CASSANDRA_BIND_HOST:$CASSANDRA_TCP_PORT. Log output:" >&2
    cat "$CASSANDRA_LOG_FILE" >&2 || true
    cat "$CASSANDRA_LOG_DIR/system.log" >&2 || true
    kill "$pid" 2>/dev/null || true
    return 1
}

start_cassandra() {
    mkdir -p \
        "$CASSANDRA_DATA_DIR" \
        "$CASSANDRA_COMMITLOG_DIR" \
        "$CASSANDRA_SAVED_CACHES_DIR" \
        "$CASSANDRA_HINTS_DIR" \
        "$CASSANDRA_LOG_DIR"

    write_cassandra_config

    "$CASSANDRA_BIN" -f -R > "$CASSANDRA_LOG_FILE" 2>&1 &
    pid=$!
    echo "$pid" > "$CASSANDRA_PID_FILE"

    wait_for_cassandra "$pid"
}

start() {
    ulimit -n 65536 2>/dev/null || true

    export CASSANDRA_INCLUDE="$CASSANDRA_INCLUDE_FILE"
    export CASSANDRA_CONF="$CASSANDRA_CONF_DIR"
    export CASSANDRA_LOG_DIR="$CASSANDRA_LOG_DIR"

    JVM_OPTS="-Dcassandra.config=file://$CASSANDRA_CONFIG_FILE"
    JVM_OPTS="$JVM_OPTS -Dcassandra.logdir=$CASSANDRA_LOG_DIR"
    JVM_OPTS="$JVM_OPTS -Djava.rmi.server.hostname=$CASSANDRA_BIND_HOST"
    JVM_OPTS="$JVM_OPTS -Dcassandra.skip_wait_for_gossip_to_settle=0"
    JVM_OPTS="$JVM_OPTS -Dcassandra.ring_delay_ms=100"
    JVM_OPTS="$JVM_OPTS -Xms2048M -Xmx4096M"
    # Явно включаем JMX без аутентификации (только для тестов)
    JVM_OPTS="$JVM_OPTS -Dcom.sun.management.jmxremote"
    JVM_OPTS="$JVM_OPTS -Dcom.sun.management.jmxremote.port=$CASSANDRA_JMX_PORT"
    JVM_OPTS="$JVM_OPTS -Dcom.sun.management.jmxremote.rmi.port=$CASSANDRA_JMX_PORT"
    JVM_OPTS="$JVM_OPTS -Dcom.sun.management.jmxremote.authenticate=false"
    JVM_OPTS="$JVM_OPTS -Dcom.sun.management.jmxremote.ssl=false"
    export JVM_OPTS

    start_cassandra
}

stop() {
    if [ -f "$CASSANDRA_PID_FILE" ]; then
        pid=$(cat "$CASSANDRA_PID_FILE" 2>/dev/null || true)
        if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
            pkill -TERM -P "$pid" 2>/dev/null || true
            kill -TERM "$pid" 2>/dev/null || true

            i=0
            while [ "$i" -lt 40 ] && kill -0 "$pid" 2>/dev/null; do
                sleep 0.25
                i=$((i + 1))
            done

            if kill -0 "$pid" 2>/dev/null; then
                pkill -KILL -P "$pid" 2>/dev/null || true
                kill -KILL "$pid" 2>/dev/null || true
            fi
        fi
    fi

    if [ -n "$CASSANDRA_TMPDIR" ] && [ "$CASSANDRA_TMPDIR" != "/" ]; then
        rm -rf "$CASSANDRA_TMPDIR"
    fi
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    *)
        die "Usage: $0 {start|stop}"
        ;;
esac
