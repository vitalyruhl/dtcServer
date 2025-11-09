# Multi-stage build for coinbase-dtc-core
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build the project
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --parallel $(nproc)

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