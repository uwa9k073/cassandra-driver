import pytest
from pytest_plugins.pytest_infra.databases.cassandra import classes, service
from testsuite.environment import service as env_service


def test_parse_connection_url_simple():
    conn = service.parse_connection_url(
        "cassandra://localhost:9042/test_keyspace",
    )
    assert conn.hosts == ("localhost",)
    assert conn.tcp_port == 9042
    assert conn.keyspace == "test_keyspace"


def test_parse_connection_url_multi_host():
    conn = service.parse_connection_url("host-a:9042,host-b:9042")
    assert conn.hosts == ("host-a", "host-b")
    assert conn.tcp_port == 9042


def test_parse_connection_url_multi_host_with_keyspace():
    conn = service.parse_connection_url(
        "cassandra://a:9042,b:9042/my_keyspace",
    )
    assert conn.hosts == ("a", "b")
    assert conn.tcp_port == 9042
    assert conn.keyspace == "my_keyspace"


def test_parse_connection_url_strips_whitespace():
    conn = service.parse_connection_url("  cassandra://localhost:9042/ks  ")
    assert conn.keyspace == "ks"


def test_parse_connection_url_default_port_single_host():
    conn = service.parse_connection_url("cassandra://localhost")
    assert conn.hosts == ("localhost",)
    assert conn.tcp_port == service.DEFAULT_CASSANDRA_PORT


def test_parse_connection_url_default_port_multi_host_no_port():
    conn = service.parse_connection_url("cassandra://a,b")
    assert conn.hosts == ("a", "b")
    assert conn.tcp_port == service.DEFAULT_CASSANDRA_PORT


def test_parse_connection_url_with_auth():
    conn = service.parse_connection_url(
        "cassandra://alice:secret@localhost:9042/myks",
    )
    assert conn.hosts == ("localhost",)
    assert conn.tcp_port == 9042
    assert conn.keyspace == "myks"
    assert conn.username == "alice"
    assert conn.password == "secret"


def test_parse_connection_url_rejects_unsupported_scheme():
    with pytest.raises(ValueError, match="Unsupported cassandra URL scheme"):
        service.parse_connection_url("http://localhost:9042")


def test_parse_connection_url_empty_raises():
    with pytest.raises(ValueError):
        service.parse_connection_url("cassandra://")


def test_parse_connection_url_mismatched_ports_raises():
    with pytest.raises(ValueError, match="same port"):
        service.parse_connection_url("h1:9042,h2:9043")


def test_parse_get_uri_roundtrip_single_host():
    original = classes.ConnectionInfo(
        hosts=("localhost",),
        tcp_port=9042,
        keyspace="roundtrip_ks",
    )
    parsed = service.parse_connection_url(original.get_uri())
    assert parsed.hosts == original.hosts
    assert parsed.tcp_port == original.tcp_port
    assert parsed.keyspace == original.keyspace
    assert parsed.username is None


def test_parse_get_uri_roundtrip_multi_host():
    original = classes.ConnectionInfo(
        hosts=("x", "y"),
        tcp_port=9042,
    )
    parsed = service.parse_connection_url(original.get_uri())
    assert parsed.hosts == original.hosts
    assert parsed.tcp_port == original.tcp_port


def test_parse_get_uri_roundtrip_with_auth():
    original = classes.ConnectionInfo(
        hosts=("localhost",),
        tcp_port=9042,
        keyspace="ks",
        username="u",
        password="p",
    )
    parsed = service.parse_connection_url(original.get_uri())
    assert parsed.hosts == original.hosts
    assert parsed.tcp_port == original.tcp_port
    assert parsed.keyspace == original.keyspace
    assert parsed.username == original.username
    assert parsed.password == original.password


def test_get_service_settings_defaults(monkeypatch):
    monkeypatch.delenv("TESTSUITE_CASSANDRA_SERVER_HOST", raising=False)
    monkeypatch.delenv("TESTSUITE_CASSANDRA_SERVER_PORT", raising=False)
    monkeypatch.delenv("TESTSUITE_CASSANDRA_STORAGE_PORT", raising=False)
    settings = service.get_service_settings()
    assert settings.host == service.DEFAULT_CASSANDRA_HOST
    assert settings.tcp_port == service.DEFAULT_CASSANDRA_PORT
    assert settings.storage_port == service.DEFAULT_CASSANDRA_STORAGE_PORT
    conn = settings.get_connection_info()
    assert conn.hosts == (service.DEFAULT_CASSANDRA_HOST,)
    assert conn.tcp_port == service.DEFAULT_CASSANDRA_PORT


def test_get_service_settings_from_env(monkeypatch):
    monkeypatch.setenv("TESTSUITE_CASSANDRA_SERVER_HOST", "10.0.0.1")
    monkeypatch.setenv("TESTSUITE_CASSANDRA_SERVER_PORT", "12345")
    monkeypatch.setenv("TESTSUITE_CASSANDRA_STORAGE_PORT", "9999")
    settings = service.get_service_settings()
    assert settings.host == "10.0.0.1"
    assert settings.tcp_port == 12345
    assert settings.storage_port == 9999


def test_create_cassandra_service(tmp_path):
    ss = service.create_cassandra_service(
        "cassandra",
        str(tmp_path),
        service.ServiceSettings(
            host="127.0.0.1",
            tcp_port=19042,
            storage_port=17000,
        ),
    )
    assert isinstance(ss, env_service.ScriptService)
    assert ss._check_ports == [19042]
