# SmartParkingAssistant (C++ / Sockets / Linux)

A client-server parking assistant built in C++ using **TCP sockets** on Linux.  
The system manages a small parking lot (10 spots), supports reservations, live availability queries, real-time notifications, and a distance-based recommendation feature.

This project includes:
- a **server** that keeps the parking state
- a **client** CLI for user commands
- a **sensor simulator** that sends periodic occupancy updates
- a **camera simulator** that “scans” spots and updates changes probabilistically

---

## Core Features

### Server (TCP, `select()`)
- Handles multiple clients concurrently using **I/O multiplexing (`select`)**
- Maintains parking spot states via a `Parcare` class:
  - `0 = free`, `1 = occupied`
- Supports a simple text protocol (length-prefixed messages)

### Client (interactive CLI)
Commands available:
- `connect` – connect to server
- `list_free` – request list of free spots
- `reserve <id>` – reserve a spot
- `watch <id>` – subscribe to notifications when a spot becomes free
- `recommend <x> <y>` – recommends the closest free spot to the given coordinates
- `disconnect` / `exit`

### Real-time notifications (`watch`)
If a client watches a spot that is currently occupied, the server stores the subscription.
When the spot becomes **free**, the server pushes:
- `alert free <id>`

### Recommendation (`recommend`)
- Maps each spot ID to a 2-row coordinate layout:
  - odd IDs on row `y=0`, even IDs on row `y=1`
- Chooses the closest free spot using **Euclidean distance**

### Sensor + Camera simulators
- `senzor.cpp`: periodically picks a random spot and sends `update <id> liber|ocupat`
- `camera.cpp`: scans a small number of random spots at intervals and sometimes flips state (probability-based), then sends updates if it detects changes

---

## Communication Protocol

Messages are sent as:
1) 4-byte length (network byte order)
2) message bytes

Implemented in:
- `send_message(...)`
- `recv_message(...)`

Examples:
- `list_free`
- `reserve 3`
- `update 7 ocupat`
- `watch 2`
- `recommend 1.50 0.20`

---

## Project Files

- `server.cpp` – TCP server, command handling, notifications, recommendation logic
- `client.cpp` – interactive client, menu, watch mode using `select()`
- `Parcare.h / Parcare.cpp` – parking state + operations
- `senzor.cpp` – sensor update simulator
- `camera.cpp` – camera scan simulator

---

## Build & Run (Linux)

### 1) Compile
```bash
g++ -std=c++17 server.cpp Parcare.cpp -o server
g++ -std=c++17 client.cpp -o client
g++ -std=c++17 senzor.cpp Parcare.cpp -o senzor
g++ -std=c++17 camera.cpp Parcare.cpp -o camera
