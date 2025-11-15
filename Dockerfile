# Multi-stage build for open-dtc-server (formerly coinbase-dtc-core)
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    libcurl4-openssl-dev \
    libssl-dev \
    pkg-config \
    ca-certificates \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Install jwt-cpp from source since it's not available in Ubuntu 22.04 packages
RUN git clone --depth 1 --branch v0.7.1 https://github.com/Thalhammer/jwt-cpp.git /tmp/jwt-cpp && \
    cd /tmp/jwt-cpp && \
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DJWT_BUILD_EXAMPLES=OFF -DJWT_BUILD_TESTS=OFF && \
    cmake --build build --parallel $(nproc) && \
    cmake --install build && \
    rm -rf /tmp/jwt-cpp

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build the project without tests for Docker
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=OFF
RUN cmake --build build --parallel $(nproc)

# Runtime stage - smaller image
FROM ubuntu:22.04 AS runtime

# Install minimal runtime dependencies
RUN apt-get update && apt-get install -y \
    ca-certificates \
    libcurl4 \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

# Copy built executable from builder stage
COPY --from=builder /app/build/coinbase_dtc_server /usr/local/bin/coinbase_dtc_server
RUN chmod +x /usr/local/bin/coinbase_dtc_server

# Verify the executable exists
RUN ls -la /usr/local/bin/coinbase_dtc_server

# Create non-root user for security
RUN useradd -r -s /bin/false dtcserver

# Switch to non-root user
USER dtcserver

# Expose port for DTC server
EXPOSE 11099

# Run the server
CMD ["/usr/local/bin/coinbase_dtc_server"]

# Test stage - for running tests in CI/CD
FROM builder AS test

# Install additional test dependencies
RUN apt-get update && apt-get install -y \
    valgrind \
    gdb \
    && rm -rf /var/lib/apt/lists/*

# Set working directory back to build
WORKDIR /app

# Default command for test stage is to run tests
CMD ["ctest", "--build-config", "Release", "--test-dir", "build", "--output-on-failure", "--verbose"]