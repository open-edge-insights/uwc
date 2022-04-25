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

uwc-common sources details and build and run instructions.

# Contents:

1. [Directory and file details](#directory-and-file-details)
2. [Pre-requisites](#prerequisites-Installation)
3. [Compiling uwc-common sources on host machine](#compiling-uwc-common-sources-on-host-machine)
4. [Steps to compile uwc-common library](#steps-to-compile-uwc-common-library)
5. [Steps to run uwc-common executable on machine](#steps to run uwc-common executable on machine)
6. [Steps to deploy sources inside container](#steps-to-deploy-sources-inside-container)
7. [Steps to run unit test cases](#steps-to-run-unit-testcases)


## Directory and file details

This section describes the uwc_common directory contents and their uses.

* uwc_util - This directory contains sources for uwc common library container.
	1. `.settings` - Eclipse project configuration files.
	2. `Build.test` - Unit test configuration to run unit test cases and generating code coverage.
	3. `Config` - Contains logger configuration directory.
	4. `Debug` - Build configuration for Debug mode.
	5. `include` - This directory contains all the header files (i.e. .hpp) required to compile uwc common library.
	6. `lib` - This directory used to keep all the third party libraries (i.e. pthread , cjson, etc. ) required for uwc common container. 
	7. `Release` - Build configuration for Release mode. (This mode will be used for final deployment mode).
	8. `src` - This directory contains all .cpp files.
	9. `Test` - Contains unit test cases files. (.hpp, .cpp, etc.).
	10. `.cproject` - Eclipse project configuration files.
	11. `.project` - Eclipse project configuration files.
	12. `sonar-project.properties` - This file is required for Softdel CI CD process for sonar qube analysis.
2. `Dockerfile.common` - Dockerfile to install all the dependencies and libraries needed by all the containers.
3. `Dockerfile.common.test` - Dockerfile to install all the dependencies and libraries needed by all the unit test containers.
4. `Dockerfile_UT` - Dockerfile to build unit test container for uwc-common sources testing.

## Prerequisites Installation

For compiling uwc-common container sources on machine without container, following pre-requisites needs to installed on machine,

1. Install make and cmake.
2. Install wget by using command "sudo apt-get install wget".
3. Install Git by using command "sudo apt install git".
4. Install all the EII libraries on host from `EdgeInsightsSoftware-v2.2-PV\IEdgeInsights\common\libs` directory. (Refer EII README files to do the same).
5. Install log4cpp (version - 1.1.3, link - https://sourceforge.net/projects/log4cpp/files/latest/download/log4cpp-1.1.3.tar.gz) library under /usr/local/ directory.
6. Install yaml-cpp (branch - yaml-cpp-0.6.3, version - 0.6.3, link - https://github.com/jbeder/yaml-cpp.git) libraries on host under /usr/local/ directory.
7. Install paho-cpp (branch develop https://github.com/eclipse/paho.mqtt.c.git) libraries on host under /usr/local/ directory.
8. Install eclipse-tahu (branch develop https://github.com/eclipse/tahu) libraries on host under /usr/local/ directory.
9. Install ssl (https://www.openssl.org/source/openssl-1.1.1g.tar.gz) libraries on host under /usr/local/ directory.
	
## Steps to Compile uwc-common

1. Compile uwc_commonas follows:
	1. Copy all the Pre-requisite library files in /usr/local/lib/ and all the prerequisite header files in the /usr/local/include directory.
	1. Go to `Sourcecode\uwc_common\uwc_util\Release` directory and open a bash terminal.
	2. Execute `$make clean all` command.
	3. After successful completion of step 2, libuwc-common.so library must be created in same directory.

**Notes:** Above instructions are specified to build the sources in "Release" mode. To build sources in "Debug" mode, please execute the same steps in "Debug" folder inside `Sourcecode\uwc_common\uwc_util\` folder. 

## Steps to run uwc-common library on machine

Since it is not an executable, it cannot be executed.
	
## Steps to deploy sources inside container

Refer to the [UWC user guide](https://open-edge-insights.github.io/uwc-docs/) for container deployments.

## Steps to run unit test cases 

1. Prerequisites Installation for unit test execution
	a. Install latest version of "gcovr" tool for coverage report generation by using command "pip install gcovr".
	b. Install gtest (wget https://github.com/google/googletest/archive/release-1.8.0.tar.gz) libraries on host under /usr/local/ directory.
	c. All other Pre-requisites to be installed mentioned in section # Pre-requisites Installation
2. Follow same steps mentioned in section # Steps to compile uwc-common, but in "Build.test" directory of `Sourcecode\uwc_common\uwc_util` instead of `Release` directory.
3. Run unit test cases
	a. Deploy ia_etcd container with dev mode using following steps. 
		1. Run `01_pre-requisites.sh --isTLS=no  --brokerAddr="mqtt_test_container" --brokerPort="11883" --qos=1 --deployMode=IPC_DEV` script
		2. Add `network_mode: host` option in two containers present in EdgeInsightsSoftware-v2.2-PV\IEdgeInsights\docker_setup\provision\dep\docker-compose-provision.yml file.
		3. Run `02_provisionEII.sh` script to deploy ia_etcd container
	2. Go to `Sourcecode\uwc_common\uwc_util\Build.test` directory and open bash terminal.
	3. Export all environment variables required for uwc-util-test container. Refer environment section from uwc-util-test service present inside docker-compose_unit_test.yml file (E.g. `export AppName="UWCUTIL"` for exporting AppName variable likewise all other variables needed to be exported in the same terminal) 
	4. After successful compilation, run the application binary with following command,
	`./uwc-common > /reports/uwc-util/UWCUTIL_status.log`
	5. After successful execution of step 4, unit test log file `UWCUTIL_status.log` must be generated in the same folder that is Build.test
4. Generate unit test coverage report
	1. Go to `Sourcecode/uwc_common/uwc_util/Build.test` directory and open bash terminal.
	2. Run the command,
		`gcovr --html -e "../Test/" -e "../include/log4cpp/Appender.hh" -exclude-throw-branches -o UWCUTIL_Report.html -r .. .`
	3. After successful execution of step 2, unit test coverage report file `UWCUTIL_Report.html` must be generated.
5. Run unit test cases inside container
	1. Follow the steps mentioned in section `## Steps to run unit test cases` of file `README.md` in the Sourcecode directory.
