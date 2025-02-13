# 🎨 Paint Application
## 📝 Introduction
<p>Real-time drawing applications play a crucial role in user experience in computer graphics. In traditional drawing applications, each pixel needs to be manipulated individually, which can result in performance issues and increased latency. Especially in high-resolution environments and complex geometric operations, data processing times can negatively impact the overall user interaction.</p>

<p>This project focuses on optimizing geometric shape rendering and freehand drawing using <b>C++</b> and the <b>ICBYTES</b> Library. By implementing a multi-threaded architecture and parallel processing techniques, the goal is to significantly improve the drawing performance and user experience.</p>

<p>This project is a simple Paint application developed using ICBYTES. Users can free-draw using various drawing tools, draw different shapes, and save their drawings.</p>

## 🎯 Purpose of Project
This project aims to develop a high-performance and multi-threaded drawing application. The main objectives are:
<p>✔ Providing instant feedback to the user by using a temporary canvas</p>
<p>✔ Improving drawing performance with multi-threaded algorithms</p>
<p>✔ Creating a flexible structure that supports various geometric shapes</p>
<p>✔ Implementing layer-based drawing with a canvas that supports transparency (alpha channel)</p>

## ✨ Features
📌 Temporary Canvas and Drawing Management
The project is based on three separate canvas structures:
<p>1️⃣ <b> Main Canvas (m):</b> The primary structure where permanent drawings are stored, and freehand drawing takes place.</p>
<p>2️⃣ <b>Temporary Canvas (tempCanvas):</b> Used for temporary drawings and real-time user interactions.</p>
<p>3️⃣ <b>Buffer Canvas (backBuffer):</b> The structure where all drawings are merged and projected on the screen.</p>

 **✔ How it works?**

During drawing, shapes are first drawn on the temporary canvas (tempCanvas) and displayed in real time.
When the user releases the left mouse button, the drawing is merged into the main canvas (m), and tempCanvas is cleared.
This structure optimizes the drawing experience by providing instant feedback to the user.

![image](https://github.com/user-attachments/assets/27d1fcfc-62c7-4ee3-89b2-35857228d650)
<p>🔹 Figure 1. Paint Application First Look</p>

### 📌 Line Drawing Algorithm
Using Bresenham's Algorithm, a high-performance and efficient line drawing technique is implemented.

🔹 **What is Bresenham's Algorithm?**
Bresenham's Line Algorithm is a fast and optimized rasterization technique that determines pixels along a straight line using only integer calculations instead of floating-point operations.
<p>✔ This minimizes the gaps between pixels and ensures smooth rendering.</p>
<p>✔ With the multi-threaded implementation, the line drawing is computed in parallel.</p>
<p>✔ Thick line support is implemented using normal vector calculations.</p>

### Example Usage in Code:

```
void DrawLine(ICBYTES& canvas, int x1, int y1, int x2, int y2, int color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        Line(canvas, x1, y1, x2, y2, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}
```

### 🚀 Performance Enhancement with Parallel Processing
Drawing operations are significantly accelerated using multi-threading.

**Traditional vs. Multi-threaded Drawing**
<p>🔹 In single-threaded drawing, CPU bottlenecks can slow down rendering, especially for high-resolution graphics.</p>
<p>🔹 In this project, each drawing task runs on a separate thread, making better use of multi-core processors.</p>

**✔ How It Works?**

Thread Pool Mechanism: Distributes workload efficiently, reducing the overhead of thread creation.
Parallel Processing: Each drawing runs in its own thread, preventing UI freezing and improving responsiveness.
Mutex Synchronization: Ensures thread safety, preventing data inconsistency during concurrent drawing operations.

### 📌 Comparison Before & After Optimization:

|Feature|Single-threaded|Multi-threaded|
|--------|-------------|---------------|
|Drawing Speed|Slow|Fast|
|UI Responsiveness|Lags|Smooth|
|CPU Utilization|Inefficient|Optimized|

✔ **Result:** Multi-threaded rendering has significantly improved the user experience by providing real-time drawing performance.

### 🎨 User Interface (GUI) Design
A user-friendly and intuitive interface has been designed to make the drawing experience smooth and efficient.

### 📌 Main Features
<p>✔ <b>Color Selection Panel:</b> Choose from predefined colors or adjust custom colors with a trackbar.</p>
<p>✔ <b>Shape Selection Buttons:</b> Draw circles, triangles, rectangles, lines with one-click.</p>
<p>✔ <b>Line Thickness Adjustment:</b> Adjust line thickness dynamically using a trackbar.</p>

![image](https://github.com/user-attachments/assets/f9e05bd2-4264-434e-a8e3-b33a3ea50e87)
<p>🔹 Figure 2. User Interface with Tools</p>

### 🖊️ Pencil Tool
✔ Freehand drawing tool that allows users to draw smoothly with adjustable thickness.

![image](https://github.com/user-attachments/assets/797b974d-7f9d-4df8-a4c8-67723e8a2029)
<p>🔹 Figure 3. Shows Line Thickness</p>

![image](https://github.com/user-attachments/assets/82f8da2f-53a0-4f37-acc8-0b5805df373a)
<p>🔹 Figure 4. Drawing function</p>

### ✏️ Eraser Tool
<p>✔ Erase specific areas of the drawing canvas.</p>
<p>✔ The eraser size can be adjusted using the line thickness slider.</p>

![image](https://github.com/user-attachments/assets/40e41172-0202-4249-bdf1-3ffaab428ff7)
<p>🔹 Figure 5. Showing how eraser works </p>

### 🧹 Clear Button
<p>✔ Clears everything from the canvas, allowing users to start fresh.</p>

![image](https://github.com/user-attachments/assets/b14b71f3-5efa-4fd9-8aed-66380ec73af3)
<p>🔹 Figure 6. After clicked clear button </p>

### 💾 Save Button
<p>✔ Saves the drawing as a BMP file.</p>
<p>✔ Uses Windows API file dialog for choosing file name & location.</p>

![image](https://github.com/user-attachments/assets/0def2367-e8ab-46e0-a421-a73ef4fe0d56)
<p>🔹 Figure 7. Save canvas </p>


### 🔧 System Requirements
**Operating System:** Windows 10 or later
**Development Environment:** Visual Studio 2022
**Required Libraries:** 
```icb_gui.h, ic_media.h, icbytes.h from ICBYTES Library```  [You can download it here](https://github.com/cembaykal/ICBYTES)

### 🛠️ Installation Guide

Clone the Project
```
git clone https://github.com/yourusername/paint-app.git
cd paint-app
```

**Open in Visual Studio & Build**
<p>Open the project in Visual Studio 2022 </p>
<p>Compile and run </p>

### 🔍 References
<p>[1] Baykal, I. C. (2024). ICBYTES: A Simplified C++ Library. </p> 
<p>[2] Foley, J., van Dam, A., Feiner, S., & Hughes, J. (1996). Computer Graphics: Principles and Practice.</p>
<p>[3] Bresenham, J. E. (1965). Algorithm for computer control of a digital plotter.</p>

