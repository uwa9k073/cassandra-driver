import dataclasses

from cassandra import cluster


class BaseError(Exception):
    pass


class CassandraDisabledError(BaseError):
    pass


@dataclasses.dataclass(frozen=True)
class ConnectionInfo:
    """Cassandra connection parameters."""

    hosts: tuple[str, ...]
    tcp_port: int
    keyspace: str | None = None
    username: str | None = None
    password: str | None = None

    def get_uri(self) -> str:
        hosts = ",".join(f"{h}:{self.tcp_port}" for h in self.hosts)
        if self.username is not None and self.password is not None:
            netloc = f"{self.username}:{self.password}@{hosts}"
        elif self.username is not None:
            netloc = f"{self.username}@{hosts}"
        else:
            netloc = hosts
        if self.keyspace:
            return f"cassandra://{netloc}/{self.keyspace}"
        return f"cassandra://{netloc}"


class Control:
    def __init__(self, enabled: bool, session: cluster.Session | None):
        self._enabled = enabled
        self._session = session

    @property
    def session(self) -> cluster.Session:
        if not self._enabled or self._session is None:
            raise CassandraDisabledError
        return self._session
