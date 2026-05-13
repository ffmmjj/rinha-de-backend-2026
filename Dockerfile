FROM ubuntu:24.04 AS build

RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    pkg-config \
    libssl-dev \
    python3 \
    python3-numpy \
    && rm -rf /var/lib/apt/lists/*

# Build h2o from source (no apt package available)
RUN apt-get install -y \
    git \
    && rm -rf /var/lib/apt/lists/* \
    && git clone --depth 1 https://github.com/h2o/h2o.git /tmp/h2o \
    && cd /tmp/h2o \
    && cmake -B build \
       -DCMAKE_BUILD_TYPE=Release \
       -DWITH_BUNDLED_SSL=OFF \
       -DWITH_LIBUV=OFF \
       -DWITH_MRUBY=OFF \
    && cmake --build build -j$(nproc) \
    && cmake --install build --prefix /usr/local

WORKDIR /app
COPY . .

# Pre-process the dataset into quantized binary format
RUN python3 scripts/preprocess_references.py

# Build the C application
RUN PKG_CONFIG_PATH="/usr/local/lib/pkgconfig" cmake -B build && cmake --build build

FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /usr/local/lib/libh2o-evloop.so* /usr/local/lib/
COPY --from=build /app/build/rinha_de_backend /app/rinha_de_backend
COPY --from=build /app/resources/references.bin /app/resources/references.bin

ENV LD_LIBRARY_PATH=/usr/local/lib

EXPOSE 9999

WORKDIR /app
CMD ["/app/rinha_de_backend"]
