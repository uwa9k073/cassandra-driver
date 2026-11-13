async def test_basic(service_client):
    # Здесь уже будут работать фикстуры и хуки из вашего плагина cassandra.py
    response = await service_client.get('/ping')
    assert response.status == 200


async def test_create_cassandra(service_client):
    response = await service_client.post('/cassandra', json = {"id": 1, "name": "john"})
    
    assert response.status == 200
    assert response.json()["id"] == 1
    assert response.json()["name"] == "john"
