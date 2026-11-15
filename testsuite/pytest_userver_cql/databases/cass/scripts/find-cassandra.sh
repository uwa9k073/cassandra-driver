#!/bin/bash

find_cassandra() {
    CASSANDRA_BIN=
    NODETOOL_BIN=

    for dir in "$TESTSUITE_CASSANDRA_BINDIR" "$CASSANDRA_HOME/bin"; do
        if [ -n "$dir" ] && [ -x "$dir/cassandra" ]; then
            CASSANDRA_BIN="$dir/cassandra"
            if [ -x "$dir/nodetool" ]; then
                NODETOOL_BIN="$dir/nodetool"
            fi
            break
        fi
    done

    if [ -z "$CASSANDRA_BIN" ]; then
        CASSANDRA_BIN=$(command -v cassandra 2>/dev/null || true)
    fi

    if [ -z "$NODETOOL_BIN" ]; then
        # Пытаемся найти nodetool рядом с cassandra
        if [ -n "$CASSANDRA_BIN" ]; then
            candidate="$(dirname "$CASSANDRA_BIN")/nodetool"
            if [ -x "$candidate" ]; then
                NODETOOL_BIN="$candidate"
            fi
        fi
        # Или в PATH
        if [ -z "$NODETOOL_BIN" ]; then
            NODETOOL_BIN=$(command -v nodetool 2>/dev/null || true)
        fi
    fi

    if [ -z "$CASSANDRA_BIN" ]; then
        die "No cassandra executable found.
Please install Cassandra and ensure cassandra is available in PATH,
or set TESTSUITE_CASSANDRA_BINDIR (or CASSANDRA_HOME) to Cassandra binaries directory."
    fi

    if [ -z "$NODETOOL_BIN" ] || [ ! -x "$NODETOOL_BIN" ]; then
        die "nodetool not found (required for readiness check).
Please ensure nodetool is available next to cassandra or in PATH."
    fi

    echo "Cassandra binary: $CASSANDRA_BIN"
    echo "Nodetool binary:  $NODETOOL_BIN"
}

find_cassandra || die "Cassandra is not found."