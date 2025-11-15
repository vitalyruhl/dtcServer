# Docker & GitHub Actions Setup

This document explains how to run the Open DTC Server tests and builds using Docker and GitHub Actions.

## Docker Setup

### Prerequisites
- Docker installed on your system
- Docker Compose (included with Docker Desktop)

### Quick Start

1. **Build and test everything:**
   ```bash
   ./test-docker.sh build
   ./test-docker.sh test
   ```

2. **Run only basic tests:**
   ```bash
   ./test-docker.sh test-basic
   ```

3. **Start the DTC server:**
   ```bash
   ./test-docker.sh run
   ```

4. **Development environment:**
   ```bash
   ./test-docker.sh dev
   ./test-docker.sh shell
   ```

### Docker Images

The project uses a multi-stage Dockerfile with three targets:

- **`builder`** - Full build environment with all dependencies
- **`test`** - Builder + test tools (valgrind, gdb) for running tests
- **`runtime`** - Minimal production image with just the server

### Docker Compose Services

- **`test`** - Runs the complete test suite
- **`dtc-server`** - Production server on port 11000
- **`dev`** - Development environment with mounted source code

## GitHub Actions CI/CD

The project uses GitHub Actions for automated testing with two main workflows:

### 1. Docker-based Testing
- Builds and tests in Docker containers
- Tests both the test image and runtime image
- Ensures containerized builds work correctly

### 2. Native Ubuntu Testing
- Tests on Ubuntu with native compilation
- Runs basic tests that don't require external credentials
- Tests server startup functionality

### Workflow Triggers
- Push to `main`, `master`, or `develop` branches
- Pull requests to those branches

### Test Environment Variables
- `COINBASE_TEST_MODE=mock` - Enables mock testing mode
- No external API credentials required for basic tests

## Local Development

### Using Docker for Development

1. **Start development environment:**
   ```bash
   docker-compose up -d dev
   ```

2. **Access development shell:**
   ```bash
   docker-compose exec dev /bin/bash
   ```

3. **Build and test inside container:**
   ```bash
   # Inside the container
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=ON
   cmake --build build
   ctest --build-config Debug --test-dir build --tests-regex "BasicTest" --verbose
   ```

### Running Specific Tests

```bash
# Run only the basic test
docker run --rm open-dtc-server:test ctest --build-config Release --test-dir build --tests-regex "BasicTest" --output-on-failure --verbose

# Run all working tests (may fail on some due to missing dependencies)
docker run --rm open-dtc-server:test
```

### Production Deployment

1. **Build production image:**
   ```bash
   docker build --target runtime -t open-dtc-server:latest .
   ```

2. **Run production server:**
   ```bash
   docker run -d -p 11000:11000 --name dtc-server open-dtc-server:latest
   ```

## Troubleshooting

### Common Issues

1. **Build failures:**
   - Ensure Docker has enough memory allocated (4GB+ recommended)
   - Check if all dependencies are properly installed in Dockerfile

2. **Test failures:**
   - Some tests may require external credentials or network access
   - Use `test-basic` to run only self-contained tests

3. **Port conflicts:**
   - The server runs on port 11000 by default
   - Change port mapping in docker-compose.yml if needed

### Debug Builds

For debugging, use the development environment:

```bash
# Start with debug build
docker-compose exec dev /bin/bash
cd /app
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
gdb ./build/test_basic
```

## CI/CD Secrets

For full testing including JWT authentication, configure these GitHub repository secrets:

- `CDP_API_KEY_ID` - Coinbase Developer Platform API Key ID
- `CDP_PRIVATE_KEY` - Coinbase Developer Platform Private Key

These are optional and only needed for complete authentication testing.