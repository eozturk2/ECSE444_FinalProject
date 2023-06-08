# Real-time Movement Classifier for STM32 Microcontrollers

## Project Overview
The project uses accelerometer data, applies filtering processes for accuracy, and includes a user mode for labeling activity. The user-defined labels assist in refining
the system's understanding of movement patterns. All the labeled data is stored in QSPI Flash memory for efficient data management.

## Motivation
At the end of the course, we had an open-ended final project where we needed to design an implement an application for the ST B-L4S5I-IOT01A boards we had been using for the labs. We found an interesting research paper[[1]](#1) that outlined many different neural network architectures for classifying movements based on wrist-worn accelerometer data. 

In the paper, the only task of the wrist-worn device is to collect accelerometer data. Both the training and inference processes are run off-board on a powerful PC. We wanted to change this by running the inference model on the wrist-worn microcontroller.

## Methodology
The first challenge was to choose the suitable network architecture among the ones proposed in the paper. The most successful networks are RNN's (Recurrent Neural Networks), but embedded ML toolchains such as TFLite and X-CUBE-AI require recurrent units to be unrolled into dense layers with identical parameters. Since the RNN's described in the paper have up to 32 recursions, it was impossible to fit the model into the limited memory of our boards. This limited our choice to densely connected networks.

Another consideration for our choice of network was the signal processing aspect of the project. Among the dense nets proposed, the ones with overlapping windows were more successful than those with no overlap. However, implementing a sliding window with overlap presented some challenges. Since there would need to be an overlap, there would need to be a FIFO buffer, which was more difficult to implement and troubleshoot since it required tight memory management and ample debugging time. Considering our time limitation for this project (2 weeks), we decided to choose the simplest proposed network - dense network with 3 second windows without overlap.

With the architecture chosen, we moved onto implementing and 

## References
<a id="1">[1]</a>
V. Nunavath et al., “Deep learning for classifying physical activities from accelerometer data,” MDPI, https://www.mdpi.com/1424-8220/21/16/5564
