# Resource-Efficient Embedded Spirometer
### Real-Time Respiratory Monitoring on STM32

## Overview

This project implements a **portable embedded spirometer** for real-time respiratory health monitoring using an STM32 microcontroller.

The system acquires airflow data from a biomedical flow sensor, processes it on-device, and computes clinically relevant lung function parameters with minimal latency and memory usage.

The primary goal is to design a **resource-efficient firmware architecture** and evaluate **performance improvements using dual-core/parallel processing techniques** for real-time biomedical signal processing.

Developed as part of **UROP – SRM Institute of Science and Technology**.

---

## Team

- Lenin Valentine – ECE, SRM KTR  
- Harshith R – ECE, SRM KTR  

Faculty Mentor:  
Dr. S. Vasanthadev Suryakala

---

## Objectives

- Real-time flow sensing and processing
- Accurate flow-to-volume computation
- Low CPU and memory footprint
- Portable embedded design
- Optimize workload using multi-core architecture
- Measure performance gains (latency, CPU load, power)

---

## System Pipeline

Flow Sensor → ADC → Calibration → Filtering → Integration → Spirometry Metrics


---

## Firmware Features

### Signal Processing
- 10 ms sampling
- Auto-zero calibration
- Noise deadband
- Trapezoidal numerical integration

### Spirometry Metrics
- Flow (SLPM / LPS)
- Volume
- FEV1
- FEV6
- FVC
- FEV1/FVC ratio
- Peak Expiratory Flow (PEF)
- Flow–Volume Loop (FVL) capture

---

## Flow–Volume Loop

During exhalation, (volume, flow) pairs are buffered to generate a flow–volume curve for analysis and visualization.

---

## Optimization Focus

This project emphasizes **efficient embedded computation**:

- Fixed-point/lightweight math where possible
- Minimal RAM usage
- Event-driven processing
- Reduced numerical drift
- Deterministic timing

### Ongoing Work
- Task separation across dual cores
- Parallel ADC + processing
- Latency benchmarking
- Throughput comparison (single vs dual core)
- Power optimization

---

## Tech Stack

- STM32 (C, HAL)
- ADC-based sensing
- Embedded signal processing
- Real-time firmware design

---

## Repository Structure

Core/Src/ → firmware
Drivers/ → HAL drivers
Docs/ → documentation


---

## Outcome

A proof-of-concept **low-cost embedded spirometer** demonstrating that accurate respiratory monitoring can be achieved on constrained hardware with optimized firmware design.
