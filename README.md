# Real-time Movement Classifier for STM32 Microcontrollers

## Project Overview
The project uses accelerometer data, applies filtering processes for accuracy, and includes a user mode for labeling activity. The user-defined labels assist in refining
the system's understanding of movement patterns. All the labeled data is stored in QSPI Flash memory for efficient data management.

## Motivation
There was a paper that took the accelerometer data and ran the inference on a PC. We implemented one of the models described in the paper, trained it, and
transferred it to the microcontroller.

## Features
1 - Real-time movement classification
2 - User activity labelling mode
3 - QSPI Flash storage for labeled training data
