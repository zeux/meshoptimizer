#====================================================================
#
# Build
# $ docker build -t build-meshoptimizer -f ./Dockerfile .
#
# Run
# $ docker run -it -v ~/:/mnt/home build-meshoptimizer
#
# Cleanup
# $ docker rmi build-meshoptimizer
#
# Delete all unused images
# $ docker system prune -f
#
#====================================================================

FROM debian:11 AS meshoptimizer

# Set timezone
ARG TZ="Europe/Berlin"
ENV TZ $TZ
RUN echo ${TZ} > /etc/timezone

# Automated
ENV DEBIAN_FRONTEND=noninteractive


#--------------------------------------------------------------------
# Install Build Dependencies

RUN \
       apt-get -yq update \
    && apt-get -yq install build-essential cmake g++ wget \
    && mkdir -p /code /app /mnt/home


# WASI SDK
ENV WASIVER 19.0
ENV WASISDK /code/wasi-sdk-${WASIVER}
RUN \
       cd /code \
    && WASIMAJ=${WASIVER%.*} \
    && WASILNK=https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASIMAJ}/wasi-sdk-${WASIVER}-linux.tar.gz \
    && wget ${WASILNK} -O wasi-sdk-${WASIVER}-linux.tar.gz \
    && tar -xf wasi-sdk-${WASIVER}-linux.tar.gz


#--------------------------------------------------------------------
# Copy local code

ADD . /code/meshoptimizer


#--------------------------------------------------------------------
# Build and install gltfpack

RUN \
       cd /code/meshoptimizer \
    && cmake . -B ./bld -DMESHOPT_BUILD_GLTFPACK=ON \
    && cmake --build ./bld -j8 \
    && cmake --install ./bld \
    && cp ./bld/gltfpack /app \
    && echo "rm -Rf ./bld && cmake . -B ./bld -DMESHOPT_BUILD_GLTFPACK=ON && cmake --build ./bld -j8" >> /root/.bash_history


#--------------------------------------------------------------------
# Build gltfpack web assembly (WASM)

RUN \
       cd /code/meshoptimizer \
    && make WASI_SDK=${WASISDK} config=release gltfpack.wasm \
    && cp ./gltf/library.wasm /app/gltfpack.wasm \
    && cp ./gltf/library.js /app/gltfpack.js \
    && sed -i 's/library\.wasm/gltfpack\.wasm/g' /app/gltfpack.js \
    && echo "make WASI_SDK=${WASISDK} config=release gltfpack.wasm" >> /root/.bash_history


#--------------------------------------------------------------------
# Setup to run

RUN \
       cd /app \
    && echo "cp -R /app/* /mnt/home" >> /root/.bash_history \
    && echo "gltfpack -si 0.1 -v -i " >> /root/.bash_history

WORKDIR /app

CMD /bin/bash
