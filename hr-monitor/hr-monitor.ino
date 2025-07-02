#include <WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <PulseSensorPlayground.h>
#include "secrets.h"

// Configurações
#define PULSE_INPUT_PIN 34
#define THRESHOLD_WINDOW 150  // Número de amostras para ajuste automático do limiar
#define SEND_INTERVAL_MS 3000
#define SAMPLE_INTERVAL_MS 10

// Objetos globais
PulseSensorPlayground pulseSensor;
WiFiClient espClient;
WiFiUDP udp;
PubSubClient mqttClient(espClient);
IPAddress thingsboardServerIP;

unsigned long lastSendTimeMs = 0;
unsigned long lastSampleTimeMs = 0;
int bpmSum = 70;  // Valor inicial não enviesa muito
int bpmCount = 1;  // Valor inicial previne div. por 0

void setup() {
  analogReadResolution(10);
  Serial.begin(115200);
  do {
    delay(1000);
  } while (!Serial);

  // ===== Configurações do PulseSensor =====
  pulseSensor.analogInput(PULSE_INPUT_PIN);
  // pulseSensor.blinkOnPulse(2);
  if (pulseSensor.begin()) {
    Serial.println("PulseSensor iniciado com sucesso.");
  } else {
    Serial.println("Falha ao iniciar PulseSensor.");
  }

  delay(5000);  // Evita problema de conexão à rede
  connectWifi();
  receiveServerAddress();
  mqttClient.setServer(thingsboardServerIP, THINGSBOARD_PORT);

  lastSampleTimeMs = millis();
  lastSendTimeMs = millis();
}

void loop() {
  if (!mqttClient.connected()) {
    Serial.print("Cliente desconectado: ");
    Serial.println(mqttClient.state());
    reconnectMqtt();
  }
  mqttClient.loop();

  if (pulseSensor.sawStartOfBeat()) {
    int bpm = pulseSensor.getBeatsPerMinute();
    bpmSum += bpm;
    bpmCount++;
    // Serial.print("Novo BPM detectado: ");
    Serial.println(bpm);
  }

  if (millis() - lastSampleTimeMs >= SAMPLE_INTERVAL_MS) {
    // Ajuste dinâmico de limiar
    updateThreshold(analogRead(PULSE_INPUT_PIN));
    lastSampleTimeMs = millis();
  }

  if (millis() - lastSendTimeMs >= SEND_INTERVAL_MS) {
    bpmSum /= bpmCount;
    bpmCount = 1;
    sendAverageBPM(bpmSum);
    lastSendTimeMs = millis();
  }
}

void receiveServerAddress() {
  char buffer[255];
  int packetSize = 0;

  udp.begin(THINGSBOARD_DISCOVERY_PORT);
  Serial.print("Aguardando IP do servidor na porta ");
  Serial.println(THINGSBOARD_DISCOVERY_PORT);

  do {
    packetSize = udp.parsePacket();
  } while (packetSize <= 0);

  int len = udp.read(buffer, 255);
  buffer[len] = '\0';
  thingsboardServerIP = udp.remoteIP();
  Serial.printf("Received %d bytes from %s: %s\n", packetSize, thingsboardServerIP.toString().c_str(), buffer);

  // Alternativa:
  // Serial.println("Insira o IP do servidor.");
  // while (Serial.available() < 7) {}           // Tamanho mínimo de um endereço IPv4
  // Serial.readBytes(thingsboardServerIP, 16);  // Tamanho máximo de um endereço IPv4
}

void updateThreshold(int sample) {
  static int sampleCount = 0;
  static int sampleMax = 0;
  static int sampleMin = 1023;
  static int filteredSample = sample;
  static int dynamicThreshold;

  filteredSample = (filteredSample + sample) / 2;
  sampleCount++;

  // Acumula sinal para ajuste de limiar
  if (constrain(filteredSample, sampleMin, sampleMax) != filteredSample) {
    sampleMax = max(sampleMax, filteredSample);
    sampleMin = min(sampleMin, filteredSample);
    if (sampleCount >= THRESHOLD_WINDOW/2) {
      dynamicThreshold = 0.3f * sampleMin + 0.7f * sampleMax;  // Limiar em 70% da amplitude.
      pulseSensor.setThreshold(dynamicThreshold);
    }
  }
  // Serial.printf("600\t400\t%d\t%d\n", filteredSample, dynamicThreshold);

  if (sampleCount >= THRESHOLD_WINDOW) {
    // pulseSensor.setThreshold(dynamicThreshold);
    sampleCount = 0;
    sampleMax = 0;
    sampleMin = 1023;
  }
}

void sendAverageBPM(int bpmAverage) {
  String payload = String("{\"bpm\":") + bpmAverage + "}";

  if (mqttClient.publish("v1/devices/me/telemetry", payload.c_str())) {
    Serial.printf("BPM %d enviado ao ThingsBoard.\n", bpmAverage);
  } else {
    Serial.println("Falha ao publicar BPM.");
  }
}

void connectWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PSWD);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado. IP: " + WiFi.localIP().toString());
}

void reconnectMqtt() {
  while (!mqttClient.connected()) {
    Serial.println("Reconectando ao ThingsBoard MQTT...");
    if (mqttClient.connect(DEVICE_ID, THINGSBOARD_TOKEN, NULL)) {
      Serial.println("Conectado ao ThingsBoard!");
    } else {
      Serial.print("Erro MQTT: ");
      Serial.print(mqttClient.state());
      Serial.println(". Tentando em 5s...");
      delay(5000);
    }
  }
}
