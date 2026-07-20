# Smart Medicine Storage Monitoring System

### ESP32 · ESP-NOW · Blynk IoT · Distributed Embedded System

> A distributed IoT-based embedded system that monitors medicine storage conditions using three ESP32 development boards. The system performs local edge-based decision making, wireless communication through ESP-NOW, and remote monitoring using the Blynk IoT platform.

---

# Project Overview

This project implements a distributed embedded architecture consisting of two sensor nodes and one gateway node.

The sensor nodes monitor **temperature, humidity, and ambient light**, while the gateway node bridges ESP-NOW communication with the Blynk IoT platform over Wi-Fi. Critical decisions such as temperature-based servo actuation and relay control are executed locally on the ESP32, allowing the system to continue operating even if cloud connectivity is unavailable.

---

# Key Features

- Distributed 3-node ESP32 architecture
- ESP-NOW peer-to-peer wireless communication
- Gateway-based cloud connectivity using Blynk IoT
- Edge-based local decision making
- Automatic temperature-based servo actuation
- Manual override through Blynk and push button
- Multi-sensor monitoring (Temperature, Humidity & Light)
- Modular firmware architecture

---

# System Objectives

- Monitor medicine storage conditions in real time
- Enable wireless communication between ESP32 nodes
- Perform local sensor processing and actuator control
- Provide remote monitoring and manual control through Blynk IoT
- Develop a modular and scalable embedded system

---

# System Evolution

| Feature | Version 1 | Version 2 |
|----------|-----------|-----------|
| Communication | BLE Mesh | ESP-NOW |
| Cloud Platform | Anedya Cloud | Blynk IoT |
| Network Setup | Mesh Provisioning | Peer Registration |
| Communication Type | Mesh | Peer-to-Peer |
| Gateway | BLE Mesh Gateway | ESP32 Wi-Fi Gateway |

---

# Why ESP-NOW?

The initial prototype used BLE Mesh for communication. During development, the communication architecture was redesigned using ESP-NOW because the final system consists of three fixed ESP32 nodes.

Benefits of ESP-NOW for this project:

- Direct peer-to-peer communication
- Simplified device configuration
- Lightweight wireless communication
- Better suited for a fixed embedded network
- Easy integration with the gateway architecture

---

# System Architecture

```text
                     +------------------------------+
                     |         Blynk Cloud          |
                     | Dashboard & Manual Control   |
                     +--------------+---------------+
                                    |
                                  Wi-Fi
                                    |
                     +--------------v---------------+
                     |      ESP32 Gateway Node      |
                     |------------------------------|
                     | ESP-NOW Receiver             |
                     | Wi-Fi Manager                |
                     | Packet Decoder               |
                     | Blynk Communication          |
                     | Command Dispatcher           |
                     +------+-----------------------+
                            |
                  ESP-NOW Peer Communication
             +--------------+---------------+
             |                              |
     +-------v-------+              +-------v-------+
     |    Node 1     |              |    Node 2     |
     | Light Monitor |              | Thermal Unit  |
     +---------------+              +---------------+
```

---

# Hardware Configuration

## Node 1 – Light Monitoring Unit

| Component | GPIO |
|----------|------|
| LDR Module | GPIO27 |
| Relay Module | GPIO5 |
| LED Indicator | GPIO2 |
| Push Button | GPIO13 |

### Responsibilities

- Monitor ambient light
- Control relay and LED
- Handle manual override using GPIO interrupt
- Transmit sensor status through ESP-NOW

---

## Node 2 – Thermal Monitoring Unit

| Component | GPIO |
|----------|------|
| DHT22 | GPIO4 |
| Servo Motor | GPIO18 |
| Relay Module | GPIO5 |

### Responsibilities

- Monitor temperature and humidity
- Compare sensor readings with configured threshold
- Control servo and relay locally
- Generate PWM using ESP32 LEDC
- Transmit telemetry through ESP-NOW

---

## Gateway Node

The gateway node acts as the communication bridge between the ESP-NOW network and the Blynk IoT platform.

### Responsibilities

- Initialize Wi-Fi
- Initialize ESP-NOW
- Register peer devices
- Receive sensor data
- Decode telemetry packets
- Update the Blynk dashboard
- Forward remote commands to sensor nodes

---

# Firmware Architecture

## Sensor Node

```text
Application
│
├── Sensor Acquisition
├── Threshold Evaluation
├── Manual Override
├── Actuator Control
├── ESP-NOW Communication
└── Packet Encoding
```

---

## Gateway Node

```text
Gateway
│
├── ESP-NOW Receiver
├── Packet Decoder
├── Wi-Fi Manager
├── Blynk Communication
├── Command Dispatcher
└── Peer Management
```

---

# Firmware State Machine

```text
Manual Override
       │
       ▼
Automatic Threshold Control
       │
       ▼
Idle Monitoring
```

Manual control can be performed using:

- Physical push button
- Blynk mobile application

During manual override, automatic control is temporarily suspended before normal operation resumes.

---

# Control Logic

## Node 1

```text
Read LDR
    │
Light Detected?
 ├── YES → Relay ON + LED ON
 └── NO  → Relay OFF + LED OFF
```

---

## Node 2

```text
Read DHT22
     │
Temperature ≥ Threshold?
 ├── YES → Servo Rotate + Relay ON
 └── NO  → Servo Reset + Relay OFF
```

---

# Communication Flow

```text
Sensors
   │
Local Processing
   │
Threshold Evaluation
   │
ESP-NOW
   │
Gateway Node
   │
Wi-Fi
   │
Blynk Cloud
```

Remote commands follow the reverse path from the Blynk application to the target sensor node through the gateway.

---

# Compact Telemetry Encoding

Temperature and humidity values are encoded into integer payloads before wireless transmission while preserving one decimal place of precision.

```cpp
int16_t tempEncoded = temperature * 10;
int16_t humEncoded  = 1000 + humidity * 10;
```

The gateway decodes the received values before updating the Blynk dashboard.

---

# Embedded Features

- Distributed embedded architecture
- ESP-NOW peer-to-peer communication
- Gateway-based cloud integration
- Edge-based local decision making
- Event-driven firmware
- Non-blocking scheduling using `millis()`
- GPIO interrupt handling
- Hardware PWM using ESP32 LEDC
- Relay and servo control
- Manual and automatic operating modes
- Compact telemetry encoding
- Modular firmware design

---

# Engineering Challenges

| Challenge | Solution |
|------------|----------|
| BLE Mesh provisioning complexity | Migrated to ESP-NOW for a fixed three-node architecture |
| ESP-NOW and Wi-Fi coexistence | Connected Wi-Fi before ESP-NOW initialization and aligned communication channels |
| Manual override conflicting with automatic control | Implemented state-based override logic |
| Servo instability | Improved power distribution and common grounding |
| Periodic sensor updates | Used `millis()`-based scheduling to keep the main loop responsive |

---

# Technologies Used

## Hardware

- ESP32 Development Boards (×3)
- DHT22 Sensor
- LDR Module (LM393)
- Servo Motor
- Relay Modules
- Push Button
- LED Indicator

## Software

- Embedded C++
- Arduino IDE
- ESP-NOW
- Wi-Fi
- Blynk IoT
- ESP32 LEDC PWM

---

# Future Improvements

- OTA firmware updates
- SD card data logging
- MQTT integration
- Battery backup support
- Additional environmental sensors

---

# Skills Demonstrated

- Embedded Systems
- Embedded C++
- ESP32 Firmware Development
- ESP-NOW
- Wireless Embedded Communication
- Sensor Interfacing
- Hardware Integration
- Edge Computing
- GPIO Interrupt Handling
- PWM Generation
- Gateway Architecture
- IoT System Design
- Distributed Embedded Systems
- Firmware Debugging
- System Integration

---

# Repository Structure

```text
Smart-Medicine-Storage-Monitor
│
├── Gateway/
├── Node1_Light_Monitor/
├── Node2_Thermal_Control/
├── Images/
└── README.md
```

---

# License

This project is intended for educational purposes and embedded systems learning.
