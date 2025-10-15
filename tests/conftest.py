import pathlib

import pytest

pytest_plugins = [
    "pytest_userver.plugins.core",
]

USERVER_CONFIG_HOOKS = ["secdist_path"]


@pytest.fixture(scope="session")
def root_dir():
    """Path to root directory service."""
    return pathlib.Path(__file__).parent.parent


@pytest.fixture(scope="session")
def secdist_path(root_dir):
    """Change path to secdist"""

    def patch_config(config, config_vars):
        components = config["components_manager"]["components"]
        components["default-secdist-provider"]["config"] = str(
            root_dir / "configs/secure_data.json"
        )

    return patch_config
