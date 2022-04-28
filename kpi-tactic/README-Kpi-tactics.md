```
* Copyright (c) 2021 Intel Corporation.

* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:

* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
```

Kpi-tactic containers sources details and build and run instructions

# Contents:

1. [Directory and File Details](#directory-and-file-details)

2. [Prerequisites Installation](#prerequisites-installation)

3. [Steps to Compile Kpi-tactic](#steps-to-compile-kpi-tactic)

4. [Steps to Run Kpi Executable on Machine](#steps-to-run-kpi-executable-on-machine)

6. [Steps to Deploy Sources Inside Containers](#steps-to-deploy-sources-inside-containers)

7. [Steps to Run Unit Test Cases](#steps-to-run-unit-test-cases)

9. [Pre-processor Flag to Enable and Disable the KPI-App on High Performance or Non-high Performance Processor](#pre-processor-flag-to-enable-and-disable-the-kpi-app-on-high-performance-or-non-high-performance-processor)

## Directory and File Details

The kpi-tactic directory consists of the following:

* KPIApp - This directory contains sources for the KPI App implementation. The following are the sub folders and files:
	- `.settings` - Eclipse project configuration files
	- `Build.test` - Unit test configuration to run unit test cases and generating code coverage
	- `Config` - Contains logger configuration directory
	- `Debug` - Build configuration for Debug mode
	- `include` - This directory contains all the header files (i.e., .hpp) required to compile kpi container
	- `lib` - This directory used to keep all the third party libraries (i.e. pthread , cjson, etc. ) required for kpi container
	- `Release` - Build configuration for Release mode. (This mode will be used for final deployment mode)
	- `src` - This directory contains all .cpp files.
	- `Test` - Contains unit test cases files. (.hpp, .cpp, etc.)
	- `.cproject` - Eclipse project configuration files
	- `.project` - Eclipse project configuration files
	- `sonar-project.properties` - This file is required for the Softdel CICD process for sonar qube analysis
* `Dockerfile` - Dockerfile to build Kpi-tactic container
* `Dockerfile_UT` - Dockerfile to build unit test container for Kpi-tactic sources testing
* docker-compose.yml - Ingredient docker-compose.yml for KPI-tactic micro service
* config.json - Ingredient config.json for KPI-tactic micro service

## Prerequisites Installation

Install the prerequisites for compiling Kpi-tactics sources on a machine without container:

1. Install make and cmake.
2. Install wget by using the command "sudo apt-get install wget".
3. Install Git by using the command "sudo apt install git".
4. Install all the EII libraries on host by running the shell script -- `sudo -E ./eii_libs_installer.sh`. For details, refer the README.md from  `IEdgeInsights\common\README.md`.
5. Install log4cpp (version - 1.1.3, link - https://sourceforge.net/projects/log4cpp/files/latest/download/log4cpp-1.1.3.tar.gz) library in the /usr/local/ directory.
6. Install yaml-cpp (branch - yaml-cpp-0.6.3, version - 0.6.3, link - https://github.com/jbeder/yaml-cpp.git) libraries on host in the /usr/local/ directory.
7. Install paho-cpp (branch develop https://github.com/eclipse/paho.mqtt.c.git) libraries on host in the /usr/local/ directory.
8. Install eclipse-tahu (branch develop https://github.com/eclipse/tahu) libraries on host in the /usr/local/ directory.
9. Install ssl (https://www.openssl.org/source/openssl-1.1.1g.tar.gz) libraries on host in the /usr/local/ directory.
10. Install uwc_common library refering to `README_UWC_Common.md` of `uwc_common` and copy `uwc_common/uwc_util/lib/libuwc-common.so` and 'uwc_common/uwc_util/include' (.hpp) in the `Sourcecode\kpi-tactic\KPIApp\lib` and `Sourcecode/kpi-tactic/KPIApp/include` directory respectively.

## Steps to Compile Kpi-tactic

1. Go to the `Sourcecode/kpi-tactic/KPIApp/Release` directory and open a terminal.
2. Run the `$make clean all` command.
3. After successful execution of step 2, application executable must be generated in the current directory (i.e. Release).

Notes : Above instructions are specified to build the sources in "Release" mode. To build sources in "Debug" mode, please execute the same steps in "Debug" folder inside `Sourcecode\kpi-tactic\` folder.

## Steps to Run kpi Executable on Machine


1. Deploy ia_etcd container with dev mode using following steps. 
	a. Run `preReq.sh` script as explained in the main uwc/README.md.
	b. Add `network_mode: host` option in two containers present in IEdgeInsights\build\provision\dep\docker-compose-provision.yml file.
	c. Run the provisioning command script to deploy the ia_etcd container as explained in the main uwc/README/md.
2. Go to the `Sourcecode/kpi-tactic/KPIApp/Release` directory and open bash terminal.
3. Set EII specific environment variables using the following command:
	`source <Complete Path of .env file present inside IEdgeInsights/build directory>`
	For example, `source /home/intel/IEdgeInsights/build/.env`
4. Export all environment variables required for Kpi container. Refer to the environment section from kpi-tactic's docker-compose.yml file (E.g., `export AppName="KPIAPP"` for exporting AppName variable likewise all other variables needed to be exported in the same terminal).
5. After successful compilation, run the application binary with the following command
	`./KPIApp-test`

## Steps to Deploy Sources Inside Containers

Refer to the Universal Wellpad Controller User Guide for information related to the container deployments.

## Steps to Run Unit Test Cases

1. To install the prerequisites for unit test execution, do the following:
    a. Install the latest version of "gcovr" tool for coverage report generation. Run the "pip install gcovr" command.
    b. Install gtest (wget https://github.com/google/googletest/archive/release-1.8.0.tar.gz) libraries on the host in the /usr/local/ directory.
    c. Install the other prerequisites mentioned in the section [Prerequisites Installation](#prerequisites-installation).
2. Follow same steps mentioned in section # Steps to compile Kpi-tactic , step 1 to step 3, but in "Build.test" directory of `Sourcecode/kpi-tactic/KPIApp` instead of the "release" directory.
3. Run unit test cases
   a. Deploy ia_etcd container with dev mode using following steps 
      i. Run the `01_pre-requisites.sh --isTLS=no  --brokerAddr="mqtt_test_container" --brokerPort="11883" --qos=1 --deployMode=IPC_DEV` script
      ii. Add `network_mode: host` option in two containers present in EdgeInsightsSoftware-v2.2-PV\IEdgeInsights\docker_setup\provision\dep\docker-compose-provision.yml file. 
      iii. Run `02_provision_UWC_.sh` script to deploy ia_etcd container.
   b. Go to `Sourcecode\kpi-tactic\KPIApp\Build.test` directory and open bash terminal.
   c. Export all environment variables required for KPIApp container. Refer environment section from kpi-tactic service present inside docker-compose_unit_test.yml file (E.g. `export AppName="KPIAPP"` for exporting AppName variable likewise all other variables needed to be exported in the same terminal) 
   d. After successful compilation, run the application binary with following command,
    `./KPIApp > kpi_tactics_status.log` 
   e. After successful execution of step 4, unit test log file `kpi_tactics_status.log` must be generated in the same folder that is Build.test
4. Generate unit test coverage report
    a. Go to `Sourcecode/kpi-tactic/KPIApp/Build.test` directory and open bash terminal.
    b. Run the command,
        `gcovr --html -e "../Test" -e "../include/ZmqHandler.hpp" -e "../include/Logger.hpp" -e "../include/EnvironmentVarHandler.hpp" -e ../../bin --exclude-throw-branches -o KPI_reoport.html -r .. .`
    c. After successful execution of step 2, unit test coverage report file `KPI_reoport.html` must be generated.

## Pre-processor Flag to Enable and Disable the KPI-App on High Performance or Non-high Performance Processor

1. By default, pre-processor flag UWC_HIGH_PERFORMANCE_PROCESSOR is disabled in Kpi App for debug & release mode.
2. To enable KPI-App on high performance processor in release mode, go to `Sourcecode\kpi-tactic\KPIApp\Release\src` directory and open subdir.mk file.
   Add the option "-DUWC_HIGH_PERFORMANCE_PROCESSOR" in below line where GCC compiler is invoked.
   `g++ -lrt -std=c++11 -I../$(PROJECT_DIR)/include -I/usr/local/include -I../$(PROJECT_DIR)/include/yaml-cpp -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<" .. .'`
3. To enable KPI-App on high performance processor in debug mode, go to `Sourcecode\kpi-tactic\KPIApp\Debug\src` directory and open subdir.mk file.
   Add the option "-DUWC_HIGH_PERFORMANCE_PROCESSOR" in below line where GCC compiler is invoked.
   `g++ -I../$(PROJECT_DIR)/include -I../$(PROJECT_DIR)/../uwc_common/uwc_util/include -I/usr/paho-c/include -I/usr/local/include -I../$(PROJECT_DIR)/include/yaml-cpp -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"`
4. To disable pre-processor flag in Kpi App, remove the option "-DUWC_HIGH_PERFORMANCE_PROCESSOR" added in steps 2 & 3 for both the Release and Debug mode.

**Note:**

* High performance processor are i3/i5/i7/i9 and low performance processor are Intel Atom® processors.
* Above steps are to run KPIApp unit test locally. To run unit test in container, follow the steps mentioned in section `## Steps to run unit test cases` of file `README.md` in Sourcecode directory. 

## Troubleshooting

If KPI-App is seen crashing, container restarting or AnalysisKPI logs not getting generated after building the Universal Wellpad Controller containers then to troubleshoot step run the 05_applyConfigChanges.sh, which would bring the containers down & up.

All GPL/LGPL/AGPL & Eclipse distribution license binary distributed components source code will be distributed via separate docker image ia_edgeinsights_uwc_src hosted in https://hub.docker.com/u/openedgeinsights. And the corresponding Docker file is present in "uwc/licenses/Dockerfile-sources/" folder.
