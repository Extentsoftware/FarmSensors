set mqttbridge_version=1.0.28
docker build . -t gluteusmaximus/mqttbridge:%mqttbridge_version% -t gluteusmaximus/mqttbridge:latest --build-arg mqttbridge_version=%mqttbridge_version%
docker push  gluteusmaximus/mqttbridge
docker push  gluteusmaximus/mqttbridge:%mqttbridge_version%
pause