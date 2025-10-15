# Automated Blockage Detection & Removal (Braccio ++ OpenCV + Nicla Vision)

[![Status](https://img.shields.io/badge/status-active-brightgreen)](#)
[![Platform](https://img.shields.io/badge/platform-Arduino%20%7C%20Python-blue)](#)
[![Boards](https://img.shields.io/badge/boards-Braccio++%20%7C%20Nano%20RP2040%20Connect%20%7C%20Nicla%20Vision-orange)](#)
[![License](https://img.shields.io/badge/license-MIT-lightgrey)](LICENSE)
[![Issues](https://img.shields.io/github/issues/Matheu2t/Automated-Blockage-Detection-and-Removal-Using-Braccio-OpenCV-and-Nicla-Vision-Camera)](../../issues)

Automates detection and removal of rock blockages in hoppers using **Nicla Vision** for on-edge detection and **Braccio++** for pick-and-place actuation. The system uses **OpenMV (on-device)** or **OpenCV (host)** pipelines and coordinates actions via **Arduino**.
---
## Table of Contents
- [Features](#features)
- [System Overview](#system-overview)
- [Hardware](#hardware)
- [Firmware & Code Layout](#firmware--code-layout)
- [Setup](#setup)
- [Build & Run](#build--run)
- [Calibration](#calibration)
- [Results](#results)
- [Troubleshooting](#troubleshooting)
- [Directory Structure](#directory-structure)
- [License](#license)
- [Acknowledgements](#acknowledgements)
---
## Features
- ✅ On-edge object detection with **Nicla Vision** (OpenMV) or host-side **OpenCV** fallback  
- ✅ **Pick-and-place** sequencing on **Braccio++** with safety limits  
- ✅ Modular pipeline (Detection → Decision → Actuation)  
- ✅ Real-time status over serial; easy logs for experiments  
- ✅ Reproducible **thesis release** (`v1.0-thesis`) + ready scripts
---
## System Overview
```mermaid
flowchart LR
  A[Camera: Nicla Vision] -->|Detections| B(OpenMV / OpenCV Pipeline)
  B -->|Decision: rock present?| C{Controller}
  C -->|Yes| D[Arduino: Braccio++ Sequencer]
  C -->|No| E[Idle / Monitor]
  D --> F[Remove & Place]
  F --> G[Log + Status over Serial]
