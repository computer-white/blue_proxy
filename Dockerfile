FROM ubuntu:22.04

RUN apt update && apt install -y \
    build-essential g++ cmake \
    libboost-all-dev libssl-dev zlib1g-dev \
    libmysqlclient-dev libhiredis-dev \
    libgumbo-dev libidn2-dev ragel \
    libyaml-cpp-dev pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN rm -rf build && mkdir build && cd build && cmake .. && make -j$(nproc)

RUN mkdir -p certs && \
    openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout certs/server.key -out certs/server.crt \
    -days 365 -subj "/CN=localhost"

EXPOSE 8020 8443
CMD ["./bin/test_proxy"]
