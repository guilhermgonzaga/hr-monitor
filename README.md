# hr-monitor

Distributed Systems coursework. Arduino sketch to send heart rate to local Thingsboard server via ESP32. The device periodically reads the user's heart rate in beats per minute (BPM) and publishes this data to a ThingsBoard server over Wi-Fi using MQTT.

The implementation

- utilizes the PulseSensorPlayground library for real-time heart rate acquisition;
- averages BPM readings leading to every transmission;
- adapts dynamically to signal strength by calibrating the pulse detection threshold automatically.

## Usage

### Hardware Requirements

- ESP32 development board;
- PPG sensor (signal compatible with [PulseSensor.com](https://pulsesensor.com/));
- Wi-Fi connectivity to the same local network as the ThingsBoard server.

### Software Dependencies

Install the following in the Arduino IDE:

- ESP32-based board support (boards manager)
- PulseSensor Playground (library manager)
- PubSubClient (library manager)
- WiFi.h (included with ESP32 board support)

### Local Network Discovery

In the original setup, the ThingsBoard server operated on a DHCP-enabled local network without DNS. Given that an IP address was required to reach the MQTT broker, a workaround was implemented using a companion PowerShell script. This script runs on the server and broadcasts its IP address via UDP when needed.

The ESP32 listens for this broadcast and updates the MQTT connection target accordingly. This avoids the need for static IP configuration or external DNS services.

### Communication Details

When powered, the ESP32 will:

- Connect to the Wi-Fi network;
- Listen for a ThingsBoard IP broadcast;
- Establish an MQTT connection to the received IP;
- Publish BPM values to the _telemetry_ topic;
