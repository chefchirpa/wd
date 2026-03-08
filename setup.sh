#!/bin/bash

echo "Updating package lists..."
sudo apt-get update

echo "Installing C++ build tools and CMake..."
sudo apt-get install -y build-essential cmake

echo "Installing libcurl for Google Sheets CSV fetching..."
sudo apt-get install -y libcurl4-openssl-dev

echo "Setup complete!"
