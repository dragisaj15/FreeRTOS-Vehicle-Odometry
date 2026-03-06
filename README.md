# Vehicle Speed and Distance Measurement using FreeRTOS

## Introduction

This project is a software simulation of a vehicle speed and distance measurement system, developed using the **FreeRTOS** operating system within the **Microsoft Visual Studio Community** environment. The primary goal of the project was to implement a solution that simulates the operation of peripherals and RTOS tasks, alongside establishing serial communication with a personal computer.

## Project Description and Features

The project simulates a system for measuring a car's speed and traveled distance. Data regarding wheel rotation is obtained from two encoders (left and right wheel), which are simulated via serial channels 0 and 1. Communication with the PC is handled via serial channel 2.

### Key Features:

* **Distance and Speed Measurement:** The system receives increment data from the two encoders (36,000 increments per revolution) every 200ms. Based on this data, the total distance traveled and current speed are calculated.
* **PC Communication (Channel 2):** Bi-directional communication is enabled using a serial communication simulator (AdvUniCom).
    * **Wheel Circumference Configuration:** By sending a message in the format `OM[circumference]\0d` (e.g., `OM200\0d` for 200 cm), the user can set the wheel's circumference.
    * **Distance Reset:** By sending the `RESET\0d` command via the PC, the total traveled distance is reset to zero.
* **LAP Measurement (Distance within a specific interval):** The `START` and `STOP` commands are simulated by pressing buttons on the LED BAR, which triggers the measurement of the distance traveled between these two events.
* **Display Output:** The measured data is displayed on a multiplexed 7-segment display (`Seg7Mux` simulator). The user selects what data is displayed by pressing the corresponding "buttons" on the LED BAR.

### Simulated Peripherals Used

Since the implementation is executed on a PC, the project utilizes hardware simulators to represent physical peripherals:
* **AdvUniCom:** A serial communication simulator providing three channels (0, 1, and 2).
* **LED_bars_plus:** A simulator for logic inputs and outputs (LEDs). It is used to simulate hardware push-buttons and indicate system status.
* **Seg7_Mux:** A 7-segment display simulator.

## Task Overview

The project consists of the following FreeRTOS tasks:

* `SerialSend_Task0`: Periodically sends a trigger message to serial channels 0 and 1 to request increment data.
* `SerialReceive_Task0`: Receives increment data from the left wheel (channel 0), processes it, and sends it to a queue.
* `SerialReceive_Task1`: Receives increment data from the right wheel (channel 1), processes it, and sends it to a queue.
* `SerialReceive_Task2`: Handles PC communication. It receives commands for configuring the wheel circumference and resetting the total distance.
* `PutEnc_Task`: Calculates the traveled distance based on the increments from both wheels, as well as the LAP distance.
* `MerenjeBrzine_Task` (Speed Measurement): Calculates the current speed based on the traveled distance and sends it to the PC.
* `SerialSend_Task2`: Transmits the current traveled distance data to the PC.
* `LCD_Task`: Responsible for displaying the measured data on the 7-segment display based on the current state of the LED BAR.

## Simulation Instructions

To run the simulation, follow these steps:

1.  **Launch the Peripherals:**
    * **AdvUniCom:** Run `AdvUniCom.exe 0` for channel 0 and `AdvUniCom.exe 1` for channel 1. In the channel 0 window, set the trigger message to send 'X' and set the automatic response format to `L12000<` (where 12000 represents a sample increment value). Do the same for channel 1, with the trigger 'X' and format `R12000<`. Use channel 2 to send commands: `OMxxx\0d` to set the wheel circumference and `RESET\0d` to reset the total distance.
    * **LED_bars_plus:** Run `LED_bars_plus.exe rGrr` to initialize 3 input columns and 1 output column.
    * **Seg7_Mux:** Run `Seg7_Mux.exe 9` to initialize a 9-digit display.

2.  **Run the project in Visual Studio.**

3.  **Use Serial Communication Commands (Channel 2):**
    * To set the wheel circumference: Send a message in the format `OM[circumference_in_cm]\0d` and press Enter. For example, `OM200\0d`.
    * To reset the distance: Send `RESET\0d` and press Enter.

4.  **Control the Display and Measurements via the LED BAR:**
    * Pressing the top LED in the 3rd column displays the **speed** and **delta distance**.
    * Pressing the bottom LED in the 3rd column displays the **total distance** and **LAP distance**. *(Note: The top and bottom LEDs in the 3rd column cannot be active simultaneously, as this will result in a blank display).*
    * Pressing the top LED in the 1st column starts the **LAP measurement**, and pressing the second LED from the top in the 1st column stops it. While the LAP measurement is active, an indicator LED (top LED in the 2nd column) will remain illuminated.
