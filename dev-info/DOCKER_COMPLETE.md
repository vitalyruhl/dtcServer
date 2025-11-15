# Docker Setup & Deployment Guide

## 🎯 Overview

Complete Docker setup for the coinbase-dtc-core project with production deployment and development environment support.

## ✅ Current Status

**Container Status**: ✅ **PRODUCTION READY**
- **Docker build**: Successful  
- **Server startup**: Running on port 11099
- **DTC Server**: CoinbaseDTCServer operational
- **Live Data**: Coinbase WebSocket streaming
- **Authentication**: JWT/SSL working

## 🚀 Quick Start

### 1. Production Deployment
```bash
# Build production container
docker build -f Dockerfile.simple -t coinbase-dtc-simple .

# Run production server
docker run -d --name coinbase-dtc-server -p 11099:11099 coinbase-dtc-simple

# Check server logs
docker logs coinbase-dtc-server

# Connect to running container
docker exec -it coinbase-dtc-server bash
```

### 2. Development Environment
```bash
# Build and start dev environment
docker-compose up --build

# Development with mounted source
docker-compose -f docker-compose.yml up dev
```

## 🏗️ Docker Configurations

### Production Image (Multi-stage)
- **File**: `Dockerfile.simple`
- **Purpose**: Optimized runtime deployment
- **Size**: Minimal (runtime dependencies only)
- **Features**: 
  - Ubuntu 22.04 base
  - C++17 runtime
  - DTC server binary
  - SSL certificates
  - Port 11099 exposed

### Development Container
- **File**: `docker-compose.yml` (dev profile)
- **Purpose**: Interactive development
- **Features**:
  - Source code mounted
  - Build tools included
  - Hot reload support
  - Debug symbols

## 📦 Core Libraries (All Building Successfully)

| Library | Purpose | Status |
|---------|---------|---------|
| `dtc_util` | DTC utility functions | ✅ |
| `dtc_protocol` | Core DTC protocol | ✅ |
| `dtc_test` | Testing framework | ✅ |
| `dtc_auth` | Authentication | ✅ |
| `exchange_base` | Base exchange interface | ✅ |
| `coinbase_feed` | Coinbase integration | ✅ |

## 🔧 Available Commands

### Docker Compose Commands
```bash
# Build all services
docker-compose build

# Start production server
docker-compose up coinbase-dtc-server

# Start development environment
docker-compose up dev

# Run tests in container
docker-compose run --rm test

# Clean up
docker-compose down
```

### Manual Docker Commands
```bash
# Build development image
docker build -f Dockerfile -t coinbase-dtc-dev .

# Run with volume mounting
docker run -v $(pwd):/app -p 11099:11099 coinbase-dtc-dev

# Interactive development session
docker run -it --rm -v $(pwd):/app coinbase-dtc-dev bash
```

## 🌐 Network Configuration

### Port Mapping
- **Host Port**: 11099
- **Container Port**: 11099
- **Protocol**: TCP
- **Interface**: 0.0.0.0 (all interfaces)

### Environment Variables
```bash
# Production
ENVIRONMENT=production
LOG_LEVEL=info
DTC_PORT=11099
SERVER_NAME=CoinbaseDTCServer

# Development
ENVIRONMENT=development
LOG_LEVEL=debug
ENABLE_MOCK_DATA=true
```

## 🔐 Security & Credentials

### Production Deployment
```bash
# Mount secrets directory
docker run -d \
  -p 11099:11099 \
  -v /path/to/secrets:/app/secrets:ro \
  --name coinbase-dtc-server \
  coinbase-dtc-simple
```

### Environment Variables
```bash
# Set Coinbase credentials
docker run -d \
  -p 11099:11099 \
  -e COINBASE_API_KEY=your_key \
  -e COINBASE_API_SECRET=your_secret \
  --name coinbase-dtc-server \
  coinbase-dtc-simple
```

## 🧪 Testing in Docker

### Run All Tests
```bash
docker build -f Dockerfile.test -t coinbase-dtc-test .
docker run --rm coinbase-dtc-test
```

### Specific Test Suites
```bash
# Protocol tests
docker run --rm coinbase-dtc-test ./test_dtc_protocol

# Integration tests  
docker run --rm coinbase-dtc-test ./test_dtc_integration

# Client tests
docker run --rm coinbase-dtc-test ./test_dtc_client
```

## 📊 Monitoring & Logs

### View Server Logs
```bash
# Follow logs in real-time
docker logs -f coinbase-dtc-server

# Show last 100 lines
docker logs --tail 100 coinbase-dtc-server

# Show logs with timestamps
docker logs -t coinbase-dtc-server
```

### Health Check
```bash
# Check if server is responding
docker exec coinbase-dtc-server netstat -ln | grep 11099

# Test DTC connection
docker exec coinbase-dtc-server ./dtc_test_client
```

## 🔄 CI/CD Integration

### GitHub Actions
```yaml
# .github/workflows/docker.yml
name: Docker Build and Test
on: [push, pull_request]
jobs:
  docker-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build and test
        run: |
          docker build -f Dockerfile.test -t test .
          docker run --rm test
```

### Local CI Testing
```bash
# Validate build locally
./scripts/test-docker.sh build

# Run full test suite
./scripts/test-docker.sh test

# Start development server
./scripts/test-docker.sh run
```

## 🔧 Troubleshooting

### Common Issues

**Port Already in Use**
```bash
# Find process using port 11099
netstat -tulpn | grep 11099

# Kill existing container
docker stop coinbase-dtc-server
docker rm coinbase-dtc-server
```

**Build Failures**
```bash
# Clean Docker cache
docker system prune -a

# Rebuild without cache
docker build --no-cache -f Dockerfile.simple .
```

**Container Won't Start**
```bash
# Check container logs
docker logs coinbase-dtc-server

# Run in foreground for debugging
docker run --rm -p 11099:11099 coinbase-dtc-simple
```

## 📁 File Structure

```
├── Dockerfile.simple       # Production container
├── Dockerfile              # Development container  
├── Dockerfile.test         # Testing container
├── docker-compose.yml      # Multi-service setup
├── .dockerignore           # Docker ignore patterns
└── scripts/
    ├── docker-entrypoint.sh
    └── test-docker.sh
```

## 🎯 Next Steps

1. **Production Deployment**: Ready for cloud deployment
2. **Monitoring**: Add metrics and health endpoints  
3. **Scaling**: Multi-container setup for high availability
4. **Security**: Production secrets management