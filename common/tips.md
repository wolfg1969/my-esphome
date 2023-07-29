```
2023-07-29 21:31:35.347 WARNING (MainThread) [homeassistant.components.mqtt.discovery] Received message on illegal discovery topic 'homeassistant/sensor/weatherstation-zhuwo//config'. The topic contains not allowed characters. For more information see https://www.home-assistant.io/integrations/mqtt/#discovery-topic
```

```
$ mosquitto_pub -h 127.0.0.1 -t "homeassistant/sensor/weatherstation-zhuwo//config" -n -d -r
Client mosqpub|23859-homepi sending CONNECT
Client mosqpub|23859-homepi received CONNACK (0)
Client mosqpub|23859-homepi sending PUBLISH (d0, q0, r1, m1, 'homeassistant/sensor/weatherstation-zhuwo//config', ... (0 bytes))
Client mosqpub|23859-homepi sending DISCONNECT
```

```
mqtt:
  discovery_object_id_generator: "device_name"
```


https://community.home-assistant.io/t/home-assistant-cant-read-actual-status-of-tasmota-sonoff-switch/146756/4

```yaml
switch:
  - name: ensuite_fan
    platform: mqtt
    state_topic: stat/EnsuiteFan/POWER
    command_topic: cmnd/EnsuiteFan/POWER
    availability_topic: tele/EnsuiteFan/LWT
    qos: 1
    payload_on: 'ON'
    payload_off: 'OFF'
    payload_available: 'Online'
    payload_not_available: 'Offline'
    retain: false
```
