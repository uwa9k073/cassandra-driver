#!/bin/sh

find_cassandra() {
    CASSANDRA_BIN=
    for dir in "$TESTSUITE_CASSANDRA_BINDIR" "$CASSANDRA_HOME/bin"; do
        if [ "x$dir" != "x" ] && [ -x "$dir/cassandra" ]; then
            CASSANDRA_BIN="$dir/cassandra"
            break
        fi
    done
    if [ "x$CASSANDRA_BIN" = "x" ]; then
        CASSANDRA_BIN=$(which cassandra)
    fi
    if [ "x$CASSANDRA_BIN" = "x" ]; then
        die "No cassandra executable found.
Please install Cassandra and ensure cassandra is available in PATH,
or set TESTSUITE_CASSANDRA_BINDIR (or CASSANDRA_HOME) to Cassandra binaries directory."
    fi
    echo "Cassandra binary: $CASSANDRA_BIN"
}

find_cassandra || die "Cassandra is not found."
