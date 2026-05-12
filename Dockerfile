FROM ubuntu:24.04 AS build

RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    pkg-config \
    libmicrohttpd-dev \
    python3 \
    python3-numpy \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Pre-process the dataset into quantized binary format
RUN python3 scripts/preprocess_references.py

# Build the C application
RUN cmake -B build && cmake --build build

FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    libmicrohttpd12 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /app/build/rinha_de_backend /app/rinha_de_backend
COPY --from=build /app/resources/references.bin /app/resources/references.bin

EXPOSE 9999

WORKDIR /app
CMD ["/app/rinha_de_backend"]
