FROM ubuntu:24.04 AS build

RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    pkg-config \
    libmicrohttpd-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -B build && cmake --build build

FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    libmicrohttpd12 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /app/build/rinha_de_backend /app/rinha_de_backend

EXPOSE 8081

CMD ["/app/rinha_de_backend"]
