# Home Alarm System (ARM mbed NXP LPC1768)

## Introduction
This project implements a **home alarm system** using the **ARM mbed NXP LPC1768** microcontroller.  
It simulates a basic alarm system with **virtual switches** acting as sensors and an **LED** serving as the alarm indicator.

The system design includes several **Unified Modeling Language (UML)** diagrams—such as class diagrams, state machine diagrams, and sequence diagrams—to illustrate both the **architecture** and **behavior** of the system.  
The main functionality is developed in **C++** and tested using the **ARM Keil Studio online compiler**.

## System Overview
The system operates through **six main states**:
1. **Unset**
2. **Exit**
3. **Set**
4. **Entry**
5. **Alarm**
6. **Report**

Transitions between these states are controlled by **user input** (via a keypad) and **sensor activations** (via virtual switches).  
A **four-digit passcode** is required for system activation, ensuring basic security control.

---

> **Note:** This project was developed for educational purposes to demonstrate state-machine-based design using the ARM mbed platform.
