```
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
```

This document provides details for MQTT-Bridge containers sources details and build and run instructions.

# Contents

1. [Directory and file details](#directory-and-file-details)

2. [Prerequisites Installation](#prerequisites-installation)

3. [Steps to Compile MQTT-Bridge](#steps-to-compile-mqtt-bridge)

4. [Steps to run MQTT-Bridge executable on machine](#steps-to-run-mqtt-bridge-executable-on-machine)

5. [Steps to deploy sources inside container](#steps-to-deploy-sources-inside-container)

6. [Steps to run unit test cases](#steps-to-run-unit-test-cases)

7. [Steps to enable or disable instrumentation logs](#steps-to-enable-or-disable-instrumentation-logs)

# Directory and File Details

This section describes the directory contents and their uses.

1. MQTT-Bridge - This directory contains sources for MQTT-Bridge implementation 
	1. `.settings` - Eclipse project configuration files
	2. `Build.test` - Unit test configuration to run unit test cases and generating code coverage
	3. `Config` - Contains logger configuration directory
	4. `Debug` - Build configuration for Debug mode
	5. `include` - This directory contains all the header files (i.e. .hpp) required to compile kpi container
	6. `lib` - This directory used to keep all the third party libraries (i.e. pthread , cjson, etc. ) required for MQTT-Bridge container. 
	7. `Release` - Build configuration for Release mode. (This mode will be used for final deployment mode)
	8. `src` - This directory contains all .cpp files.
	9. `Test` - Contains unit test cases files. (.hpp, .cpp, etc.)
	10. `.cproject` - Eclipse project configuration files
	11. `.project` - Eclipse project configuration files
	12. `sonar-project.properties` - This file is required for Softdel CICD process for sonar qube analysis
2. `Dockerfile` - Dockerfile to build MQTT-Bridge container.
3. `Dockerfile_UT` - Dockerfile to build unit test container for MQTT-Bridge sources testing.
4. docker-compose.yml -- Ingredient docker-compose.yml for mqtt-bridge micro service.
5. config.json - Ingredient config.json for mqtt-bridge micro service.

# Prerequisites Installation

For compiling MQTT-Bridge sources on machine without container, following pre-requisites needs to installed on machine,
1. Install make and cmake
2. Install wget by using command "sudo apt-get install wget".
3. Install Git by using command "sudo apt install git"
4. Install all the EII libraries on host by running the shell script -- `sudo -E ./eiI_libs_installer.sh`. Refre the README.md from  `IEdgeInsights\common\README.md` for details.
5. Install log4cpp (version - 1.1.3, link - https://sourceforge.net/projects/log4cpp/files/latest/download/log4cpp-1.1.3.tar.gz) library under /usr/local/ directory.
6. Install yaml-cpp (branch - yaml-cpp-0.6.3, version - 0.6.3, link - https://github.com/jbeder/yaml-cpp.git) libraries on host under /usr/local/ directory.
7. Install paho-cpp (branch develop https://github.com/eclipse/paho.mqtt.c.git) libraries on host under /usr/local/ directory.
8. Install eclipse-tahu (branch develop https://github.com/eclipse/tahu) libraries on host under /usr/local/ directory.
9. Install ssl (https://www.openssl.org/source/openssl-1.1.1g.tar.gz) libraries on host under /usr/local/ directory.
10. Install uwc_common library refering to `README_UWC_Common.md` of `uwc_common` and copy `uwc_common/uwc_util/lib/libuwc-common.so` and 'uwc_common/uwc_util/include' (.hpp) in `Sourcecode\mqtt-bridge\MQTT-Bridge\lib` and `Sourcecode\mqtt-bridge\MQTT-Bridge\include` directory respectively.

# Steps to Compile MQTT-Bridge

1. Go to `Sourcecode\mqtt-bridge\MQTT-Bridge\Release` directory and open a terminal.
2. Execute `$make clean all` command.
3. After successful execution of step 2, application executable (with name `MQTT_Bridge`) must be generated in current directory (i.e. Release).

**Notes:** Above instructions are specified to build the sources in "Release" mode. To build sources in "Debug" mode, please execute the same steps in "Debug" folder inside `Sourcecode\mqtt-bridge\` folder.

# Steps to Run MQTT-Bridge Executable on Machine
1. Deploy ia_etcd container with dev mode using following steps. 
	a. Run `01_uwc_pre_requisites.sh` script as explained in the main uwc/README.md.
	b. Run the provisioning command script to deploy ia_etcd container as explainedin main uwc/README.md.
2. Go to `Sourcecode\mqtt-bridge\MQTT-Bridge\Release` directory and open bash terminal.
3. Set EII specific environment variables using below command.
	`source <Complete Path of .env file present inside IEdgeInsights/build directory>`
	For example, `source /home/intel/uwc-releases/IEdgeInsights/build/.env`
4. Export all environment variables required for mqtt-bridge-test container. Refer to the environment section from mqtt-bridge-test service present inside docker-compose.yml of mqtt-bridge service file (E.g. `export AppName="MQTT-Bridge"` for exporting AppName variable likewise all other variables needed to be exported in the same terminal). 
5. After successful compilation, run the application binary with following command,
	`./MQTT_Bridge`

# Steps to Deploy Sources Inside Container

Refer to the Universal Wellpad Controller user guide for container deployments.

# Steps to Run Unit Test Cases
1. Pre-requisites Installation for unit test execution
    1. Install latest version of "gcovr" tool for coverage report generation by using command "pip install gcovr".
    2. Install gtest (wget https://github.com/google/googletest/archive/release-1.8.0.tar.gz) libraries on host under /usr/local/ directory.
    3. All other prerequisites to be installed mentioned in section # Prerequisites Installation
2. Follow same steps mentioned in section # Steps to compile MQTT-Bridge, step 1 to step 3, but in "Build.test" directory of `Sourcecode\mqtt-bridge\MQTT-Bridge` instead of "release" directory.
3. Run unit test cases
    1. Go to `Sourcecode\mqtt-bridge\MQTT-Bridge\Build.test` directory and open bash terminal.
    2. Export all environment variables required for mqtt-bridge-test container. Refer environment section from mqtt-bridge-test service present inside docker-compose_unit_test.yml file (E.g. `export AppName="MQTT-Bridge"` for exporting AppName variable likewise all other variables needed to be exported in the same terminal) 
    3. After successful compilation, run the application binary with following command,
    `./MQTT_Bridge_test > mqtt-bridge_test_status.log` 
    4. After successful execution of step 4, unit test log file `mqtt-bridge_test_status.log` must be generated in the same folder that is Build.test
4. Generate unit test coverage report
    1. Go to `Sourcecode\mqtt-bridge\MQTT-Bridge\Build.test` directory and open bash terminal.
    2. Run the command,
        `gcovr --html -f "../src/Common.cpp" -f "../src/Main.cpp" -f "../src/MQTTPublishHandler.cpp" -f "../src/MQTTSubscribeHandler.cpp" -f "../src/QueueMgr.cpp" -f "../include/Common.hpp" -f "../include/MQTTPublishHandler.hpp" -f "../include/MQTTSubscribeHandler.hpp" -f "../include/QueueMgr.hpp" --exclude-throw-branches -o MQTT-Bridge_Report.html -r .. .`
    3. After successful execution of step 2, unit test coverage report file `MQTT-Bridge_Report.html` must be generated.


**Notes:** Above steps are to run KPIApp unit test locally. In order to run unit test in container, please follow the steps mentioned in section `## Steps to run unit test cases` of file `README.md` in Sourcecode directory. 

## Steps to Enable or Disable Instrumentation Logs

By default, the instrumentation logs are enabled for debug mode & disabled for release mode. 

1. Go to `Sourcecode\mqtt-bridge\MQTT-Bridge\Release\src` directory and open the subdir.mk file. Do one of the following"
	* To enable the instrumentation logs, go to g++ command at line number 39 & add the option "-DINSTRUMENTATION_LOG".
	* To disable the instrumentation logs, go to g++ command at line number 39 check & remove the option "-DINSTRUMENTATION_LOG" if found.

All GPL/LGPL/AGPL & Eclipse distribution license binary distributed components source code will be distributed via separate docker image ia_edgeinsights_uwc_src hosted in https://hub.docker.com/u/openedgeinsights. And the corresponding Docker file is present in "uwc/licenses/Dockerfile-sources/" folder.

