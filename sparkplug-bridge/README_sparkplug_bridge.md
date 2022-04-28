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

Sparkplug-bridge sources details and build and run instructions.

# Contents

1. [Directory and file details](#directory-and-file-details)
2. [Prerequisites Installation](#prerequisites-installation)
3. [Steps to Compile Sparkplug-bridge](#steps-to-compile-sparkplug-bridge)
4. [Steps to Run Sparkplug-bridge Executable on Machine](#steps-to-run-sparkplug-bridge-executable-on-machine)
5. [Steps to Deploy Sources inside Container](#steps-to-deploy-sources-inside-container)
6. [Steps to Run Unit Test Cases](#steps-to-run-unit-testcases)


## Directory and File Details

This section describes the sprarkplug-bridge directory contents and their uses.

* SPARKPLUG-BRIDGE - This directory contains sources for Sparkplug-bridge App container.
	- `.settings` - Eclipse project configuration files.
	- `Build.test` - Unit test configuration to run unit test cases and generating code coverage.
	- `Config` - Contains logger configuration directory.
	- `Debug` - Build configuration for Debug mode.
	- `include` - This directory contains all the header files (i.e. .hpp) required to compile sparkplug-bridge container.
	- `lib` - This directory used to keep all the third party libraries (i.e. pthread , cjson, etc. ) required for sparkplug-bridge container. 
	- `Release` - Build configuration for Release mode. This mode will be used for final deployment mode.
	- `src` - This directory contains all .cpp files.
	- `Test` - Contains unit test cases files. (.hpp, .cpp, etc.)
	- `.cproject` - Eclipse project configuration files.
	- `.project` - Eclipse project configuration files.
	- `sonar-project.properties` - This file is required for Softdel CICD process for sonar qube analysis
* `Dockerfile` - Dockerfile to build sparkplug-bridge container.
* `Dockerfile_UT` - Dockerfile to build unit test container for sparkplug-bridge sources testing.
* `config.json` - Ingredient config.json for sparkplug-bridge micro service.

## Prerequisites Installation

Install the prerequisites to compile the sparkplug-bridge container sources on a machine without container:

1. Install make and cmake.
2. Install wget by using command "sudo apt-get install wget".
3. Install Git by using command "sudo apt install git".
4. Install all the EII libraries on host by running the shell script -- `sudo -E ./eii_libs_installer.sh`. Refer the README.md from  `IEdgeInsights\common\README.md` for details.
5. Install log4cpp (version - 1.1.3, link - https://sourceforge.net/projects/log4cpp/files/latest/download/log4cpp-1.1.3.tar.gz) library under /usr/local/ directory.
6. Install yaml-cpp (branch - yaml-cpp-0.6.3, version - 0.6.3, link - https://github.com/jbeder/yaml-cpp.git) libraries on host under /usr/local/ directory.
7. Install paho-cpp (branch develop https://github.com/eclipse/paho.mqtt.c.git) libraries on host under /usr/local/ directory.
8. Install eclipse-tahu (branch develop https://github.com/eclipse/tahu) libraries on host under /usr/local/ directory.
9. Install ssl (https://www.openssl.org/source/openssl-1.1.1g.tar.gz) libraries on host under /usr/local/ directory.
10. Install uwc_common library refering to `README_UWC_Common.md` of `uwc_common` folder and copy `uwc_common/uwc_util/lib/libuwc-common.so` and `uwc_common/uwc_util/include/all .hpp files` in `Sourcecode\sparkplug-bridge\SPARKPLUG-BRIDGE\lib` and `Sourcecode\sparkplug-bridge\SPARKPLUG-BRIDGE\include` directory respectively.
	
## Steps to Compile Sparkplug-bridge 

1. Go to `Sourcecode\sparkplug-bridge\SPARKPLUG-BRIDGE\Release directory` and open a terminal.
2. Execute the ``$make clean all`` command.
3. After successfull execution of step 3, application executable must be generated in current directory (i.e. Release).

**Notes:** Above instructions are specified to build the sources in "Release" mode. To build sources in the "Debug" mode, execute the same steps in the "Debug" folder inside the `Sourcecode\sparkplug-bridge\SPARKPLUG-BRIDGE\` folder. 

## Steps to Run Sparkplug-bridge Executable on Machine

1. Deploy ia_etcd container with dev mode using following steps. 
	1. Run `01_uwc_pre_requisites.sh` script as explained in the main uwc/README.md.
	2. Run the provisioning command script to deploy ia_etcd container as explainedin main uwc/README.md.
2. Go to `Sourcecode\sparkplug-bridge\SPARKPLUG-BRIDGE\Release` directory and open bash terminal.
3. Set EII specific environment variables using below command.
	`source <Complete Path of .env file present inside IEdgeInsights/build directory>`
	For example, `source /home/intel/uwc-releases/IEdgeInsights/build/.env`
4. Export all other environment variables required for sparkplug-bridge container. Refer environment section from sparkplug-bridge service present inside docker-compose.yml file of sparkplug-bridge service (E.g., `export AppName="SPARKPLUG-BRIDGE"` for exporting AppName variable likewise all other variables needed to be exported in the same terminal) 
5. After successful compilation, run the application binary with following command,
	`./SPARKPLUGBRIDGE`
	
## Steps to Deploy Sources Inside Container

Refer to the [Universal Wellpad Controller User Guide](https://open-edge-insights.github.io/uwc-docs/) for container deployments.

## Steps to Run Unit Test Cases 

1. Prerequisites installation for unit test execution.
	a. Install latest version of "gcovr" tool for coverage report generation by using command "pip install gcovr".
	b. Install gtest (wget https://github.com/google/googletest/archive/release-1.8.0.tar.gz) libraries on host under /usr/local/ directory.
	c. All other prerequisites to be installed mentioned in section #Prerequisites Installation.
2. Follow same steps mentioned in section # Steps to compile sparkplug-bridge, step 1 to step 3, but in "Build.test" directory of `Sourcecode\sparkplug-bridge\SPARKPLUG-BRIDGE` instead of the "Release" directory.
3. Run unit test cases
	a. Go to the `Sourcecode\sparkplug-bridge\SPARKPLUG-BRIDGE\Build.test` directory and open bash terminal.
	b. Export all environment variables required for sparkplug-bridge-test container. Refer environment section from sparkplug-bridge-test service present inside docker-compose_unit_test.yml file (E.g. `export AppName="SPARKPLUG-BRIDGE"` for exporting AppName variable likewise all other variables needed to be exported in the same terminal) 
	c. After successful compilation, run the application binary with following command,
	`./SPARKPLUG-BRIDGE-TEST > SPARKPLUGBRIDGE_status.log`
	d. After successful execution of step 4, unit test log file `SPARKPLUGBRIDGE_status.log` must be generated in the same folder that is Build.test
4. Generate unit test coverage report
	a. Go to `Sourcecode\sparkplug-bridge\SPARKPLUG-BRIDGE\Build.test` directory and open bash terminal.
	b. Run the command,
		`gcovr --html -e "../Test" -e "../include/utils" -e "../include/tahu" -e "../include/yaml-cpp" -e "../src/utils" -e "../include/log4cpp/Appender.hh" -e "../include/ConfigManager.hpp" -e "../include/EnvironmentVarHandler.hpp" -e "../include/Logger.hpp" -e "../include/NetworkInfo.hpp" --exclude-throw-branches -o SPARKPLUGBRIDGE_Report.html -r .. .`
	c. After successful execution of step 2, unit test coverage report file `SPARKPLUGBRIDGE_Report.html` must be generated.
5. Run unit test cases inside container
	a. Follow the steps mentioned in section `## Steps to run unit test cases` of file `README.md` in Sourcecode directory.

**Notes:** When unit test is executed locally (not inside container), two test cases fail and coverage is 10% less. This is because cert files paths which are mentioned in constructor of class CSCADAHandler in SCADAHandler.cpp are untraceable.

All GPL/LGPL/AGPL & Eclipse distribution license binary distributed components source code will be distributed via separate docker image ia_edgeinsights_uwc_src hosted in https://hub.docker.com/u/openedgeinsights. And the corresponding Docker file is present in "uwc/licenses/Dockerfile-sources/" folder.

