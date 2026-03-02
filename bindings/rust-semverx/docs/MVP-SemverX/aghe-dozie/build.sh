# Clean up previous build
rm -rf build

# Create new build
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_ODTS_INTEGRATION=ON -DBUILD_TESTING=ON -DBUILD_EXAMPLES=ON

# Build the project
cmake --build . --parallel 4

# Run tests
ctest --verbose

# Install (optional)
# sudo cmake --install .
