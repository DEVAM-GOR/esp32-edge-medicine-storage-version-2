# Smart Medicine Storage Monitor
### ESP32 · ESP-NOW · Blynk IoT · Edge-First Architecture

> A distributed 3-node IoT system for real-time pharmaceutical storage monitoring and autonomous actuation — built with edge-first control logic, sub-100ms response latency, and cloud visibility via Blynk.

---

## Why I Built This

Medicines are failing patients silently.

Not because of bad doctors or wrong prescriptions — but because of a humidity spike nobody caught, a light exposure that lasted too long, or a temperature breach between two manual checks. The WHO estimates a large fraction of medicines lose efficacy due to exactly these unmonitored storage failures.

I wanted to build a system that actually responds — not just monitors.

---

## Evolution: v1 → v2

This is **v2** of this project.

| | v1 (Previous) | v2 (This Repo) |
|---|---|---|
| **Protocol** | BLE Mesh + nRF Mesh App | ESP-NOW (peer-to-peer, MAC layer) |
| **Cloud** | Anedya Cloud | Blynk IoT |
| **Router required** | Yes | No |
| **Peer setup** | nRF mesh provisioning | MAC address registration |
| **Coexistence** | Separate radio | ESP-NOW + WiFi on single 2.4GHz radio |

**Why I switched:**
BLE Mesh added provisioning complexity and latency that wasn't acceptable for a safety-critical actuation system. ESP-NOW operates at the MAC layer — no TCP/IP, no router, no handshake overhead. For a system where a temperature breach needs a servo response in under 100ms, the protocol choice matters.

---

## System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    LAYER 1: EDGE NODES                  │
│                                                         │
│  ┌──────────────────┐      ┌──────────────────────────┐ │
│  │    NODE 1        │      │        NODE 2            │ │
│  │  Light Monitor   │      │    Thermal Control       │ │
│  │                  │      │                          │ │
│  │  LDR (LM393)     │      │  DHT22 (Temp + Humidity) │ │
│  │  Relay Module    │      │  Servo Motor (LEDC PWM)  │ │
│  │  LED Indicator   │      │  Relay Module            │ │
│  │  Push Button ISR │      │                          │ │
│  └────────┬─────────┘      └────────────┬─────────────┘ │
│           │   ESP-NOW (no router)        │               │
└───────────┼─────────────────────────────┼───────────────┘
            │                             │
            ▼                             ▼
┌─────────────────────────────────────────────────────────┐
│                 LAYER 2: MASTER GATEWAY                 │
│                                                         │
│              ESP32 (Dual-radio hub)                     │
│         ESP-NOW ←──────────────→ WiFi                   │
│    Registers peers via MAC address                      │
│    Forwards telemetry to Blynk (V0–V4)                  │
│    Relays remote commands back to nodes                 │
└──────────────────────────┬──────────────────────────────┘
                           │ WiFi
                           ▼
┌─────────────────────────────────────────────────────────┐
│                  LAYER 3: BLYNK CLOUD                   │
│                                                         │
│   V0 → Temperature gauge                               │
│   V1 → Humidity gauge                                   │
│   V2 → LDR light status                                 │
│   V3 → Relay command (Node 1)                           │
│   V4 → Servo + Relay command (Node 2)                   │
└─────────────────────────────────────────────────────────┘
```

**Core design rule: Cloud enhances, never blocks.**
If WiFi drops, edge nodes keep sensing and actuating autonomously.

---

## Hardware — Node by Node

### Node 1 — Light Monitoring Unit

| Component | Pin | Role |
|---|---|---|
| LDR Module (LM393) | GPIO27 (DO) | Digital light detection |
| Relay Module | GPIO5 | External alarm / light control |
| LED Indicator | GPIO2 | Visual alert |
| Push Button | GPIO13 (ISR) | Manual override (hardware interrupt) |

**How it works:**
- LDR resistance changes with light → LM393 comparator converts to digital HIGH/LOW
- Light detected → relay + LED trigger instantly at the edge
- Button press fires a hardware ISR → activates 10-second manual override

---

### Node 2 — Thermal Control Unit

| Component | Pin | Role |
|---|---|---|
| DHT22 | GPIO4 | Temperature + humidity sensing |
| Servo Motor | GPIO18 (LEDC PWM) | Cooling simulation (0°–90° sweep) |
| Relay Module | GPIO (control) | External fan / cooling device |

**How it works:**
- DHT22 reads continuously (non-blocking, millis() based)
- Temp ≥ 28°C → servo sweeps via LEDC PWM at ~50Hz + relay activates
- Relay decouples 3.3V ESP32 logic from high-power cooling loads

---

### Master Node — Gateway

- No sensors
- Registers both nodes via MAC addresses using `esp_now_add_peer()`
- Runs ESP-NOW and WiFi simultaneously on one 2.4GHz radio
- Streams data to Blynk virtual pins (V0–V4)
- Receives Blynk write events and relays commands to target nodes via ESP-NOW

---

## Firmware Design

### Priority Arbitration (no RTOS needed)

```
Priority 1 (Highest): Manual Override
    → Physical button ISR OR Blynk app command
    → Active for 10 seconds
    → All sensor logic paused during window

Priority 2: Automatic Sensor Logic
    → Runs only when no override is active
    → Node 1: light → relay + LED
    → Node 2: temp ≥ 28°C → servo + relay

Priority 3 (Lowest): Idle
    → All actuators off
    → Minimal power draw
```

**Implementation:** `boolean overrideActive` flag + `millis()` timestamp. Zero blocking code. Zero `delay()`.

---

### Data Encoding — Two Values, One Packet

Sending temperature AND humidity in a single ESP-NOW `int16_t` frame:

```cpp
// Encoding (Node 2)
int16_t tempEncoded  = (int16_t)(temperature * 10);       // e.g. 29.5°C → 295
int16_t humEncoded   = (int16_t)(1000 + humidity * 10);   // e.g. 50.5% → 1505

// Decoding (Master)
if (value >= 1000) {
    humidity    = (value - 1000) / 10.0;   // ≥1000 → humidity
} else {
    temperature = value / 10.0;            // <1000 → temperature
}
```

One packet per reading cycle. No extra overhead.

---

### ESP-NOW + WiFi Coexistence Fix

Both protocols share the 2.4GHz radio. Without channel alignment, ESP-NOW packets vanish silently.

**Fix:**
1. Connect WiFi first (locks the channel)
2. Bind ESP-NOW peers to the same channel
3. Enable `CONFIG_SW_COEXIST_ENABLE` for automatic time-sharing

---

### Servo Jitter Fix

Relay switching caused power rail spikes → servo jitter.

**Fix:**
- Separate power domains for relay and servo
- Add decoupling capacitors across servo power pins
- Common GND discipline across all components

---

## Control & Data Flow

```
// Uplink (sense → cloud)
Node → [ESP-NOW] → Master → [WiFi] → Blynk Dashboard

// Downlink (user → actuator)
Blynk App → [WiFi] → Master → [ESP-NOW] → Node → Actuator

// Edge (no internet needed)
Sensor Threshold Breached → Node Logic → Relay/Servo (< 40ms)
```

---

## Measured Performance

| Metric | Value |
|---|---|
| Local actuation latency | ~20–40ms |
| ESP-NOW packet delivery | Sub-millisecond (MAC layer) |
| Override window | 10 seconds |
| Sensor polling | Non-blocking (millis-based) |
| Servo PWM frequency | ~50Hz (LEDC) |

---

## Tech Stack

```
Hardware:   ESP32 (×3), DHT22, LDR/LM393, Servo Motor, Relay Modules
Protocol:   ESP-NOW (peer-to-peer), WiFi
Cloud:      Blynk IoT (virtual pins V0–V4)
Firmware:   C/C++ (Arduino framework / ESP-IDF concepts)
PWM:        LEDC (ESP32 hardware PWM)
```

---

## What I'd Build Next

- [ ] Replace servo with real cooling element (Peltier / fan)
- [ ] Closed-loop temperature control (PID)
- [ ] Telemetry buffering + timestamping for breach history
- [ ] OTA firmware updates via Blynk or HTTPS
- [ ] Fault detection: sensor timeouts, brownout detection, watchdog timers
- [ ] Mesh network for 5+ node deployments
- [ ] Push / SMS alerts on threshold breach
- [ ] Battery backup + power domain isolation

---

## Challenges & How I Solved Them

| Challenge | Solution |
|---|---|
| ESP-NOW MAC addresses must be hardcoded | Recorded MACs at startup via `esp_wifi_get_mac()`, registered with `esp_now_add_peer()` |
| ESP-NOW + WiFi channel conflict | Connected WiFi first to lock channel, bound ESP-NOW peers to same channel |
| Sensor logic cancelling manual overrides | `overrideActive` boolean flag + millis() timestamp blocks auto logic for 10s |
| Servo jitter from relay switching | Power domain separation + decoupling capacitors |

---

## Real-World Applicability

The relay architecture makes this directly scalable:

- Swap LED → cooling fan, AC unit, or alarm system
- No firmware changes needed — relay handles the power domain gap
- Architecture suits pharmacy cold storage, hospital ICU supply rooms, vaccine logistics

---

*Built by Devam C Gor — 3rd year B.Tech student, Ahmedabad*
*Open to embedded / IoT firmware internships and roles*
*📍 Ahmedabad, Gujarat, India*
