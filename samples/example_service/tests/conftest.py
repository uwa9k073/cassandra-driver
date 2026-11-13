import json
import pathlib
import pytest

pytest_plugins = [
    'pytest_userver.plugins.core',
    # Подключаем ваш плагин так же, как это сделает внешний пользователь
    'pytest_userver_cql.plugins.cassandra',
]


@pytest.fixture(scope='session')
def service_secdist_path(tmp_path_factory) -> pathlib.Path:
    """Создает временный файл secdist.json для запуска сервиса."""
    secdist_data = {
        "cassandra_settings": {
            "keyspaces": {
                "store": {
                    "nodes": [
                        {
                            "use-ssl": False,
                            "use-compression": False,
                            "allow-all": True,
                            "contact-point": "localhost",
                            "port": 9042,
                        }
                    ]
                }
            }
        }
    }
    secdist_file = tmp_path_factory.mktemp('secdist') / 'secure_data.json'
    secdist_file.write_text(json.dumps(secdist_data, indent=4))
    return secdist_file


@pytest.fixture(scope='session')
def service_env(service_secdist_path):
    """Передает путь к файлу в переменную окружения SECDIST_PATH при старте сервиса."""
    return {
        'SECDIST_PATH': str(service_secdist_path),
    }
