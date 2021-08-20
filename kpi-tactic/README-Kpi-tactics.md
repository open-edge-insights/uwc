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

Kpi-tactic containers sources details and build and Run instructions

# Contents:

1. [Directory and file details](#All-internal-directory-file-details)

2. [Compiling Kpi-tactic sources on host machine](#Compiling-sources)

3. [Pre-requisites](#Pre-requisites-Installation)

4. [Steps to compile Kpi-tactic](#Steps-to-compile-Kpi-tactic)

5. [Steps to compile Kpi-tactic](#Steps-to-compile-Kpi-tactic)

6. [Steps to run Kpi-tactic executable on machine](#Steps-to-run-Kpi-tactic-executable-on-machine)

7. [Steps to deploy sources inside container](#Steps-to-deploy-sources-inside-container)

8. [Steps to run unit test cases](#Steps-to-run-unit-testcases)

9. [Steps to enable/disable Kpi App for high performance processor](#Steps-to-enable/disable-Kpi-App-for-high-performance-processor(i7/i10))


# Directory and file details
Section to describe all directory contents and it's uses.

1. KPIApp - This directory contains sources for KPI App implementation 
	1. `.settings` - Eclipse project configuration files
	2. `Build.test` - Unit test configuration to run unit test cases and generating code coverage
	3. `Config` - Contains logger configuration directory
	4. `Debug` - Build configuration for Debug mode
	5. `include` - This directory contains all the header files (i.e. .hpp) required to compile kpi container
	6. `lib` - This directory used to keep all the third party libraries (i.e. pthread , cjson, etc. ) required for kpi container. 
	7. `Release` - Build configuration for Release mode. (This mode will be used for final deployment mode)
	8. `src` - This directory contains all .cpp files.
	9. `Test` - Contains unit test cases files. (.hpp, .cpp, etc.)
	10. `.cproject` - Eclipse project configuration files
	11. `.project` - Eclipse project configuration files
	12. `sonar-project.properties` - This file is required for Softdel CICD process for sonar qube analysis
2. `Dockerfile` - Dockerfile to build Kpi-tactic container.
3. `Dockerfile_UT` - Dockerfile to build unit test container for Kpi-tactic sources testing.
4. docker-compose.yml -- Ingredient docker-compose.yml for KPI-tactic micro service.
5. config.json - Ingredient config.json for KPI-tactic micro service.

# Pre-requisites Installation

For compiling Kpi-tactics sources on machine without container, following pre-requisites needs to installed on machine,
1. Install make and cmake
2. Install wget by using command "sudo apt-get install wget".
3. Install Git by using command "sudo apt install git"
4. Install all the EII libraries on host by running the shell script -- `sudo -E ./eii_libs_installer.sh`. Refre the README.md from  `IEdgeInsights\common\README.md` for details.
5. Install log4cpp (version - 1.1.3, link - https://sourceforge.net/projects/log4cpp/files/latest/download/log4cpp-1.1.3.tar.gz) library under /usr/local/ directory.
6. Install yaml-cpp (branch - yaml-cpp-0.6.3, version - 0.6.3, link - https://github.com/jbeder/yaml-cpp.git) libraries on host under /usr/local/ directory.
7. Install paho-cpp (branch develop https://github.com/eclipse/paho.mqtt.c.git) libraries on host under /usr/local/ directory.
8. Install eclipse-tahu (branch develop https://github.com/eclipse/tahu) libraries on host under /usr/local/ directory.
9. Install ssl (https://www.openssl.org/source/openssl-1.1.1g.tar.gz) libraries on host under /usr/local/ directory.
10. Install uwc_common library refering to `README_UWC_Common.md` of `uwc_common` and copy `uwc_common/uwc_util/lib/libuwc-common.so` and 'uwc_common/uwc_util/include' (.hpp) in `Sourcecode\kpi-tactic\KPIApp\lib` and `Sourcecode/kpi-tactic/KPIApp/include` directory respectively.

# Steps to compile Kpi-tactic

1. Go to `Sourcecode/kpi-tactic/KPIApp/Release` directory and open a terminal.
2. Execute `$make clean all` command.
3. After successful execution of step 2, application executable must be generated in current directory (i.e. Release).

Notes : Above instructions are specified to build the sources in "Release" mode. To build sources in "Debug" mode, please execute the same steps in "Debug" folder inside `Sourcecode\kpi-tactic\` folder.

# Steps to run kpi executable on machine
1. Deploy ia_etcd container with dev mode using following steps. 
	1. Run `preReq.sh` script as explained in the main uwc/README.md.
	2. Add `network_mode: host` option in two containers present in IEdgeInsights\build\provision\dep\docker-compose-provision.yml file.
	3. Run th eprovisioning command script to deploy ia_etcd container as explainedin main uwc/README/md.
2. Go to `Sourcecode/kpi-tactic/KPIApp/Release` directory and open bash terminal.
3. Set EII specific environment variables using below command.
	`source <Complete Path of .env file present inside IEdgeInsights/build directory>`
	For example `source /home/intel/IEdgeInsights/build/.env`
4. Export all environment variables required for Kpi container. Refer environment section from kpi-tactic's docker-compose.yml file (E.g. `export AppName="KPIAPP"` for exporting AppName variable likewise all other variables needed to be exported in the same terminal).
5. After successful compilation, run the application binary with following command,
	`./KPIApp-test`

# Steps to deploy sources inside container
Kindly Refer UWC user guide for container deployments

# Steps to run unit test cases
1. Pre-requisites Installation for unit test execution
    1. Install latest version of "gcovr" tool for coverage report generation by using command "pip install gcovr".
    2. Install gtest (wget https://github.com/google/googletest/archive/release-1.8.0.tar.gz) libraries on host under /usr/local/ directory.
    3. All other Pre-requisites to be installed mentioned in section # Pre-requisites Installation
2. Follow same steps mentioned in section # Steps to compile Kpi-tactic , step 1 to step 3, but in "Build.test" directory of `Sourcecode/kpi-tactic/KPIApp` instead of "release" directory.
3. Run unit test cases
    1. Deploy ia_etcd container with dev mode using following steps. 
        1. Run `01_pre-requisites.sh --isTLS=no  --brokerAddr="mqtt_test_container" --brokerPort="11883" --qos=1 --deployMode=IPC_DEV` script
        2. Add `network_mode: host` option in two containers present in EdgeInsightsSoftware-v2.2-PV\IEdgeInsights\docker_setup\provision\dep\docker-compose-provision.yml file. 
        3. Run `02_provision_UWC_.sh` script to deploy ia_etcd container
    2. Go to `Sourcecode\kpi-tactic\KPIApp\Build.test` directory and open bash terminal.
    3. Export all environment variables required for KPIApp container. Refer environment section from kpi-tactic service present inside docker-compose_unit_test.yml file (E.g. `export AppName="KPIAPP"` for exporting AppName variable likewise all other variables needed to be exported in the same terminal) 
    4. After successful compilation, run the application binary with following command,
    `./KPIApp > kpi_tactics_status.log` 
    5. After successful execution of step 4, unit test log file `kpi_tactics_status.log` must be generated in the same folder that is Build.test
4. Generate unit test coverage report
    1. Go to `Sourcecode/kpi-tactic/KPIApp/Build.test` directory and open bash terminal.
    2. Run the command,
        `gcovr --html -e "../Test" -e "../include/ZmqHandler.hpp" -e "../include/Logger.hpp" -e "../include/EnvironmentVarHandler.hpp" -e ../../bin --exclude-throw-branches -o KPI_reoport.html -r .. .`
    3. After successful execution of step 2, unit test coverage report file `KPI_reoport.html` must be generated.

# Steps to enable/disable Kpi App for high performance processor(i7/i10)
1. By default in Kpi App, high performance processor functionality is disabled for debug mode & disabled for release mode.
2. Go to `Sourcecode\kpi-tactic\KPIApp\Release\src` directory and open subdir.mk file.
3. To enable Kpi App for high performance processor functionality, go to g++ command at line number 54 & add the option
	"-DUWC_HIGH_PERFORMANCE_PROCESSOR".
4. To disable Kpi App for high performance processor functionality, go to g++ command at line number 54 check & remove the option 
	"-DUWC_HIGH_PERFORMANCE_PROCESSOR" if found.

Notes : Above steps are to run KPIApp unit test locally. In order to run unit test in container, please follow the steps mentioned in section `## Steps to run unit test cases` of file `README.md` in Sourcecode directory. 

Troubleshooting steps:
1. If KPI-App is seen crashing, container restarting or AnalysisKPI logs not getting generated after building the UWC containers then as a troubleshooting step kindly run the 05_applyConfigChanges.sh, which would bring the containers down & up.