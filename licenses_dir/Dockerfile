# Copyright (c) 2022 Intel Corporation.

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Dockerfile to maintain the source code of GPL/LGPL/AGPL/Eclipse license components used in UWC stack

FROM ubuntu:20.04 AS base

WORKDIR /thirdparty

ARG INSTALL_SOURCES="yes"

# Install pre-requisites for running wget, git clone commands.
RUN apt-get update && \
    apt-get install -y --no-install-recommends wget git ca-certificates

# Download source of zeroMQ lib
RUN mkdir -p src_libzmq && \
    cd src_libzmq && \
    wget -q --show-progress https://github.com/zeromq/libzmq/releases/download/v4.3.4/zeromq-4.3.4.tar.gz -O zeromq.tar.gz

# Download source of Paho.MQTT.C lib
RUN cd ${WORKDIR} && \
    mkdir -p src_pahomqtt_C && \
    cd src_pahomqtt_C && \
    git clone --branch develop https://github.com/eclipse/paho.mqtt.c.git

# Download source of Paho.MQTT.CPP lib
RUN cd ${WORKDIR} && \
    mkdir -p src_pahomqtt_CPP && \
    git clone --branch v1.1 https://github.com/eclipse/paho.mqtt.cpp.git && \
    cd .. && \
    wget https://github.com/eclipse/paho.mqtt.cpp/blob/f3a2a52259b5463ab50eee6d1c033cba18d6e84b/src/async_client.cpp?raw=true && \
    wget https://github.com/eclipse/paho.mqtt.cpp/blob/f3a2a52259b5463ab50eee6d1c033cba18d6e84b/src/mqtt/async_client.h?raw=true && \
    wget https://github.com/eclipse/paho.mqtt.cpp/blob/f3a2a52259b5463ab50eee6d1c033cba18d6e84b/src/async_client.cpp?raw=true

# Download source of log4cpp lib
RUN cd ${WORKDIR} && \
    mkdir -p src_log4cpp && \
    cd src_log4cpp && \
    wget https://sourceforge.net/projects/log4cpp/files/latest/download/log4cpp-1.1.3.tar.gz

# Download source of Tahu lib
RUN cd ${WORKDIR} && \
    mkdir -p src_tahu && \
    cd src_tahu && \
    git clone --branch develop https://github.com/eclipse/tahu

FROM ubuntu:20.04
LABEL Description="EII sources image"
LABEL Vendor="Intel Corporation"

WORKDIR /thirdparty

COPY --from=base /thirdparty /thirdparty

ENTRYPOINT ["/bin/bash"]