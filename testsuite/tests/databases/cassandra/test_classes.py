import pytest
from pytest_plugins.pytest_infra.databases.cassandra import classes


def test_connection_info_get_uri_single_host():
    conn = classes.ConnectionInfo(
        hosts=("localhost",),
        tcp_port=9042,
        keyspace="ks",
    )
    assert conn.get_uri() == "cassandra://localhost:9042/ks"


def test_connection_info_get_uri_multi_host():
    conn = classes.ConnectionInfo(
        hosts=("a", "b"),
        tcp_port=9042,
    )
    assert conn.get_uri() == "cassandra://a:9042,b:9042"


def test_connection_info_get_uri_with_credentials():
    conn = classes.ConnectionInfo(
        hosts=("localhost",),
        tcp_port=9042,
        keyspace="ks",
        username="u",
        password="p",
    )
    assert conn.get_uri() == "cassandra://u:p@localhost:9042/ks"


def test_connection_info_get_uri_username_only():
    conn = classes.ConnectionInfo(
        hosts=("localhost",),
        tcp_port=9042,
        username="u",
        password=None,
    )
    assert conn.get_uri() == "cassandra://u@localhost:9042"


def test_control_session_raises_when_disabled():
    ctrl = classes.Control(enabled=False, session=None)
    with pytest.raises(classes.CassandraDisabledError):
        _ = ctrl.session


def test_control_session_returns_when_enabled():
    class _FakeSession:
        pass

    fake = _FakeSession()
    ctrl = classes.Control(enabled=True, session=fake)
    assert ctrl.session is fake
