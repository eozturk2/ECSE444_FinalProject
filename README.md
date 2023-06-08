# Real-time Movement Classifier for STM32 Microcontrollers

## Project Overview
The project uses accelerometer data, applies filtering processes for accuracy, and includes a user mode for labeling activity. The user-defined labels assist in refining
the system's understanding of movement patterns. All the labeled data is stored in QSPI Flash memory for efficient data management.

## Motivation
At the end of the course, we had an open-ended final project where we needed to design an implement an application for the ST B-L4S5I-IOT01A boards we had been using for the labs. We found an interesting research paper[[1]](#1) that outlined many different neural network architectures for classifying movements based on wrist-worn accelerometer data. 

In the paper, the only task of the wrist-worn device is to collect accelerometer data. Both the training and inference processes are run off-board on a powerful PC. We wanted to change this by running the inference model on the wrist-worn microcontroller.

## Features
1 - Real-time movement classification
2 - User activity labelling mode
3 - QSPI Flash storage for labeled training data

## References
<a id="1">[1]</a>
V. Nunavath et al., “Deep learning for classifying physical activities from accelerometer data,” MDPI, https://www.mdpi.com/1424-8220/21/16/5564
