import pathlib
import pytest


USERVER_CONFIG_HOOKS = ["config_patch"]

pytest_plugins = [
    "pytest_userver.plugins.core",
]


@pytest.fixture(scope="session")
def root_dir():
    """Path to root directory service."""
    return pathlib.Path(__file__).parent.parent


@pytest.fixture(scope="session")
def config_patch(root_dir):
    """Change path to secdist"""

    def patch_config(config, _):
        components = config["components_manager"]["components"]
        components["default-secdist-provider"]["config"] = str(
            root_dir / "configs/secure_data.json"
        )

    return patch_config
