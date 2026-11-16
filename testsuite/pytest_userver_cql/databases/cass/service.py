import pathlib
import typing
import urllib.parse

from testsuite.environment import service, utils

from . import classes

DEFAULT_CASSANDRA_HOST = "localhost"
DEFAULT_CASSANDRA_PORT = 19042
DEFAULT_CASSANDRA_STORAGE_PORT = 17000

SERVICE_SCRIPT_PATH = pathlib.Path(__file__).parent.joinpath(
    "scripts/service-cassandra.sh",
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
            "LD_PRELOAD": "",
            "CASSANDRA_TMPDIR": working_dir,
            "CASSANDRA_HOST": settings.host,
            "CASSANDRA_TCP_PORT": str(settings.tcp_port),
            "CASSANDRA_STORAGE_PORT": str(settings.storage_port),
            **(env or {}),
        },
        check_ports=[settings.tcp_port],
        start_timeout=utils.getenv_float(
            key="TESTSUITE_CASSANDRA_SERVER_START_TIMEOUT",
            default=240.0,
        ),
    )



def _extract_nodes_from_secdist(
    secdist_data: dict[str, typing.Any],
) -> tuple[list[str], int | None]:
    cassandra_settings = secdist_data.get("cassandra_settings") or {}
    keyspaces = cassandra_settings.get("keyspaces") or {}

    hosts: list[str] = []
    port: int | None = None

    for ks_config in keyspaces.values():
        for node in ks_config.get("nodes") or []:
            contact_point = node.get("contact-point")
            if contact_point and contact_point not in hosts:
                hosts.append(contact_point)
            node_port = node.get("port")
            if node_port is not None:
                node_port = int(node_port)
                if port is None:
                    port = node_port
                elif port != node_port:
                    raise ValueError(
                        f"Conflicting Cassandra ports in secdist: {port} vs {node_port}",
                    )
    return hosts, port


def get_service_settings_from_secdist(
    secdist_data: dict[str, typing.Any],
) -> ServiceSettings:
    defaults = get_service_settings()
    hosts, port = _extract_nodes_from_secdist(secdist_data)
    return ServiceSettings(
        host=hosts[0] if hosts else defaults.host,
        tcp_port=port if port is not None else defaults.tcp_port,
        storage_port=defaults.storage_port,
    )



def get_connection_info_from_secdist(
    secdist_data: dict[str, typing.Any],
    fallback: ServiceSettings,
) -> classes.ConnectionInfo:
    hosts, port = _extract_nodes_from_secdist(secdist_data)
    return classes.ConnectionInfo(
        hosts=tuple(hosts) if hosts else (fallback.host,),
        tcp_port=port if port is not None else fallback.tcp_port,
    )


def parse_connection_url(url: str) -> classes.ConnectionInfo:
    url = url.strip()
    if "://" not in url:
        url = f"cassandra://{url}"
    parsed = urllib.parse.urlsplit(url)
    if parsed.scheme and parsed.scheme != "cassandra":
        raise ValueError(
            f"Unsupported cassandra URL scheme {parsed.scheme!r}",
        )

    netloc = parsed.netloc
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
        if host.startswith("["):
            hostname, sep, rest = host[1:].partition("]")
            if not sep:
                raise ValueError(f"Invalid IPv6 host {host!r}")
            parsed_hosts.append(hostname)
            if rest.startswith(":"):
                current_port = int(rest[1:])
                if port is None:
                    port = current_port
                elif port != current_port:
                    raise ValueError(
                        "All cassandra hosts must use the same port in URL",
                    )
        elif ":" in host:
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
