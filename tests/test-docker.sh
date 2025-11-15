#!/bin/bash

# Docker Test Runner Script for Open DTC Server
# This script provides easy commands for building and testing with Docker

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

function print_usage() {
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  build        Build all Docker images"
    echo "  test         Run tests in Docker container"
    echo "  test-basic   Run only basic tests"
    echo "  run          Start the DTC server container"
    echo "  dev          Start development environment"
    echo "  clean        Clean up Docker images and containers"
    echo "  logs         Show logs from running DTC server"
    echo "  shell        Open shell in development container"
    echo ""
}

function build_images() {
    echo -e "${BLUE}Building Docker images...${NC}"
    docker build --target builder -t open-dtc-server:builder .
    docker build --target test -t open-dtc-server:test .
    docker build --target runtime -t open-dtc-server:runtime .
    echo -e "${GREEN}✅ Build completed${NC}"
}

function run_tests() {
    echo -e "${BLUE}Running tests in Docker container...${NC}"
    docker run --rm open-dtc-server:test
}

function run_basic_tests() {
    echo -e "${BLUE}Running basic tests only...${NC}"
    docker run --rm open-dtc-server:test ctest --build-config Release --test-dir build --tests-regex "BasicTest" --output-on-failure --verbose
}

function start_server() {
    echo -e "${BLUE}Starting DTC server...${NC}"
    docker run --rm --name dtc-server -p 11000:11000 -d open-dtc-server:runtime
    echo -e "${GREEN}✅ DTC server started on port 11000${NC}"
    echo "Use '$0 logs' to view logs, '$0 stop' to stop"
}

function start_dev() {
    echo -e "${BLUE}Starting development environment...${NC}"
    docker-compose up -d dev
    echo -e "${GREEN}✅ Development environment started${NC}"
    echo "Use '$0 shell' to access the development shell"
}

function stop_server() {
    echo -e "${BLUE}Stopping DTC server...${NC}"
    docker stop dtc-server 2>/dev/null || echo "Server not running"
    echo -e "${GREEN}✅ Server stopped${NC}"
}

function show_logs() {
    docker logs dtc-server
}

function open_shell() {
    docker-compose exec dev /bin/bash
}

function cleanup() {
    echo -e "${YELLOW}Cleaning up Docker images and containers...${NC}"
    docker stop dtc-server 2>/dev/null || true
    docker-compose down 2>/dev/null || true
    docker rmi open-dtc-server:builder open-dtc-server:test open-dtc-server:runtime 2>/dev/null || true
    docker system prune -f
    echo -e "${GREEN}✅ Cleanup completed${NC}"
}

# Main command handler
case "$1" in
    build)
        build_images
        ;;
    test)
        run_tests
        ;;
    test-basic)
        run_basic_tests
        ;;
    run)
        start_server
        ;;
    stop)
        stop_server
        ;;
    dev)
        start_dev
        ;;
    logs)
        show_logs
        ;;
    shell)
        open_shell
        ;;
    clean)
        cleanup
        ;;
    *)
        print_usage
        exit 1
        ;;
esac