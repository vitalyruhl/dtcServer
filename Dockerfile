# Multi-stage build for coinbase-dtc-core
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

# Build the project (skip failing decode_jwt target)
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --parallel $(nproc) --target coinbase_dtc_server

# Runtime stage - smaller image
FROM ubuntu:22.04 AS runtime

# Install minimal runtime dependencies
RUN apt-get update && apt-get install -y \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy built executable from builder stage
COPY --from=builder /app/build/coinbase_dtc_server /usr/local/bin/coinbase_dtc_server
RUN chmod +x /usr/local/bin/coinbase_dtc_server

# Create non-root user for security
RUN useradd -r -s /bin/false coinbase

# Switch to non-root user
USER coinbase

# Expose port (adjust as needed for your application)
EXPOSE 8080

# Run the server
CMD ["/usr/local/bin/coinbase_dtc_server"]