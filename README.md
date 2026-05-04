
<img alt="POCKETLOGO" src="https://github.com/user-attachments/assets/21c22d14-1460-40d7-acda-0cd5611a5099" /><?xml version="1.0" encoding="UTF-8" standalone="no"?><!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd"><svg width="100%" height="100%" viewBox="0 0 4000 1000" version="1.1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" xml:space="preserve" xmlns:serif="http://www.serif.com/" style="fill-rule:evenodd;clip-rule:evenodd;stroke-linecap:square;stroke-miterlimit:1.5;"><rect id="Mesa-de-trabajo2" serif:id="Mesa de trabajo2" x="0" y="0" width="4000" height="1000" style="fill:none;"/><g><g><path d="M877.059,119.722c56.571,0 102.5,45.929 102.5,102.5c0,56.571 -45.929,102.5 -102.5,102.5c-56.571,0 -102.5,-45.929 -102.5,-102.5c0,-56.571 45.929,-102.5 102.5,-102.5Zm0,64.603c20.916,0 37.897,16.981 37.897,37.897c0,20.916 -16.981,37.897 -37.897,37.897c-20.916,0 -37.897,-16.981 -37.897,-37.897c0,-20.916 16.981,-37.897 37.897,-37.897Z" style="fill:#0041b9;"/><path d="M304.51,675.278c56.571,0 102.5,45.929 102.5,102.5c0,56.571 -45.929,102.5 -102.5,102.5c-56.571,0 -102.5,-45.929 -102.5,-102.5c0,-56.571 45.929,-102.5 102.5,-102.5Zm0,64.603c20.916,0 37.897,16.981 37.897,37.897c0,20.916 -16.981,37.897 -37.897,37.897c-20.916,0 -37.897,-16.981 -37.897,-37.897c0,-20.916 16.981,-37.897 37.897,-37.897Z" style="fill:#0041b9;"/><path d="M304.51,706.65c16.131,-147.633 63.796,-270.848 191.263,-365.987c123.291,-92.022 159.556,-71.366 282.304,-91.741c7.85,-1.303 15.924,-2.707 24.243,-4.239c13.026,-2.399 12.501,-1.583 25.348,-5.819m101.402,-45.508c20.643,-10.985 20.931,-11.609 36.828,-22.263c31.113,-20.852 50.951,-37.759 50.951,-37.759" style="fill:none;stroke:#0040b8;stroke-width:25px;"/><path d="M367.955,753.886c0,0 82.156,-77.296 171.501,-93.917c87.81,-16.336 66.534,51.945 156.529,61.25c101.249,10.469 167.418,-45.941 167.418,-45.941" style="fill:none;stroke:#0041b9;stroke-width:25px;stroke-linejoin:round;"/></g><g transform="matrix(750,0,0,750,3797.989939,779.96253)"></svg>

https://github.com/user-attachments/assets/ffd8a5dc-b968-441e-92b7-520ee179eb4f

Pocket is a packet sniffer for Windows built with C++ and Direct2D. It uses Npcap for capture and analysis.

## Features

* **Direct2D Rendering**: GPU-based interface for display.
* **Privacy Mode**: IP and MAC address obfuscation.
* **Analytics**: Protocol distribution charts.
* **Dual Scrolling**: Navigation for list and details.
* **Adapter Support**: Capture on interfaces.
* **Npcap**: Capture backend.

## Requirements

* Windows 7+.
* Npcap (you'll be prompted to the installer otherwise).
* Visual C++ Redistributable.

## Build

* Windows 10 or later.
* Visual Studio 2022.
* Npcap SDK.
* Windows SDK.

### Compilation
```powershell
MSBuild Pocket.sln /p:Configuration=Release /p:Platform=x64
```

## Installation

1. Build `Release|x64`.
2. Run `ISCC.exe Pocket.iss`.
3. Execute `Output/PocketSetup.exe`.
