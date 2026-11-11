import pathlib
import typing
import urllib.parse

from testsuite.environment import service, utils

from . import classes

DEFAULT_CASSANDRA_HOST = "localhost"
DEFAULT_CASSANDRA_PORT = 19042
DEFAULT_CASSANDRA_STORAGE_PORT = 17000

SERVICE_SCRIPT_PATH = pathlib.Path(__file__).parent.joinpath(
    "scripts/service-cassandra",
)


class ServiceSettings(typing.NamedTuple):
    host: str
    tcp_port: int
    storage_port: int

    def get_connection_info(self) -> classes.ConnectionInfo:
        return classes.ConnectionInfo(
            hosts=(self.host,),
            tcp_port=self.tcp_port,
        )


def create_cassandra_service(
    service_name,
    working_dir,
    settings: ServiceSettings | None = None,
    env: dict[str, str] | None = None,
):
    if settings is None:
        settings = get_service_settings()
    return service.ScriptService(
        service_name=service_name,
        script_path=str(SERVICE_SCRIPT_PATH),
        working_dir=working_dir,
        environment={
            "CASSANDRA_TMPDIR": working_dir,
            "CASSANDRA_HOST": settings.host,
            "CASSANDRA_TCP_PORT": str(settings.tcp_port),
            "CASSANDRA_STORAGE_PORT": str(settings.storage_port),
            **(env or {}),
        },
        check_ports=[settings.tcp_port],
        start_timeout=utils.getenv_float(
            key="TESTSUITE_CASSANDRA_SERVER_START_TIMEOUT",
            default=40.0,
        ),
    )


def parse_connection_url(url: str) -> classes.ConnectionInfo:
    url = url.strip()
    if "://" not in url:
        url = f"cassandra://{url}"
    parsed = urllib.parse.urlparse(url)
    if parsed.scheme and parsed.scheme != "cassandra":
        raise ValueError(
            f"Unsupported cassandra URL scheme {parsed.scheme!r}",
        )

    netloc = parsed.netloc or parsed.path
    if not netloc:
        raise ValueError("Empty cassandra connection URL")

    if "@" in netloc:
        userinfo, hosts_part = netloc.rsplit("@", 1)
        username, _, password = userinfo.partition(":")
    else:
        hosts_part = netloc
        username = None
        password = None

    hosts = tuple(host.strip() for host in hosts_part.split(",") if host.strip())
    if not hosts:
        raise ValueError("No cassandra hosts in connection URL")

    parsed_hosts = []
    port = None
    for host in hosts:
        if ":" in host:
            hostname, host_port = host.rsplit(":", 1)
            parsed_hosts.append(hostname)
            current_port = int(host_port)
            if port is None:
                port = current_port
            elif port != current_port:
                raise ValueError(
                    "All cassandra hosts must use the same port in URL",
                )
        else:
            parsed_hosts.append(host)

    keyspace = parsed.path.lstrip("/") or None
    return classes.ConnectionInfo(
        hosts=tuple(parsed_hosts),
        tcp_port=port or DEFAULT_CASSANDRA_PORT,
        keyspace=keyspace,
        username=username or None,
        password=password or None,
    )


def get_service_settings() -> ServiceSettings:
    return ServiceSettings(
        host=utils.getenv_str(
            "TESTSUITE_CASSANDRA_SERVER_HOST",
            DEFAULT_CASSANDRA_HOST,
        ),
        tcp_port=utils.getenv_int(
            "TESTSUITE_CASSANDRA_SERVER_PORT",
            DEFAULT_CASSANDRA_PORT,
        ),
        storage_port=utils.getenv_int(
            "TESTSUITE_CASSANDRA_STORAGE_PORT",
            DEFAULT_CASSANDRA_STORAGE_PORT,
        ),
    )
