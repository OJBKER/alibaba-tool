# Alibaba Tool

## Overview
Alibaba Tool is a C++ application designed to interact with the Alibaba International Station API. The application features a graphical user interface (GUI) built using ImGui, allowing users to easily access and utilize the API's functionalities.

## Project Structure
```
alibaba-tool
├── src
│   ├── main.cpp          # Entry point of the application
│   ├── gui
│   │   └── imgui.cpp     # Implementation of the ImGui GUI framework
│   ├── api
│   │   └── alibaba_api.cpp # Functions to interact with the Alibaba API
│   └── utils
│       └── helpers.cpp    # Utility functions for various tasks
├── include
│   ├── imgui.h           # Header for ImGui functions and classes
│   ├── alibaba_api.h     # Header for Alibaba API functions and classes
│   └── helpers.h         # Header for utility functions
├── CMakeLists.txt        # CMake configuration file
├── README.md             # Documentation for the project
└── LICENSE               # Licensing information
```

## Setup Instructions
1. **Clone the repository:**
   ```
   git clone https://github.com/yourusername/alibaba-tool.git
   cd alibaba-tool
   ```

2. **Install dependencies:**
   Ensure you have CMake and a C++ compiler installed on your system.

3. **Build the project:**
   ```
   mkdir build
   cd build
   cmake ..
   make
   ```

4. **Run the application:**
   ```
   ./alibaba-tool
   ```

## Usage Guidelines
- Upon launching the application, you will be presented with the ImGui interface.
- Use the GUI to interact with the Alibaba International Station API.
- Refer to the API documentation for details on available functionalities and how to use them.

## Contributing
Contributions are welcome! Please submit a pull request or open an issue for any suggestions or improvements.

## License
This project is licensed under the MIT License. See the LICENSE file for more details.