# Copyright (c) 2021 Intel Corporation.

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

ARG EII_VERSION
ARG UBUNTU_IMAGE_VERSION
ARG UWC_VERSION
FROM uwc_common:$UWC_VERSION as uwc_common
LABEL description="Modbus TCP Master Application"

# Build Modbus Stack
FROM ia_common:$EII_VERSION as modbus-lib
WORKDIR /

# compile modbus stack
COPY modconn /modconn/.
COPY --from=uwc_common ${CMAKE_INSTALL_PREFIX}/include /usr/local/include
COPY --from=uwc_common ${CMAKE_INSTALL_PREFIX}/lib /usr/local/lib
WORKDIR modconn
RUN cd Release \
    && make clean all -s

# Build Modbus application
FROM ia_common:$EII_VERSION as modbus-app
WORKDIR /

# Copy built library from images
COPY --from=uwc_common /usr/yaml /bin/yaml-cpp/.

COPY Modbus-App /Modbus-App/. 
COPY --from=modbus-lib /modconn/Release/libModbusMasterStack.so /Modbus-App/lib/
COPY --from=modbus-lib /modconn/Inc/API.h /Modbus-App/include/
COPY --from=uwc_common /EII/log4cpp/build/include/log4cpp /Modbus-App/include/log4cpp
COPY --from=uwc_common /EII/log4cpp/build/lib/* /Modbus-App/lib/
COPY --from=uwc_common /uwc_util/Release/libuwc-common.so /Modbus-App/lib/
COPY --from=uwc_common /uwc_util/include/* /Modbus-App/include/
COPY --from=uwc_common /usr/paho-cpp/lib/lib* /Modbus-App/lib/
COPY --from=uwc_common /usr/lib/libpaho* /Modbus-App/lib/
COPY --from=uwc_common /usr/paho-cpp/include/mqtt /usr/local/include/mqtt
COPY --from=uwc_common ${CMAKE_INSTALL_PREFIX}/include /usr/local/include
COPY --from=uwc_common ${CMAKE_INSTALL_PREFIX}/lib /usr/local/lib
# compile modbus app sources
RUN cd /Modbus-App/Release \
    &&	export LD_LIBRARY_PATH='/Modbus-App/lib:/usr/local/lib:/bin/yaml-cpp/lib' \
    &&  export INCLUDE_DIR='/bin' \
    &&	make clean all -s

# Build Modbus container service
FROM ubuntu:$UBUNTU_IMAGE_VERSION as runtime
WORKDIR /

# creating base directories
ENV APP_WORK_DIR /opt/intel/app/
RUN mkdir -p /opt/intel/app \
	&& mkdir -p /opt/intel/app/logs \
	&& mkdir -p /opt/intel/config \
	&& mkdir -p /opt/intel/eii/uwc_data

RUN apt update && apt install --no-install-recommends -y libcjson1 libzmq5 zlib1g && \
    rm -rf /var/lib/apt/lists/*

WORKDIR ${APP_WORK_DIR}

# copy required libs and includes
ARG CMAKE_INSTALL_PREFIX
COPY --from=modbus-lib /modconn/Release/libModbusMasterStack.so ${APP_WORK_DIR}
COPY --from=modbus-app /Modbus-App/Release/ModbusMaster ${APP_WORK_DIR}
COPY --from=uwc_common /usr/yaml/lib/*.so.* ${APP_WORK_DIR}
COPY --from=uwc_common /EII/log4cpp/build/lib/* ${APP_WORK_DIR}
COPY --from=modbus-app /Modbus-App/Config/log4cpp.properties /opt/intel/config
COPY --from=uwc_common /uwc_util/Release/libuwc-common.so ${APP_WORK_DIR}
COPY --from=uwc_common /usr/lib/libpaho* ${APP_WORK_DIR}
COPY --from=uwc_common /usr/paho-cpp/lib/lib* ${APP_WORK_DIR}
COPY --from=uwc_common $CMAKE_INSTALL_PREFIX/lib/* ${APP_WORK_DIR}
# set permissions to working dir for eiiuser
RUN chown -R ${EII_UID}:${EII_UID} /opt/intel/config \
   && chown -R ${EII_UID}:${EII_UID} ${APP_WORK_DIR} \
   && chmod -R 770 ${APP_WORK_DIR}

#set the environment variable
ENV LD_LIBRARY_PATH ${APP_WORK_DIR}:/usr/local/lib

# Run Modbus container
CMD ./ModbusMaster
