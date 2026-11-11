import contextlib
import typing

import cassandra.auth
import cassandra.cluster
import pytest

from . import classes, service


def pytest_addoption(parser):
    group = parser.getgroup("cassandra")
    group.addoption("--cassandra")
    group.addoption(
        "--no-cassandra",
        help="Disable use of Cassandra",
        action="store_true",
    )


def pytest_configure(config):
    config.addinivalue_line(
        "markers",
        "cassandra: per-test Cassandra initialization",
    )


def pytest_report_header(config):
    conninfo = _get_connection_info(config)
    return [f"Cassandra: {conninfo.get_uri()}"]


def pytest_service_register(register_service):
    register_service("cassandra", service.create_cassandra_service)


@pytest.fixture(scope="session")
def cassandra_disabled(pytestconfig) -> bool:
    return pytestconfig.option.no_cassandra


@pytest.fixture(scope="session")
def _cassandra_service_settings() -> service.ServiceSettings:
    return service.get_service_settings()


@pytest.fixture(scope="session")
def cassandra_connection_info(
    pytestconfig,
    _cassandra_service_settings,
) -> classes.ConnectionInfo:
    return _get_connection_info(pytestconfig)


@pytest.fixture(scope="session")
def _cassandra_service(
    ensure_service_started,
    cassandra_disabled,
    pytestconfig,
    _cassandra_service_settings,
) -> bool:
    if cassandra_disabled:
        return False
    if not pytestconfig.option.cassandra:
        ensure_service_started(
            "cassandra",
            settings=_cassandra_service_settings,
        )
    return True


@pytest.fixture(scope="session")
def _cassandra_cluster(
    _cassandra_service,
    cassandra_connection_info,
) -> typing.Iterator[cassandra.cluster.Cluster | None]:
    if not _cassandra_service:
        yield None
        return

    auth_provider = None
    if (
        cassandra_connection_info.username is not None
        and cassandra_connection_info.password is not None
    ):
        auth_provider = cassandra.auth.PlainTextAuthProvider(
            username=cassandra_connection_info.username,
            password=cassandra_connection_info.password,
        )

    instance = cassandra.cluster.Cluster(
        contact_points=list(cassandra_connection_info.hosts),
        port=cassandra_connection_info.tcp_port,
        auth_provider=auth_provider,
    )
    with contextlib.closing(instance):
        yield instance


@pytest.fixture(scope="session")
def _cassandra_control(
    _cassandra_cluster,
    cassandra_connection_info,
) -> typing.Iterator[classes.Control]:
    if _cassandra_cluster is None:
        yield classes.Control(enabled=False, session=None)
        return

    session = _cassandra_cluster.connect(cassandra_connection_info.keyspace)
    with contextlib.closing(session):
        yield classes.Control(enabled=True, session=session)


@pytest.fixture
def cassandra(_cassandra_control) -> cassandra.cluster.Session:
    return _cassandra_control.session


def _get_connection_info(config) -> classes.ConnectionInfo:
    connstr = config.option.cassandra
    if connstr:
        return service.parse_connection_url(connstr)
    settings = service.get_service_settings()
    return settings.get_connection_info()
