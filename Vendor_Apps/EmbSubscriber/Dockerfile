# Copyright (c) 2021 Intel Corporation.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


# Dockerfile for emb_subscriber
ARG EII_VERSION
ARG DOCKER_REGISTRY
ARG UBUNTU_IMAGE_VERSION
ARG ARTIFACTS="/artifacts"
FROM ia_common:$EII_VERSION as common
FROM ia_eiibase:${EII_VERSION} as builder
LABEL description="Emb Subscriber image"

WORKDIR ${GOPATH}/src/IEdgeInsights
ARG CMAKE_INSTALL_PREFIX
ENV CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
COPY --from=common ${CMAKE_INSTALL_PREFIX}/include ${CMAKE_INSTALL_PREFIX}/include
COPY --from=common ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib
COPY --from=common /eii/common/util/util.go common/util/util.go
COPY --from=common ${GOPATH}/src ${GOPATH}/src
COPY --from=common /eii/common/libs/EIIMessageBus/go/EIIMessageBus $GOPATH/src/EIIMessageBus
COPY --from=common /eii/common/libs/ConfigMgr/go/ConfigMgr $GOPATH/src/ConfigMgr


COPY ./*.go ./
COPY go.mod ./
ENV PATH="$PATH:/usr/local/go/bin" \
    PKG_CONFIG_PATH="$PKG_CONFIG_PATH:${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" \
    LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${CMAKE_INSTALL_PREFIX}/lib"

# These flags are needed for enabling security while compiling and linking with cpuidcheck in golang
ENV CGO_CFLAGS="$CGO_FLAGS -I ${CMAKE_INSTALL_PREFIX}/include -O2 -D_FORTIFY_SOURCE=2 -Werror=format-security -fstack-protector-strong -fPIC" \
    CGO_LDFLAGS="$CGO_LDFLAGS -L${CMAKE_INSTALL_PREFIX}/lib -z noexecstack -z relro -z now"

ARG ARTIFACTS
RUN mkdir $ARTIFACTS && \
    GO111MODULE=on go build -o $ARTIFACTS/subscriber ./*.go

FROM ubuntu:$UBUNTU_IMAGE_VERSION as runtime

ARG ARTIFACTS
ARG EII_UID
ARG EII_USER_NAME
RUN groupadd $EII_USER_NAME -g $EII_UID && \
    useradd -r -u $EII_UID -g $EII_USER_NAME $EII_USER_NAME
RUN apt update && apt install --no-install-recommends -y libcjson1 libzmq5 zlib1g && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

ARG CMAKE_INSTALL_PREFIX
ENV CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
COPY --from=builder ${CMAKE_INSTALL_PREFIX}/lib .local/lib
COPY --from=builder $ARTIFACTS .

ENV LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:/app/.local/lib
HEALTHCHECK NONE
USER $EII_USER_NAME
ENTRYPOINT ["./subscriber"]
