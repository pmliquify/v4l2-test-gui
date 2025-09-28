# V4L2 Test GUI - AI Assistant Instructions

## Project Overview
A Qt6-based desktop application that receives and displays camera images from V4L2 drivers via TCP/IP. Designed for embedded camera driver development where images are captured on headless systems and displayed on development workstations.

## Architecture & Key Components

### Core Components
- **`MainWindow`**: Primary Qt application window managing UI state and orchestrating other components
- **`SocketServer`**: TCP server handling binary image data reception using state machine pattern
- **`ImageWidget`**: Custom Qt widget for image display with interactive ROI (Region of Interest) features
- **`Image`/`Plane`**: Core data structures representing multi-plane camera images
- **`ProjectManager`**: Handles saving/loading ROI configurations and analysis settings

### Data Flow
1. TCP client sends binary image data following `ImageHeader` + plane data protocol (see `sockettypes.hpp`)
2. `SocketServer` receives data using state machine (`WAITING_FOR_HEADER` → `WAITING_FOR_PLANE_SIZE` → `WAITING_FOR_PLANE_DATA`)
3. Raw image data converted to `QImage` via OpenCV (`convert.cpp`)
4. `ImageWidget` displays image with interactive ROI overlays and analysis functions

### Build System
- **CMake-based**: Root CMakeLists.txt + subdirectory structure (`src/common/`, `src/v4l2-test-gui/`)
- **Dependencies**: Qt6 (Widgets, Network), OpenCV
- **Cross-platform**: Special handling for macOS app bundles with icon resources

## Development Patterns

### Qt Integration
- Uses Qt's MOC system (`CMAKE_AUTOMOC ON`) - headers with Q_OBJECT must be properly declared
- UI files (`mainwindow.ui`) auto-processed via `CMAKE_AUTOUIC`
- Resource files (`mainwindow.qrc`) for embedded assets via `CMAKE_AUTORCC`

### Image Processing Pipeline
- Raw V4L2 formats converted through OpenCV Mat → QImage chain
- Stride offset handling for debug/alignment issues (`m_strideOffset` in MainWindow)
- Support for both processed and raw image display modes

### ROI & Analysis System
- Interactive ROI drawing/editing with resize handles (8-point resize system in `ImageWidget`)
- Plugin-style analysis functions using factory pattern (`createFunction()` in `function.hpp`)
- Currently supports Histogram analysis, designed for extensibility
- Project files save ROI configurations as JSON

### State Management
- Connection status tracking (`m_connected`, `m_imageReceived` flags)
- FPS calculation using sequence numbers and timestamps
- Settings persistence via QSettings for window state and last project

## Key File Relationships

- `sockettypes.hpp`: Defines binary protocol structures shared between client/server
- `convert.cpp`: Handles V4L2 → OpenCV → Qt image format conversions
- `imagewidget.cpp`: Implements complex mouse interaction for ROI manipulation
- `projectmanager.cpp`: JSON serialization for analysis configurations
- `common/` library: Shared image/plane data structures used by both GUI and potential CLI tools

## Development Commands

```bash
# Standard build (from project root)
mkdir build && cd build
cmake .. && cmake --build .

# Run executable (macOS)
./src/v4l2-test-gui/v4l2-test-gui.app/Contents/MacOS/v4l2-test-gui

# Docker development environment
cd doc/ubuntu22_docker_umgebung
./run-docker-env.sh
```

## Testing & Debugging
- Application expects TCP clients on configurable port (default varies)
- Image data follows specific binary protocol - see `ImageHeader` struct for format
- OpenCV Mat debugging: Check format, channels, and stride alignment issues
- ROI coordinates are tracked in both widget and image coordinate systems

## Platform-Specific Notes
- **macOS**: App bundle creation with custom icon handling
- **Linux**: Standard executable, Docker environment provided for Ubuntu 22.04
- Icon generation scripts in `src/v4l2-test-gui/icons/` for platform-specific formats