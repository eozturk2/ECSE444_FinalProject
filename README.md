# Real-time Movement Classifier for STM32 Microcontrollers

## Project Overview
The project uses accelerometer data, applies filtering and scaling, and includes a user mode for labeling activity. The user-defined labels assist in refining
the system's understanding of movement patterns. All the labeled data is stored in QSPI Flash memory for efficient data management. There are 14 distinct activity categories available. These are brushing teeth, climbing stairs, combing hair, descending stairs, drinking from a glass, eating meat (as in with fork and knife), eating soup (as in with a spoon), getting up from bed, lying down on the bed, pouring water, sitting down on chair, standing up from chair, using telephone and walking.

## Motivation
At the end of the course, we had an open-ended final project where we needed to design an implement an application for the ST B-L4S5I-IOT01A boards we had been using for the labs. We found an interesting research paper[[1]](#1) that outlined many different neural network architectures for classifying movements based on wrist-worn accelerometer data. In the paper, the only task of the wrist-worn device is to collect accelerometer data. Both the training and inference processes are run off-board on a powerful PC. We wanted to change this by running the inference model on the wrist-worn microcontroller.

## Implementation and Challenges
The first challenge was to choose the suitable network architecture among the ones proposed in the paper. The most successful networks were RNN's (Recurrent Neural Networks), but embedded ML toolchains such as TFLite and X-CUBE-AI require recurrent units to be unrolled into dense layers with identical parameters. Since the RNN's described in the paper have up to 32 recursions, it was impossible to fit the model into the limited memory of our boards. This limited our choice to densely connected networks.

Another consideration for our choice of network was the signal processing aspect of the project. Among the dense nets proposed, the ones with overlapping windows were more successful than those with no overlap. However, implementing a sliding window with overlap presented some challenges. Since there would need to be an overlap, there would need to be a FIFO buffer, which was more difficult to implement and troubleshoot since it required tight memory management and ample debugging time. Considering our time limitation for this project (2 weeks), we decided to choose the simplest proposed network - dense network with 3 second windows without overlap.

With the architecture chosen, we moved onto implementing the neural network in TensorFlow and training the model with the same accelerometer data [[2]](#1) that was used in the paper. Once that was complete, we transferred the model into a TFLite model, which the ST Microelectronics X-CUBE-AI toolchain accepted to generate weights files and AI functions that were usable with the STM32 HAL library. From this point, we incorporated the network into our C program flow.

Concurrently, we developed code that reads acceleration data over an I2C-connected accelerometer. We were careful to match the sampling rate and scaling used by the accelerometer gathering the training data. We also created a simple user interface that displays the current classification result. Since time permitted, we added a training mode accessible via pushbutton. In this mode, the user can label what activity they have been doing over the last three seconds. Then, the labeled 3 second window is saved to flash memory. If desired, the contents of flash memory can be dumped over UART terminal, allowing further training of the model on the PC.

One challenge in the implementation was the signal processing aspect of the project. Since the classification needed to be done in realtime, the ML model needed to run concurrently with the timer interrupt handler that periodically read from the accelerometer. This meant that the access to the common data buffer needed to be tightly controlled. We implemented this using simple semaphores to prevent concurrent accesses. The main loop waits until the buffer is full of enough samples, blocks the interrupt from accessing the buffer while it quickly copies the buffer. Then, the timer interrupt is free to fill the buffer while the main loop runs the inference.

## Results
After the implementation was complete, we demoed it for our professor. We strapped our boards to our wrists using rubber bands and connected the board to the laptop via USB. According to our anectodal observations, the model was accurate around half the time, across 14 different movement categories. We believe that this is decently close to the 59% accuracy reported for the same network running on a PC. We found that the model could discern very well between these broad categories:

1 - Lying down
2 - Walking tasks: climbing stairs, descending stairs, walking
3 - Sitting tasks: sitting on chair, eating soup, eating meat, pouring water
4 - "Hand to the face" tasks: using telephone, brushing teeth, combing hair

There were almost no mistakes between these four broad categories. Most of the confusion was while discerning between tasks within each category. For example, descending and climbing stairs were two actions that were often confused, while both of them could sometimes be discerned from walking. Sedentary tasks were almost randomly guessed, but the "hand to the face" tasks were mostly accurate.

We believe that the difference between the reported accuracy and our model's accuracy originate mostly from the accelerometer measurements. The dataset we used contained information about the orientation of the accelerometer, its sampling rate and the scale of its measurements.[[2]](#1) Missing were the exact location of the accelerometer on the wrist, the model and make of the device, and physical mounting details. Since these details were unknown to us, we were unable to exactly replicate the data acquisition process. 

Moreover, our less-than-ideal mounting method (rubber bands, wired connection to laptop that we had to carry on the other hand) meant that the accelerometer was not firmly attached. Shifts in position during active use may have contributed to the loss in accuracy as compared to the same model running on the PC.

Another possible reasons include our assumptions about the dataset. In the dataset, the movement data are labeled by gender. We assumed that this was not an important detail, and that the same action would generate a similar acceleration pattern regardless of the user's gender. For this reason, we simply disregarded the gender label and used all the dataset as training data. 

## References
<a id="1">[1]</a>
V. Nunavath et al., “Deep learning for classifying physical activities from accelerometer data,” MDPI, https://www.mdpi.com/1424-8220/21/16/5564

<a id="1">[2]</a>
“UCI Machine Learning Repository: Dataset for ADL Recognition
with Wrist-worn Accelerometer Data Set,” archive.ics.uci.edu.
https://archive.ics.uci.edu/ml/datasets/Dataset+for+ADL+Recognition
+with+Wrist-worn+Accelerometer
