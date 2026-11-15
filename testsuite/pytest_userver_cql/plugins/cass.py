import contextlib
import json
import pathlib
import typing


import cassandra
import pytest

from ..databases.cass import classes, service


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
    connstr = config.option.cassandra
    if connstr:
        conninfo = service.parse_connection_url(connstr)
        return [f"Cassandra: {conninfo.get_uri()}"]
    return ["Cassandra: configured via secdist"]


def pytest_service_register(register_service):
    register_service("cassandra", service.create_cassandra_service)


@pytest.fixture(scope="session")
def cassandra_disabled(pytestconfig) -> bool:
    return pytestconfig.option.no_cassandra


@pytest.fixture(scope="session")
def cassandra_secdist_data(request) -> dict[str, typing.Any]:
    """Reads secdist dictionary from service fixtures if present."""
    if "service_env" in request.fixturenames:
        env = request.getfixturevalue("service_env") or {}
        env_path = env.get("SECDIST_PATH")
        if env_path:
            secdist_path = pathlib.Path(env_path)
            if secdist_path.is_file():
                return json.loads(secdist_path.read_text())

    if "service_secdist_path" in request.fixturenames:
        raw_path = request.getfixturevalue("service_secdist_path")
        if raw_path:
            secdist_path = pathlib.Path(raw_path)
            if secdist_path.is_file():
                return json.loads(secdist_path.read_text())

    for fixture_name in ("service_secdist", "secdist_data"):
        if fixture_name in request.fixturenames:
            data = request.getfixturevalue(fixture_name)
            if isinstance(data, dict) and "cassandra_settings" in data:
                return data

    return {}


@pytest.fixture(scope="session")
def cassandra_service_settings(
    cassandra_secdist_data,
) -> service.ServiceSettings:
    return service.get_service_settings_from_secdist(cassandra_secdist_data)


@pytest.fixture(scope="session")
def cassandra_connection_info(
    pytestconfig,
    cassandra_secdist_data,
    cassandra_service_settings,
) -> classes.ConnectionInfo:
    connstr = pytestconfig.option.cassandra
    if connstr:
        return service.parse_connection_url(connstr)
    return service.get_connection_info_from_secdist(
        cassandra_secdist_data,
        fallback=cassandra_service_settings,
    )


@pytest.fixture(scope="session")
def _cassandra_service(
    ensure_service_started,
    cassandra_disabled,
    pytestconfig,
    cassandra_service_settings,
) -> bool:
    if cassandra_disabled:
        return False
    if not pytestconfig.option.cassandra:
        ensure_service_started(
            "cassandra",
            settings=cassandra_service_settings,
        )
    return True


@pytest.fixture
def auto_client_deps(_cassandra_service, _cassandra_schema):
    """Ensures Cassandra service is started before userver service_client."""


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
        protocol_version=4
    )
    try:
            yield instance
    finally:
            instance.shutdown()



@pytest.fixture(scope="session")
def _cassandra_control(
    _cassandra_cluster,
    cassandra_connection_info,
) -> typing.Iterator[classes.Control]:
    if _cassandra_cluster is None:
        yield classes.Control(enabled=False, session=None)
        return

    session = _cassandra_cluster.connect()
    try:
        yield classes.Control(enabled=True, session=session)
    finally:
        session.shutdown()


@pytest.fixture
def _cassandra(_cassandra_control: classes.Control ) -> cassandra.cluster.Session:
    return _cassandra_control.session



# ============================================================
# Пользователь переопределяет эту фикстуру
# ============================================================
@pytest.fixture(scope="session")
def cassandra_schema_paths() -> list[str | pathlib.Path]:
    """
    Возвращает список путей к .cql файлам со схемой.
    Аналог того, как в postgres передают sql-файлы.
    """
    return []


# ============================================================
# Внутренняя: читает файлы
# ============================================================
@pytest.fixture(scope="session")
def _cassandra_schema_statements(
    cassandra_schema_paths: list[str | pathlib.Path],
) -> list[str]:
    statements: list[str] = []

    for raw_path in cassandra_schema_paths:
        path = pathlib.Path(raw_path)
        if not path.is_file():
            raise FileNotFoundError(f"Cassandra schema file not found: {path}")

        content = path.read_text(encoding="utf-8")

        # Простой split по `;` (как обычно делают с sql-файлами в testsuite)
        for stmt in content.split(";"):
            stmt = stmt.strip()
            # Пропускаем пустые и чисто комментариевые куски
            if not stmt or all(
                line.strip().startswith("--") or not line.strip()
                for line in stmt.splitlines()
            ):
                continue
            statements.append(stmt)

    return statements


# ============================================================
# Основная фикстура, которую используют в тестах
# ============================================================
@pytest.fixture(scope="session")
def _cassandra_schema(
    _cassandra_control: classes.Control,          # session-scoped
    _cassandra_schema_statements: list[str],
) -> typing.Iterator[None]:
    """
    Применяет схему из файлов, указанных в cassandra_schema_paths.
    """
    if not _cassandra_control._enabled:
        yield
        return

    session = _cassandra_control.session

    for statement in _cassandra_schema_statements:
        session.execute(statement)

    yield
