# Object Recognition System (CS5330 – Project 2)

**Author:** Kaiyuan Lu  
**Group Members:** None (individual project)

---

## Project Demonstration Video
- **Demo Video Link:** [YouTube – Object Recognition System Demo](https://youtube.com/shorts/l3H9EnLRjoI?feature=share)

---

## Project Overview
This project implements a **real-time 2D object recognition system** using OpenCV in C++.  
The system performs the following pipeline tasks:
1. **Thresholding:** Automatically separates the object from the background using a dynamic K-Means (K=2) threshold.
2. **Morphological Filtering:** Cleans up the binary image using custom and OpenCV-based opening/closing operations.
3. **Connected Components:** Segments the image into individual regions and selects the most relevant object.
4. **Feature Extraction:** Computes translation, scale, and rotation-invariant features such as Hu moments, fill ratio, aspect ratio, and orientation.
5. **Training Mode:** Allows saving of object features (press `N`) into a CSV database.
6. **Classification:** Classifies new objects using a nearest-neighbor distance metric.
7. **Performance Evaluation:** Logs predicted vs. true labels to generate a 5×5 confusion matrix.
8. **CNN One-Shot Classification (Task 9):**  
   Uses **ResNet18 (resnet18-v2-7.onnx)** to compute CNN embeddings and classify objects based on sum-squared distance (SSD).

---

## Development Environment

| Component | Details |
|------------|----------|
| **Operating System** | Windows 10 64-bit |
| **IDE / Compiler** | Microsoft Visual Studio 2022 (C++17) |
| **OpenCV Version** | 4.12.0 |
| **ONNX Runtime** | 1.22.1 |
| **Deep Model Used** | `resnet18-v2-7.onnx` |
| **Hardware** | Intel CPU (no GPU acceleration) |

---

## ▶Instructions to Run

1. Make sure the download file.
2. Compile and run the program.

**If using Visual Studio:**
- Open the project.
- Build and run (`Ctrl + F5`).

**If using terminal (with OpenCV 4.12+):**
```bash
g++ main.cpp -o ObjectRecognition.exe -std=c++17 `pkg-config --cflags --libs opencv4`
./ObjectRecognition.exe

