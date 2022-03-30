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
ARG CMAKE_INSTALL_PREFIX
FROM uwc_common:$UWC_VERSION as uwc_common
LABEL description="KPI Tactic Application"

# Build KPI Tactic application
FROM ia_common:$EII_VERSION as kpitactic
WORKDIR /
# Install libzmq
RUN rm -rf deps && \
    mkdir -p deps && \
    cd deps && \
    wget -q --show-progress https://github.com/zeromq/libzmq/releases/download/v4.3.4/zeromq-4.3.4.tar.gz -O zeromq.tar.gz && \
    tar xf zeromq.tar.gz && \
    cd zeromq-4.3.4 && \
    ./configure --prefix=${CMAKE_INSTALL_PREFIX} && \
    make install

# Install cjson
RUN rm -rf deps && \
    mkdir -p deps && \
    cd deps && \
    wget -q --show-progress https://github.com/DaveGamble/cJSON/archive/v1.7.12.tar.gz -O cjson.tar.gz && \
    tar xf cjson.tar.gz && \
    cd cJSON-1.7.12 && \
    mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_PREFIX}/include -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} .. && \
    make install

RUN rm -rf /var/lib/apt/lists/*
# copy required dependencies
COPY KPIApp /KPIApp/. 
COPY --from=uwc_common /usr/yaml/include/yaml-cpp /usr/local/include/yaml-cpp
COPY --from=uwc_common /usr/yaml/lib/* /usr/local/lib/
COPY --from=uwc_common /usr/paho-cpp/lib/lib* /KPIApp/lib/
COPY --from=uwc_common /usr/lib/libpaho* /KPIApp/lib/
COPY --from=uwc_common /usr/ssl/lib/* /usr/local/lib/
COPY --from=uwc_common /usr/ssl/include/* /usr/local/include/
COPY --from=uwc_common /usr/paho-cpp/include/mqtt /usr/local/include/mqtt
COPY --from=uwc_common /usr/include/MQTT* /KPIApp/include/
COPY --from=uwc_common /EII/log4cpp/build/include/log4cpp /KPIApp/include/log4cpp
COPY --from=uwc_common /EII/log4cpp/build/lib/* /KPIApp/lib/
COPY --from=uwc_common /uwc_util/Release/libuwc-common.so /KPIApp/lib/
COPY --from=uwc_common /uwc_util/include/* /KPIApp/include/
COPY --from=uwc_common $CMAKE_INSTALL_PREFIX/include /usr/local/include
COPY --from=uwc_common $CMAKE_INSTALL_PREFIX/lib /usr/local/lib
COPY --from=uwc_common ${CMAKE_INSTALL_PREFIX}/include ${CMAKE_INSTALL_PREFIX}/include
COPY --from=uwc_common ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib
# build sources
RUN cd /KPIApp/Release \
    &&	export LD_LIBRARY_PATH='/KPIApp/lib:/usr/local/lib' \
    &&	make clean all -s

# Build KPI App container service
FROM ubuntu:$UBUNTU_IMAGE_VERSION as runtime
WORKDIR /

# creating base directories
ENV APP_WORK_DIR /opt/intel/app/
RUN mkdir -p /opt/intel/app \
	&& mkdir -p /opt/intel/app/logs \
	&& mkdir -p /opt/intel/config \
	&& mkdir -p /opt/intel/eii/uwc_data
WORKDIR ${APP_WORK_DIR}
ARG CMAKE_INSTALL_PREFIX
# copy required dependencies
COPY --from=kpitactic ${CMAKE_INSTALL_PREFIX}/lib  ${APP_WORK_DIR}
COPY --from=kpitactic ${CMAKE_INSTALL_PREFIX}/include ${APP_WORK_DIR}
COPY --from=kpitactic /KPIApp/Release/KPIApp ${APP_WORK_DIR}
COPY --from=uwc_common /usr/yaml/lib/*.so.* ${APP_WORK_DIR}
COPY --from=uwc_common /usr/lib/libpaho* ${APP_WORK_DIR}
COPY --from=uwc_common /usr/ssl/lib/* /usr/local/lib/
COPY --from=uwc_common /usr/ssl/include/* /usr/local/include/
COPY --from=uwc_common /usr/paho-cpp/lib/lib* ${APP_WORK_DIR}
COPY --from=uwc_common /EII/log4cpp/build/lib/* ${APP_WORK_DIR}
COPY --from=kpitactic /KPIApp/Config/log4cpp.properties /opt/intel/config
COPY --from=uwc_common /uwc_util/Release/libuwc-common.so ${APP_WORK_DIR}
COPY --from=uwc_common $CMAKE_INSTALL_PREFIX/lib/*  ${APP_WORK_DIR}
# set permissions to working dir for eiiuser
RUN chown -R ${EII_UID}:${EII_UID} /opt/intel/config \
   && chown -R ${EII_UID}:${EII_UID} ${APP_WORK_DIR} \
   && chmod -R 770 ${APP_WORK_DIR}

#set the environment variable
ENV LD_LIBRARY_PATH ${APP_WORK_DIR}:/usr/local/lib
HEALTHCHECK CMD ["/bin/sh","-c", "ps -C KPIAPP >/dev/null && echo Running || echo Not running; exit 1"]
# Run the container
CMD ./KPIApp
