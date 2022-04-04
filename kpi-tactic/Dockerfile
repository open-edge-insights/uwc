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
LABEL description="KPI Tactic Application"

# Build KPI Tactic application
FROM ia_common:$EII_VERSION as kpitactic
WORKDIR /

##ospdt st
RUN apt-get update && apt-get install -y git
RUN git clone https://github.com/daald/dpkg-licenses.git
RUN cd dpkg-licenses && ./dpkg-licenses > /before-kpi-1.txt
##ospdt end

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

# build sources
RUN cd /KPIApp/Release \
    &&	export LD_LIBRARY_PATH='/KPIApp/lib:/usr/local/lib' \
    &&	make clean all -s

RUN apt-get install -y vim

##ospdt st
RUN cd dpkg-licenses && ./dpkg-licenses > /after-kpi-1.txt
RUN grep -xvFf /before-kpi-1.txt /after-kpi-1.txt > /apt_licenses_kpi-1.txt
##ospdt end

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

##ospdt st
RUN apt-get update && apt-get install -y git
RUN git clone https://github.com/daald/dpkg-licenses.git
RUN cd dpkg-licenses && ./dpkg-licenses > ${APP_WORK_DIR}/before-kpi-2.txt
##ospdt end


RUN apt update && apt install --no-install-recommends -y libcjson1 libzmq5 zlib1g && \
    rm -rf /var/lib/apt/lists/*
# copy required dependencies
COPY --from=kpitactic /KPIApp/Release/KPIApp ${APP_WORK_DIR}
COPY --from=uwc_common /usr/yaml/lib/*.so.* ${APP_WORK_DIR}
COPY --from=uwc_common /usr/lib/libpaho* ${APP_WORK_DIR}
COPY --from=uwc_common /usr/ssl/lib/* /usr/local/lib/
COPY --from=uwc_common /usr/ssl/include/* /usr/local/include/
COPY --from=uwc_common /usr/paho-cpp/lib/lib* ${APP_WORK_DIR}
COPY --from=uwc_common /EII/log4cpp/build/lib/* ${APP_WORK_DIR}
COPY --from=kpitactic /KPIApp/Config/log4cpp.properties /opt/intel/config
COPY --from=uwc_common /uwc_util/Release/libuwc-common.so ${APP_WORK_DIR}
ARG CMAKE_INSTALL_PREFIX
COPY --from=uwc_common $CMAKE_INSTALL_PREFIX/lib/*  ${APP_WORK_DIR}
# set permissions to working dir for eiiuser
RUN chown -R ${EII_UID}:${EII_UID} /opt/intel/config \
   && chown -R ${EII_UID}:${EII_UID} ${APP_WORK_DIR} \
   && chmod -R 770 ${APP_WORK_DIR}

##ospdt st
RUN cd dpkg-licenses && ./dpkg-licenses > ${APP_WORK_DIR}/after-kpi-2.txt
RUN grep -xvFf ${APP_WORK_DIR}/before-kpi-2.txt ${APP_WORK_DIR}/after-kpi-2.txt > ${APP_WORK_DIR}/apt_licenses_kpi-2.txt
##ospdt end


#set the environment variable
ENV LD_LIBRARY_PATH ${APP_WORK_DIR}:/usr/local/lib

# Run the container
CMD ./KPIApp
