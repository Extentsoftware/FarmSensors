 FROM python:3.7-alpine

LABEL maintainer="Lapoulton" \
      description="MQTT to InfluxDB Bridge"

COPY requirements.txt /
RUN pip install --no-cache-dir --upgrade -r /requirements.txt
COPY . /app
COPY cayennelpp /app/cayennelpp
WORKDIR /app
ARG mqttbridge_version
ENV mqttbridge_version=${mqttbridge_version}
RUN echo mqttbridge_version
CMD ["python3", "-u", "main.py"]
