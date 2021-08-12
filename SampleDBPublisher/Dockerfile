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


# Dockerfile for SampleDBPublisher
ARG EII_VERSION
ARG ARTIFACTS="/artifacts"
ARG EII_UID
ARG EII_USER_NAME
ARG UBUNTU_IMAGE_VERSION
FROM ia_eiibase:$EII_VERSION as base
FROM ia_common:$EII_VERSION as common

FROM base as builder
LABEL description="SampleDBPublisher image"

ARG ARTIFACTS
RUN mkdir $ARTIFACTS
WORKDIR /app

COPY ./*.py $ARTIFACTS

FROM ubuntu:$UBUNTU_IMAGE_VERSION as runtime

# Setting python dev env
RUN apt-get update && \
    apt-get install -y --no-install-recommends python3-distutils python3-minimal && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

ARG EII_UID
ARG EII_USER_NAME
RUN groupadd $EII_USER_NAME -g $EII_UID && \
    useradd -r -u $EII_UID -g $EII_USER_NAME $EII_USER_NAME

ARG ARTIFACTS
ARG CMAKE_INSTALL_PREFIX
ENV PYTHONPATH $PYTHONPATH:/app/.local/lib/python3.8/site-packages:/app
COPY --from=common ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib
COPY --from=common /eii/common/util/*.py util/
COPY --from=common /root/.local/lib .local/lib
COPY --from=builder $ARTIFACTS .
RUN chown -R ${EII_UID} .local/lib/python3.8
USER $EII_USER_NAME

ENV LD_LIBRARY_PATH $LD_LIBRARY_PATH:${CMAKE_INSTALL_PREFIX}/lib
ENV PATH $PATH:/app/.local/bin

HEALTHCHECK NONE

ENTRYPOINT ["python3", "-u", "main.py"]
