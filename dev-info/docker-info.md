# Docker Development Guide

## Overview

This guide explains how to use Docker for developing the coinbase-dtc-core project. Docker provides a consistent development environment across different operating systems and eliminates the need to install C++ compilers and dependencies locally.

## Prerequisites

- **Docker Desktop** (Windows/Mac) or **Docker Engine** (Linux)
- **Docker Compose** (usually included with Docker Desktop)
- **Git** for version control

### Installing Docker Desktop

1. **Windows**: Download from [Docker Desktop for Windows](https://www.docker.com/products/docker-desktop/)
2. **Mac**: Download from [Docker Desktop for Mac](https://www.docker.com/products/docker-desktop/)
3. **Linux**: Install Docker Engine and Docker Compose separately

## Project Docker Setup

The project includes two Docker configurations:

### 1. **Production Image** (Multi-stage build)
- **Purpose**: Optimized runtime image for deployment
- **Size**: Minimal (only runtime dependencies)
- **Usage**: `docker-compose up` or `docker-compose build`

### 2. **Development Container** (Profile: dev)
- **Purpose**: Interactive development environment
- **Features**: Source code mounted, build tools included
- **Usage**: `docker-compose --profile dev run coinbase-dtc-core-dev`

## Development Workflow

### Quick Start

```bash
# Clone the repository
git clone https://github.com/vitalyruhl/coinbase-dtc-core.git
cd coinbase-dtc-core

# Start development container
docker-compose --profile dev run coinbase-dtc-core-dev

# Inside the container, build and test
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/coinbase_dtc_server
```

### Development Commands

#### Starting Development Environment

```bash
# Start interactive development container
docker-compose --profile dev run coinbase-dtc-core-dev

# Start with custom command
docker-compose --profile dev run coinbase-dtc-core-dev bash

# Start and keep container running
docker-compose --profile dev up coinbase-dtc-core-dev
```

#### Inside the Development Container

```bash
# Configure CMake (Debug mode for development)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Build the project
cmake --build build

# Run the server
./build/coinbase_dtc_server

# Run JWT authentication tests (requires CDP credentials)
./build/test_jwt_auth

# Run all tests
./build/coinbase_dtc_core_tests

# Quick rebuild and run
cmake --build build && ./build/coinbase_dtc_server
```

#### Installing Additional Dependencies

```bash
# Inside the container, install development libraries
apt-get update && apt-get install -y \
    libcurl4-openssl-dev \
    nlohmann-json3-dev \
    libssl-dev \
    pkg-config

# JWT-cpp is automatically built from source (v0.7.1) for ES256/ECDSA authentication

# For debugging tools
apt-get install -y gdb valgrind

# For networking tools  
apt-get install -y curl wget netcat-openbsd
```

### File Changes and Live Development

#### Source Code Editing
- **Edit files** on your host machine (Windows/Mac/Linux)
- **Files are automatically synced** to the container via volume mounts
- **No need to rebuild container** for source code changes

#### Build Artifacts
- **Build directory** is preserved between container sessions
- **Anonymous volume** prevents build artifacts from syncing back to host
- **Clean builds** when needed with: `rm -rf build && cmake -S . -B build`

## Docker Compose Services

### Main Service Configuration

```yaml
# Production service
coinbase-dtc-core:
  build:
    context: .
    target: runtime
  ports:
    - "8080:8080"

# Development service (profile: dev)
coinbase-dtc-core-dev:
  build:
    target: builder  # Uses builder stage with dev tools
  volumes:
    - .:/app         # Mount source code
    - /app/build     # Anonymous volume for build artifacts
  profiles:
    - dev
```

### Available Commands

```bash
# Build production image
docker-compose build

# Run production container
docker-compose up

# Run production container in background
docker-compose up -d

# Stop all services
docker-compose down

# View logs
docker-compose logs

# Development container (interactive)
docker-compose --profile dev run coinbase-dtc-core-dev

# Development container (daemon)
docker-compose --profile dev up coinbase-dtc-core-dev
```

## Development Tips

### Efficient Development Cycle

```bash
# 1. Start development container
docker-compose --profile dev run coinbase-dtc-core-dev

# 2. Initial build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# 3. Quick development loop (run multiple times)
cmake --build build && ./build/coinbase_dtc_server

# 4. Test your changes
./build/coinbase_dtc_core_tests

# 5. Clean rebuild when needed
rm -rf build && cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

### Debugging in Container

```bash
# Install debugging tools
apt-get update && apt-get install -y gdb

# Build with debug symbols
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Run with debugger
gdb ./build/coinbase_dtc_server

# Memory leak detection
valgrind --leak-check=full ./build/coinbase_dtc_server
```

### Testing Network Connections

```bash
# Test Coinbase API connectivity
curl "https://api.exchange.coinbase.com/products/BTC-USD/ticker"

# Test local server (when implemented)
curl http://localhost:8080/status

# Check open ports
netstat -tuln
```

### Performance Optimization

```bash
# Build with optimizations (Release mode)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Parallel build (faster)
cmake --build build --parallel $(nproc)

# Clean build when switching modes
rm -rf build
```

## Troubleshooting

### Common Issues

#### Container Won't Start
```bash
# Check Docker daemon is running
docker ps

# Check for port conflicts
docker-compose down
docker-compose --profile dev run coinbase-dtc-core-dev

# View container logs
docker-compose logs coinbase-dtc-core-dev
```

#### Build Failures
```bash
# Clean build directory
rm -rf build

# Update base image
docker-compose pull

# Rebuild container from scratch
docker-compose build --no-cache
```

#### Permission Issues (Linux)
```bash
# Fix file permissions
sudo chown -R $USER:$USER .

# Or run container with user mapping
docker-compose --profile dev run --user $(id -u):$(id -g) coinbase-dtc-core-dev
```

#### Network Issues
```bash
# Test internet connectivity in container
curl -I https://google.com

# Check DNS resolution
nslookup api.exchange.coinbase.com

# Test specific endpoints
curl https://api.exchange.coinbase.com/products
```

### Performance Tips

#### Faster Builds
- **Use BuildKit**: Set `DOCKER_BUILDKIT=1` environment variable
- **Layer caching**: Docker Compose automatically caches unchanged layers
- **Parallel builds**: Use `cmake --build build --parallel $(nproc)`

#### Container Optimization
- **Persistent volumes**: Build artifacts are preserved between sessions
- **Multi-stage builds**: Production images are smaller and faster
- **Minimal base image**: Ubuntu 22.04 provides good balance of size and compatibility

## Integration with GitHub Actions

The project includes automated CI/CD that:

1. **Builds the Docker image** on every push
2. **Runs tests** in the container environment  
3. **Publishes images** to Docker Hub (when configured)
4. **Deploys to servers** (when configured)

### Local Testing of CI/CD

```bash
# Test the same build process as GitHub Actions
docker build -t coinbase-dtc-core .

# Run the same test as CI
docker run --rm coinbase-dtc-core

# Test multi-architecture builds (if needed)
docker buildx build --platform linux/amd64,linux/arm64 .
```

## VS Code Integration

### Remote Development

1. **Install Extensions**:
   - Remote-Containers
   - Docker
   - C/C++

2. **Open in Container**:
   - `Ctrl+Shift+P` → "Remote-Containers: Reopen in Container"
   - Or click "Reopen in Container" when prompted

3. **Configure `.devcontainer/devcontainer.json`** (optional):
   ```json
   {
     "name": "Coinbase DTC Core",
     "dockerComposeFile": "../docker-compose.yml",
     "service": "coinbase-dtc-core-dev",
     "workspaceFolder": "/app"
   }
   ```

### IntelliSense Configuration

The container provides a consistent environment for C++ IntelliSense:

- **Headers**: Standard C++17 headers available
- **Libraries**: CMake finds dependencies automatically  
- **Debugging**: GDB integration works out of the box

## Production Deployment

### Building Production Image

```bash
# Build optimized production image
docker-compose build coinbase-dtc-core

# Test production image locally
docker-compose up coinbase-dtc-core

# Tag for deployment
docker tag coinbase-dtc-core:latest your-registry/coinbase-dtc-core:v1.0.0
```

### Deployment Options

1. **Docker Hub**: `docker push your-username/coinbase-dtc-core`
2. **Container Registry**: Push to AWS ECR, Google GCR, etc.
3. **Direct Server**: Copy image with `docker save/load`

## Next Steps

1. **Start Development**: Use the development container to implement Coinbase API integration
2. **Add Dependencies**: Install required libraries (libcurl, nlohmann-json)
3. **Implement Features**: Follow the TODO.md roadmap
4. **Test Continuously**: Use the quick development cycle for rapid iteration
5. **Deploy**: Use production Docker images for server deployment

## Useful Resources

- [Docker Compose Documentation](https://docs.docker.com/compose/)
- [Multi-stage Builds](https://docs.docker.com/build/building/multi-stage/)
- [Docker Best Practices](https://docs.docker.com/develop/best-practices/)
- [VS Code Remote Development](https://code.visualstudio.com/docs/remote/containers)