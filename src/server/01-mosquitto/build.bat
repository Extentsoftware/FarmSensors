set mosquitto_version=1.0.27
docker build . -t gluteusmaximus/mosquitto:%mosquitto_version% -t gluteusmaximus/mosquitto:latest --build-arg mosquitto_version=%mosquitto_version%
docker push  gluteusmaximus/mosquitto
docker push  gluteusmaximus/mosquitto:%mosquitto_version%
pause