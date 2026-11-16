import json
import pathlib
import pytest

pytest_plugins = [
    'pytest_userver.plugins.core',
    # Подключаем ваш плагин так же, как это сделает внешний пользователь
    'pytest_userver_cql.plugins.cass',
]


USERVER_CONFIG_HOOKS = ["config_patch"]


@pytest.fixture(scope="session", name="root_dir")
def _root_dir():
    """Path to root directory service."""
    return pathlib.Path(__file__).parent.parent



@pytest.fixture(scope="session")
def config_patch(root_dir):

    def patch_config(config, _):
        components = config["components_manager"]["components"]
        components["default-secdist-provider"]["config"] = str(
            root_dir / "configs/secure_data.json"
        )

    return patch_config


import pathlib
import pytest

@pytest.fixture(scope="session")
def cassandra_schema_paths(root_dir) -> list[pathlib.Path]:
    return [
        root_dir / "cassandra/schemas/keyspace.cql",
        root_dir / "cassandra/schemas/table.cql",
    ]
