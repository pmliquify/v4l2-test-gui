# GUI to display images acquired with v4l2-test

The goal of this application is to support the development of v4l2 camera drivers and to provide customers with a simple way to display images from a v4l2 camera. The application relies on images being sent via TCP/IP, making it easy to view images from a headless embedded system on a host PC.

## Version 0.3.0

## Build

```bash
sudo apt update
sudo apt install -y build-essential cmake qt6-base-dev libopencv-dev
git clone https://github.com/pmliquify/v4l2-test-gui.git
cd v4l2-test-gui
mkdir build
cd build
cmake ..
cmake --build .
```

You can find the executable in the build/src/v4l2-test-gui/ directory.