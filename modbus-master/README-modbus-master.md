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

Modbus containers and Modbus stack sources details and build and Run instructions
This is applicable for TCP as well as RTU.

# Contents:

1. [Directory and File Details](#directory-and-file-details)

2. [Prerequisites Installation](#prerequisites-installation)

3. [Steps to Compile Modbus-tcp-master](#steps-to-compile-modbus-tcp-master)

4. [Steps to Compile Modbus-rtu-master](#steps-to-compile-modbus-rtu-master)

5. [Steps to Run Modbus Executable on Machine](#steps-to-run-modbus-executable-on-machine)

6. [Steps to Deploy Sources inside Container](#steps-to-deploy-sources-inside-container)

7. [Steps to Run Unit Test Cases](#steps-to-run-unit-test-cases)

8. [Steps to Enable or Disable Instrumentation Logs](#steps-to-enable-or-disable-instrumentation-logs)


## Directory and File Details

This section describes the modbus-master directory contents and their uses.

* Modbus-App - This directory contains sources for Modbus App implementation for TCP and RTU containers.
	- `.settings` - Eclipse project configuration files.
	- `Build.test` - Unit test configuration to run unit test cases and generating code coverage.
	- `Config` - Contains logger configuration directory.
	- `Debug` - Build configuration for Debug mode.
	- `include` - This directory contains all the header files (i.e. .hpp) required to compile the modbus container.
	- `lib` - This directory used to keep all the third party libraries i.e., pthread , cjson, and so on required for the modbus container. 
	- `Release` - Build configuration for Release mode. This mode will be used for final deployment mode.
	- `src` - This directory contains all .cpp files.
	- `Test` - Contains unit test cases files such as .hpp, .cpp, and so on.
	- `.cproject` - Eclipse project configuration files.
	- `.project` - Eclipse project configuration files.
	- `sonar-project.properties` - This file is required for Softdel CICD process for sonar qube analysis.
	- modbus_RTU: This folder is in parallel to Modbus-App & contains indivisual docker-compose & config files for modbus-RTU.
	- modbus-TCP: This folder is in parallel to Modbus-App & contains indivisual docker-compose & config files for modbus-RTU.

* SoftMod_Stack - This directory contains sources for Modbus Stack implementation for TCP and RTU. 
	- `.settings` - Eclipse project configuration files.
	- `Debug` - Build configuration for Debug mode.
	- `include` - This directory contains all the header files (i.e., .hpp) required to compile modbus container.
	- `lib` - This directory used to keep all the third party libraries (i.e., pthread , cjson, etc. ) required for modbus container. 
	- `Release` - Build configuration for Release mode. This mode will be used for final deployment mode.
	- `src` - This directory contains all .cpp files.
	- `.cproject` - Eclipse project configuration files
	- `.project` - Eclipse project configuration files.
	- `sonar-project.properties` - This file is required for Softdel CICD process for sonar qube analysis.
* `Dockerfile` - Dockerfile to build modbus-tcp-master container.
* `Dockerfile_RTU` - Dockerfile to build modbus-rtu-master container.
* `Dockerfile_UT` - Dockerfile to build unit test container for modbus-tcp-master sources testing.
* `Dockerfile_UT_RTU` - Dockerfile to build unit test container for modbus-rtu-master sources testing.
		
## Prerequisites Installation

For compiling modbus container sources on machine without container, following pre-requisites needs to installed on machine,

1. Install make and cmake.
2. Install wget by using command "sudo apt-get install wget".
3. Install Git by using command "sudo apt install git".
4. Install all the EII libraries on host by running the shell script -- `sudo -E ./eii_libs_installer.sh`. Refre the README.md from  `IEdgeInsights\common\README.md` for details.
5. Install log4cpp (version - 1.1.3, link - https://sourceforge.net/projects/log4cpp/files/latest/download/log4cpp-1.1.3.tar.gz) library under /usr/local/ directory.
6. Install yaml-cpp library (branch - yaml-cpp-0.6.3, version - 0.6.3, link - https://github.com/jbeder/yaml-cpp.git) libraries on host under /usr/local/ directory.
7. Install paho-cpp (branch develop https://github.com/eclipse/paho.mqtt.c.git) libraries on host under /usr/local/ directory.
8. Install uwc_common library refering to `README_UWC_Common.md` of `uwc_common` folder and copy `uwc_common/uwc_util/lib/libuwc-common.so` and `uwc_common/uwc_util/include/all .hpp files` in `Sourcecode\modbus-master/Modbus-App/lib ` and `Sourcecode/modbus-master/Modbus-App/include` directory respectively.
	
# Steps to Compile Modbus-tcp-master 

1. Compile SoftMod_Stack with following steps,
	1. Go to `Sourcecode\modbus-master\SoftMod_Stack\Release` directory and open a bash terminal.
	2. Execute `$make clean all` command.
	3. After successful completion of step 2, libModbusMasterStack.so library must be created in same directory.
	4. Copy libModbusMasterStack.so librray in `Sourcecode\modbus-master\Modbus-App\lib `directory
2. Go to `Sourcecode\modbus-master\Modbus-App\Release directory` and open a terminal.
3. Execute ``$make clean all`` command
4. After successfull execution of step 3, application executable must be generated in current directory (i.e. Release).

# Steps to Compile Modbus-rtu-master

1. Compile SoftMod_Stack with following steps,
	1. Go to `Sourcecode\modbus-master\SoftMod_Stack\Release` directory and open a bash terminal.
	2. Remove `-DMODBUS_STACK_TCPIP_ENABLED` macro from `Sourcecode\modbus-master\SoftMod_Stack\Release\Src\subdir.mk` file.
	3. Execute `$make clean all` command.
	4. After successful completion of step 2, libModbusMasterStack.so library must be created in same directory.
	5. Copy libModbusMasterStack.so library in `Sourcecode\modbus-master\Modbus-App\lib` directory
2. Go to `Sourcecode\modbus-master\Modbus-App\Release` directory and open a terminal.
3. Remove `-DMODBUS_STACK_TCPIP_ENABLED` macro from `Sourcecode\modbus-master\Modbus-App\Release\src\subdir.mk` file.
4. Execute `$make clean all` command.
5. After successful execution of step 2, application executable (with name `ModbusMaster`) must be generated in current directory (i.e. Release).

Notes : Above instructions are specified to build the sources in "Release" mode. To build sources in "Debug" mode, please execute the same steps in "Debug" folder inside `Sourcecode\modbus-master\` folder. 

# Steps to Run Modbus Executable on Machine

1. Deploy ia_etcd container with dev mode using following steps. 
	1. Run `preReq.sh` script as explained in the main uwc/README.md.
	2. Add `network_mode: host` option in two containers present in IEdgeInsights\build\provision\dep\docker-compose-provision.yml file.
	3. Run th eprovisioning command script to deploy ia_etcd container as explainedin main uwc/README/md.
2. Go to `Sourcecode\modbus-master\Modbus-App\Release` directory and open bash terminal.
3. Set EII specific environment variables using below command.
	`source <Complete Path of .env file present inside IEdgeInsights/build directory>`
	For example `source /home/intel/uwc-releases/IEdgeInsights/build/.env`
4. Export all other environment variables required for modbus container. Refer environment section from modbus-tcp-master service present inside docker-compose.yml file (E.g. `export AppName="TCP"` for exporting AppName variable likewise all other variables needed to be exported in the same terminal) for modbus-tcp-master container and for RTU refer same section from modbus-rtu-master container 
5. After successful compilation, run the application binary with following command,
	`./ModbusMaster`
	
# Steps to Deploy Sources Inside Container

Refer to the Universal Wellpad Controller user guide for container deployments.

## Steps to Run Unit Test Cases 

1. Install prerequisites for unit test execution.
	a. Install latest version of "gcovr" tool for coverage report generation by using command "pip install gcovr".
	b. Install gtest (wget https://github.com/google/googletest/archive/release-1.8.0.tar.gz) libraries on host under /usr/local/ directory.
	c. All other prerequisites to be installed mentioned in section # Pre-requisites Installation.
2. Follow same steps mentioned in section # Steps to compile modbus-tcp-master and # Steps to compile modbus-rtu-master, step 1 to step 4/5, but in "Build.test" directory of `Sourcecode/modbus-master/Modbus-App` instead of "Release" directory.
3. Run unit test cases
	a. Deploy ia_etcd container with dev mode using following steps. 
		i. Run `01_pre-requisites.sh --isTLS=no  --brokerAddr="mqtt_test_container" --brokerPort="11883" --qos=1 --deployMode=IPC_DEV` script.
		ii. Add `network_mode: host` option in two containers present in EdgeInsightsSoftware-v2.2-PV\IEdgeInsights\docker_setup\provision\dep\docker-compose-provision.yml file.
		iii. Run `02_provision_UWC.sh` script to deploy ia_etcd container.
	b. Go to `Sourcecode/modbus-master/Modbus-App/Build.test` directory and open bash terminal.
	c. Follow steps 3 and 4 of `# Steps to run modbus executable on machine` section.
	d. After successful compilation, run the application binary with following command,
	`./ModbusMaster_test > modbus-tcp-master_test_status.log` for modbus-tcp and `./ModbusMaster_test > modbus-rtu-master_test_status.log` for modbus-rtu.
	e. After successful execution of step 4, unit test log file `modbus-tcp-master_test_status.log` and `modbus-rtu-master_test_status.log` must be generated in the same folder that is Build.test.
4. Generate unit test coverage report
	a. Go to `Sourcecode/modbus-master/Modbus-App/Build.test` directory and open bash terminal.
	b. Run the command,
		`gcovr --html -e "../Test" -e "../include/log4cpp" -e ../../bin --exclude-throw-branches -o ModbusTCP_report.html -r .. .` and `gcovr --html -e "../Test" -e "../include/log4cpp" -e ../../bin --exclude-throw-branches -o ModbusRTU_report.html -r .. .` for modbus-tcp and modbus-rtu respectively.
	c. After successful execution of step 2, unit test coverage report file `ModbusTCP_report.html/ModbusRTU_report.html` must be generated.
5. Run unit test cases inside container
	a. Follow the steps mentioned in the section `## Steps to run unit test cases` of `README.md` in the Sourcecode directory.

## Enable or Disable Instrumentation Logs

By default, the instrumentation logs are enabled for debug mode and disabled for the release mode. Complete the following steps to manage (enable or disable) intrumentation logs:

1. Go to the `Sourcecode\modbus-master\Modbus-App\Release\src` directory 
2. Open the subdir.mk file.
3. To enable the instrumentation logs, go to g++ command at line number 41 and add the option "-DINSTRUMENTATION_LOG".
4. To disable the instrumentation logs, go to g++ command at line number 41 check and remove the option "-DINSTRUMENTATION_LOG" (if found).

