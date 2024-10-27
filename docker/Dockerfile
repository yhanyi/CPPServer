FROM debian:bullseye AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    curl \
    cmake \
    git \
    pkg-config \
    libpq-dev \
    libpqxx-dev \
    postgresql-client \
    libcurl4-openssl-dev \
    nlohmann-json3-dev \
    libssl-dev \
    wget \
    libgtest-dev \
    zlib1g-dev

# Install prometheus-cpp from source
WORKDIR /tmp
RUN git clone --recursive https://github.com/jupp0r/prometheus-cpp.git && \
    cd prometheus-cpp && \
    git submodule init && \
    git submodule update && \
    mkdir build && \
    cd build && \
    cmake .. -DBUILD_SHARED_LIBS=ON && \
    cmake --build . --parallel 4 && \
    cmake --install .

# Build our application
WORKDIR /app
COPY . .

# Update library paths
RUN ldconfig

# Set environment for C++17
ENV CXXFLAGS="-std=c++17"

# Build the application with verbose output
RUN make clean && \
    make -B V=1 all && \
    ls -la server server_tests cache_tests

# Runtime image
FROM debian:bullseye-slim

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libpqxx-6.4 \
    postgresql-client \
    libcurl4 \
    libssl1.1 \
    zlib1g \
    && rm -rf /var/lib/apt/lists/*

# Copy prometheus-cpp libraries from builder
COPY --from=builder /usr/local/lib/libprometheus-cpp*.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/libcivetweb*.so* /usr/local/lib/

# Copy built application and config files
COPY --from=builder /app/server /usr/local/bin/
COPY prometheus.yml /etc/prometheus/

# Update library cache
RUN ldconfig

EXPOSE 8080 9091
CMD ["server"]
