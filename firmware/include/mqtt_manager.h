#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H
#include <PubSubClient.h>

extern PubSubClient client;

void setupMQTT();
void reconnectMQTT();
void publishTestMessage();
void mqttLoop();

#endif