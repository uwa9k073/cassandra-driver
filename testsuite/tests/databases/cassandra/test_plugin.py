from types import SimpleNamespace

from pytest_plugins.pytest_infra.databases.cassandra import classes, plugin, service


def test_get_connection_info_uses_cli_url():
    config = SimpleNamespace(
        option=SimpleNamespace(
            cassandra="cassandra://host.example:19042/namespace",
        ),
    )
    info = plugin._get_connection_info(config)
    assert info.hosts == ("host.example",)
    assert info.tcp_port == 19042
    assert info.keyspace == "namespace"


def test_get_connection_info_empty_cli_falls_back_to_settings(monkeypatch):
    config = SimpleNamespace(option=SimpleNamespace(cassandra=None))

    def fake_settings():
        return service.ServiceSettings(
            host="from-env",
            tcp_port=5555,
            storage_port=1111,
        )

    monkeypatch.setattr(service, "get_service_settings", fake_settings)
    info = plugin._get_connection_info(config)
    assert info.hosts == ("from-env",)
    assert info.tcp_port == 5555
    assert info.keyspace is None


def test_get_connection_info_empty_string_cli_falls_back_to_settings(monkeypatch):
    config = SimpleNamespace(option=SimpleNamespace(cassandra=""))

    monkeypatch.setattr(
        service,
        "get_service_settings",
        lambda: service.ServiceSettings(
            host="h",
            tcp_port=1,
            storage_port=2,
        ),
    )
    info = plugin._get_connection_info(config)
    assert info == classes.ConnectionInfo(hosts=("h",), tcp_port=1)


def test_pytest_report_header_includes_uri(monkeypatch):
    expected = classes.ConnectionInfo(
        hosts=("127.0.0.1",),
        tcp_port=19042,
        keyspace="ks",
    )
    monkeypatch.setattr(plugin, "_get_connection_info", lambda _c: expected)

    config = SimpleNamespace()

    lines = plugin.pytest_report_header(config)
    assert len(lines) == 1
    assert lines[0].startswith("Cassandra: ")
    assert expected.get_uri() in lines[0]
