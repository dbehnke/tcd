ARG BUILD_VERSION=dev

FROM debian:trixie AS base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libnng-dev \
    libcurl4-gnutls-dev \
    libboost-all-dev \
    nlohmann-json3-dev \
    libfmt-dev \
    libopus-dev \
    libogg-dev \
    unzip \
    python3 \
    golang-go \
    wget \
    curl \
    xxd \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

FROM base AS imbe-builder

WORKDIR /build/imbe_vocoder

COPY src/imbe_vocoder /build/imbe_vocoder

RUN make clean && make && (make install || true) \
    && cp libimbe_vocoder.a /usr/local/lib/ \
    && cp *.h /usr/local/include/

FROM base AS md380-builder

WORKDIR /build/md380_vocoder_dynarmic

COPY src/md380_vocoder_dynarmic /build/md380_vocoder_dynarmic

RUN rm -rf build && mkdir build

WORKDIR /build/md380_vocoder_dynarmic/build

RUN cmake .. && make && bash -x ../makelib.sh \
    && cp libmd380_vocoder.a /usr/local/lib/ \
    && cp ../md380_vocoder.h /usr/local/include/

FROM base AS tcd-builder

COPY --from=imbe-builder /usr/local/lib/libimbe_vocoder.a /usr/local/lib/
COPY --from=imbe-builder /usr/local/include/*.h /usr/local/include/
COPY --from=md380-builder /usr/local/lib/libmd380_vocoder.a /usr/local/lib/
COPY --from=md380-builder /usr/local/include/md380_vocoder.h /usr/local/include/

WORKDIR /build/tcd

COPY src/urfd /build/urfd
COPY src/tcd /build/tcd

WORKDIR /build/tcd

RUN echo "swambe2=true" > tcd.mk && \
    echo "swmodes=true" >> tcd.mk && \
    echo "debug=false" >> tcd.mk

RUN sed -i 's/-lmd380_vocoder/-lmd380_vocoder -lfmt/g' Makefile

RUN make clean && make swmodes=true BUILD_VERSION=${BUILD_VERSION}

FROM scratch AS output

COPY --from=tcd-builder /build/tcd/tcd /tcd
