# Real-time Movement Classifier for STM32 Microcontrollers

## Project Overview
The project uses accelerometer data, applies filtering and scaling, and includes a user mode for labeling activity. The user-defined labels assist in refining
the system's understanding of movement patterns. All the labeled data is stored in QSPI Flash memory for efficient data management. There are 14 distinct activity categories available. These are brushing teeth, climbing stairs, combing hair, descending stairs, drinking from a glass, eating meat (as in with fork and knife), eating soup (as in with a spoon), getting up from bed, lying down on the bed, pouring water, sitting down on chair, standing up from chair, using telephone and walking.

## Motivation
At the end of the course, we had an open-ended final project where we needed to design an implement an application for the ST B-L4S5I-IOT01A boards we had been using for the labs. We found an interesting research paper[[1]](#1) that outlined many different neural network architectures for classifying movements based on wrist-worn accelerometer data. In the paper, the only task of the wrist-worn device is to collect accelerometer data. Both the training and inference processes are run off-board on a powerful PC. We wanted to change this by running the inference model on the wrist-worn microcontroller.

## Methodology
The first challenge was to choose the suitable network architecture among the ones proposed in the paper. The most successful networks were RNN's (Recurrent Neural Networks), but embedded ML toolchains such as TFLite and X-CUBE-AI require recurrent units to be unrolled into dense layers with identical parameters. Since the RNN's described in the paper have up to 32 recursions, it was impossible to fit the model into the limited memory of our boards. This limited our choice to densely connected networks.

Another consideration for our choice of network was the signal processing aspect of the project. Among the dense nets proposed, the ones with overlapping windows were more successful than those with no overlap. However, implementing a sliding window with overlap presented some challenges. Since there would need to be an overlap, there would need to be a FIFO buffer, which was more difficult to implement and troubleshoot since it required tight memory management and ample debugging time. Considering our time limitation for this project (2 weeks), we decided to choose the simplest proposed network - dense network with 3 second windows without overlap.

With the architecture chosen, we moved onto implementing the neural network in TensorFlow and training the model with the same accelerometer data that was used in the paper. Once that was complete, we transferred the model into a TFLite model, which the ST Microelectronics X-CUBE-AI toolchain accepted to generate weights files and AI functions that were usable with the STM32 HAL library. From this point, we incorporated the network into our C program flow.

Concurrently, we developed code that reads acceleration data over an I2C-connected accelerometer. We were careful to match the sampling rate and scaling used by the accelerometer gathering the training data. We also created a simple user interface that displays the current classification result. Since time permitted, we added a training mode accessible via pushbutton. In this mode, the user can label what activity they have been doing over the last three seconds. Then, the labeled 3 second window is saved to flash memory. If desired, the contents of flash memory can be dumped over UART terminal, allowing further training of the model on the PC.

One challenge in the implementation was the signal processing aspect of the project. Since the classification needed to be done in realtime, the ML model needed to run concurrently with the timer interrupt handler that periodically read from the accelerometer. This meant that the access to the common data buffer needed to be tightly controlled. We implemented this using simple semaphores to prevent concurrent accesses. The main inference loop waits until the buffer is full of enough samples, blocks the interrupt from accessing the buffer while it quickly copies the buffer. Then, the timer interrupt is free to fill the buffer 

## References
<a id="1">[1]</a>
V. Nunavath et al., “Deep learning for classifying physical activities from accelerometer data,” MDPI, https://www.mdpi.com/1424-8220/21/16/5564
