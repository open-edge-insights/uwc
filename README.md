# Universal Wellpad Controller

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

# Contents:
1. [Directory details](#directory-details)
2. [Install generic prerequisites](#install-generic-prerequisites)
3. [Install Prerequisites for Universal Wellpad Controller](#install-prerequisites-for-universal-wellpad-controller)
4. [Provision and Build of the Edge Insights for Industrial and Universal Wellpad Controller services](#provision-and-build-of-the-edge-insights-for-industrial-and-universal-wellpad-controller-services)
5. [Run Edge Insights for Industrial and Universal Wellpad Controller services](#run-edge-insights-for-industrial-and-universal-wellpad-controller-services)
6. [Multistage build](#multi-stage-build)
7. [Verify container status](#verify-container-status)
8. [Apply configuration changes](#apply-configuration-changes)
9. [Uninstallation script](#uninstallation-script)
10. [Data Persistence feature](#data-persistence-feature)
11. [Sample Database Publisher](#sample-database-publisher)
12. [Unit Tests](#unit-tests)
13. [Debugging steps](#debugging-steps) 
14. [Troubleshooting](#troubleshooting)

## Directory details

The uwc directory consists of the following:

* modbus-master: This directory contains the modbus TCP1 and RTU1 containers sources and docker file for building the container. It also has the ingredient docker-compose.yml and config.json for modbus TCP & RTU services. For details, refer to the `README-modbus-master.md` file of modbus-master folder.
* mqtt-bridge: This directory contains the mqtt-bridge container sources and docker file for building the container. It also has the ingredient docker-compose.yml and config.json for mqtt-bridge service. For details, refer to the `README-mqtt-bridge.md` file of mqtt-bridge folder.
* MQTT: This directory contains the mqtt container sources and docker file for building the container. It also has the ingredient docker-compose.yml and config.json for MQTT internal broker service. For details, refer to the `README-MQTT.md` file in the MQTT folder.
* sparkplug-bridge: This directory contains the sparkplug-bridge sources and docker file for building the container. It also has the ingredient docker-compose.yml and config.json for SPARKPLUG-BRIDGE service. For details, refer to the `README_sparkplug_bridge.md` file in the sparkplug-bridge folder.
* uwc-common: This directory contains sources for the uwc common library container and Dockerfile to install all the dependencies and libraries required by the containers. For details, refer to the `README-UWC_Common.md` file in the uwc_common folder.
* kpi-tactic: This directory contains the kpi-tactic container sources and docker file for building the container. For details, refer to the `README-kpi-tactic.md` file in the kpi-tactic folder.
* Vendor_Apps: This directory contains the sample-publisher and sample-subscriber container sources and docker file for building the container. For details, refer to the `README-VA.md` file in the Vendor_Apps folder.
* Others: This directory contains configurations for ETCD required during provisioning
* uwc_common:
  This directory contains common dockerfiles for Universal Wellpad Controller.
* eii_configs:
  This directory contains the config files specific to Universal Wellpad Controller which would replace the default EII config files that come as part of cloning the ingredient EII git repos.
* uwc_recipes:
  This directory contains all the recipe use cases of Universal Wellpad Controller (Universal Wellpad Controller services in different combinations).

## Install Generic Prerequisites

1. To install the prerequisites, follow the steps in the section `EII-Prerequisites-Installation` of the `<working-directory>/IEdgeInsights/README.md`.

  ```sh
     <working-dir>/IEdgeInsights/build
     sudo ./pre_requisites.sh --help
  ```
  **Note:** 
  If the error "Docker CE installation step is failed" is seen while running pre-requisite.sh script on a new system, then rerun the pre_requisite.sh script again. This is a known bug in the docker community for Docker CE.

2. If the required Universal Wellpad Controller code base is not yet repo synched or (git repositories cloned), then follow the repo or git steps from `<working-directory>/IEdgeInsights/../.repo/manifests/README.md` to repo sync or git clone the codebase.
3. [optional] Steps to apply the RT Patch.

  * To install the RT_patch disable secure boot (as per the limitation of Ubuntu OS).
  * To find the exact RT Patch for your system and install it manually, refer to the links, https://mirrors.edge.kernel.org/pub/linux/kernel/ or https://mirrors.edge.kernel.org/pub/linux/kernel/projects/rt/ 
  
 **Note:** 
 The steps to install Universal Wellpad Controller are available in the Universal Wellpad Controller User Guide. For more information, see https://open-edge-insights.github.io/uwc-docs/Pages/page_04.html
    
## Install Prerequisites for Universal Wellpad Controller 

To install the prerequisites, run the following scripts that are specific to Universal Wellpad Controller from the `IEdgeInsights\uwc` directory
  
  ```sh
    cd <working-dir>/IEdgeInsights/uwc/build_scripts
    sudo -E ./01_uwc_pre_requisites.sh
  ```

## Provision and Build of the Edge Insights for Industrial and Universal Wellpad Controller services

Runs the builder script enabling to choose the Universal Wellpad Controller recipe needed for the use case. Next, it prompts you to select `develeopment mode` or `production mode` and prepares the setup to run in the selected mode. Next it prompts the user to select `pre-build images` or `build images locally` Finally it does the provisioning of EII and Universal Wellpad Controller services based on the recipe & mode selected and builds all the microservices of the recipe. 

**Example:**

```sh
    cd <working-dir>/IEdgeInsights/uwc/build_scripts
    sudo -E ./02_provision_build_UWC.sh
```
**Note:** The commands provided in the example will execute the script in the interactive mode. 

The script will run in the non-interactive mode when the command line arguments are provided. The help option describes all command line arguments.

```sh
    cd <working-dir>/IEdgeInsights/uwc/build_scripts
    sudo ./02_provision_build_UWC.sh --help
```
## Run Edge Insights for Industrial and Universal Wellpad Controller services

The script runs the Edge Insights for Industrial (EII) and Universal Wellpad Controller services as containers in background (daemon process).

```sh
  cd <working-dir>/IEdgeInsights/uwc/build_scripts
  sudo -E ./03_Run_UWC.sh 
```

## Multistage Build

There are two ways in which the containers can be built either by using the opensource pre-build images as present in [openedgeinsights](https://hub.docker.com/u/openedgeinsights) or by building images locally. 

**Note:** For usage details, run the following command:

```sh
 sudo ./02_provision_build_UWC.sh --help
```

## Verify Container Status

The following command verifies the container status:

```sh
sudo docker ps
```

## Apply Configuration Changes

If any configuration changes are made to the Universal Wellpad Controller YML files, then running the following script will bring down the containers and bring them up back. 

**Note:** The services are not built here.
 
 ```sh
   cd <working-dir>/IEdgeInsights/uwc/build_scripts
   sudo -E 05_applyConfigChanges.sh
  ```

## Uninstallation Script

Using the uninstallation script, you can uninstall and remove the Universal Wellpad Controller installation. Run the following commands:
  
  ```sh
   cd <working-dir>/IEdgeInsights/uwc/build_scripts
   sudo -E 04_uninstall_UWC.sh
  ```

## Data Persistence Feature
 
The Data Persistence feature in Universal Wellpad Controller enables the user to save the response JSON received from the end device (or simulator) regarding the data points in database (InfluxDB). The response received in JSON format is converted to metrics inside the Telegraf* microservice before passing the data to InfluxDB for storage. The data can be stored in the database by adding a field called "dataPersist" in the data points YML configuration files (flowmeter_datapoints.yml or iou_datapoints.yml). The possible values for this include "true" or "false" (Boolean values). Data is stored in the database if the dataPersist field value is true and not stored if the value is false. Also, if the field "dataPersist" is skipped, then the datapoint is not stored.

 **Notes:**
 * Any string value in the JSON response payload which is published to Telegraf* should have all the string fields listed in the Telegraf configuration file for enabling the metrics convertion feature of Telegraf* to take effect. 
 * The retention period can be configured in the config.json file of the InfluxDBConnector* microservice. For more information, see `https://github.com/open-edge-insights/eii-influxdb-connector/blob/master/config.json`. The default retention period is set to 24 hours. Although the field "retention" in the config.json file is set to 23 hours, it includes a default shard duration of 1 hour which totally accounts to 24 hours of data retention. For more information, refer to the InfluxDB* documentation "https://www.influxdata.com/blog/influxdb-shards-retention-policies/".

## Sample Database Publisher

A sample database publisher publishes the sample JSON data onto the EII MessageBus ZMQ broker. The JSON payload published on ZMQ broker is subscribed by Telegraf* and written to the Influx database based on the "dataPersist" flag being `true` or `false` in the input JSON payload. If the "dataPersist" flag is `true` then the JSON data is written to InfluxDB and if the flag is `false` then the JSON data is not written. This DB publisher app is containerized and doesn't get involved with any of the Universal Wellpad Controller services.

Follow the steps to running the DB publisher app:

1. Ensure that the [EmbPublisher APP](../tools/EmbPublisher/) is present after the repo is cloned.

2. The Universal Wellpad Controller Embpublisher's config.json and the json input file is present in [eii_configs](./eii_configs/service_specific_cfgs/EmbPublisher/).

3. You can modify the [UWC_Sample_DB_Publisher_config.json](./eii_configs/service_specific_cfgs/EmbPublisher/UWC_Sample_DB_Publisher_config.json) according to requirement. Ensure if the string key is to be added in sample JSON payload then, add the string key in the [Telegraf_devmode.conf](./eii_configs/service_specific_cfgs/Telegraf/Telegraf_devmode.conf) or [Telegraf.conf](./eii_configs/service_specific_cfgs/Telegraf/Telegraf.conf) json_string_fields section.

 ```yaml
 <string_key>: <value_key>
 ```

4. After editing ensure that the [config.json](./eii_configs/service_specific_cfgs/EmbPublisher/config.json) "msg_file" filed is pointing to required file.

 ```yaml
 "msg_file": "UWC_Sample_DB_Publisher_config.json"
 ```
5. Run the [01_uwc_pre_requisites.sh](./build_scripts/01_uwc_pre_requisites.sh) script.

**Note:**

Rerun the 01_uwc_pre_requisites.sh script after making any changes to the config files mentioned earlier.

## Unit Tests

All the Universal Wellpad Controller modules have unit tests enabled in the Production mode. To run the unit tests, follow the steps below:

```sh
 cd <working-dir>/IEdgeInsights/uwc/build_scripts
 sudo -E ./06_UnitTestRun.sh "false"
```
Check for the unit test reports for all services of Universal Wellpad Controller in "<working-dir>/IEdgeInsights/build/unit_test_reports/".

## Debugging steps
  
The following are the steps for debugging:
  
1. Checking the container logs 
   Syntax - sudo docker logs <container_name>
   Example, to check modbus-tcp-container logs execute "sudo docker logs modbus-tcp-container" command.
2. Command to check logs inside the container "sudo docker exec -it <container_name> bash"
3. Use "cat <log_file_name>" to see log file inside the container
4. Copying logs from container to host machine
   Syntax - docker cp <container_name>:<file to copy from container> <file to be copied i.e., host directory>
5. To check the IP address of machine, use "ifconfig" command.
6. For Modbus RTU, to check attached COM port for serial communication, use "dmesg | grep tty" command.
7. The container logs for each of the Universal Wellpad Controller microservice is volume mounted in the directory "/opt/intel/eii/container_logs/", with a separate sub-directory for each micro-service.
8. Also the Universal Wellpad Controller configuration files can be edited in the sub-directory "/opt/intel/eii/uwc_data", if there is a need to change the configurations.

## Redirect docker logs to file including errors

To redirect docker logs run the following command:

  ```sh
   docker logs modbus-tcp-container > docker.log 2>&1
  ```

* The ETCD UI is available at
  * For the Dev mode `http://localhost:7070/etcdkeeper/` 
  * For the Prod mode `https://localhost:7071/etcdkeeper/` 
  
  > **Note:** Use the following credentials, username is root, and the password is eii123.

* EII Message Bus related interface configurations (including secrets) for all Universal Wellpad Controller and EII containers are stored in the ia_etcd server. You can access the ia_etcd server using the ETCD UI as mentioned earlier.

**Notes:**
  
*  If docker-compose.yml is modified then bring down the containers and then bring them up as mentioned.
*  If previous containers are running on the deploy machine, then stop the containers using the command for bringing down containers as mentioned above.
*  Can use MQTT.FX or any other MQTT client to verify the flow or all 6 operations for RT/Non-RT, read/write,polling operations

  ## Troubleshooting
  
Refer to the following for troubleshooting:
  
* To set proxy for docker, follow method 2 as mentioned here [Configuring environment variables](https://www.thegeekdiary.com/how-to-configure-docker-to-use-proxy/).
* In prod mode, the "Certificates" directory in "<working-dir>/IEdgeInsights/build" needs 'sudo su" to be accessed. To open the Certificates folder, do the following:

  ```sh
   cd <working-dir>/IEdgeInsights/build
   sudo su
   cd Certificates
  # After accessing Certificates, enter the "exit" command and the terminal will return back to normal mode.
   exit
  ```
* If the KPI-Tactic application is seen crashing, container restarting, or the AnalysisKPI.log files is not getting generated after building the Universal Wellpad Controller containers then run the 05_applyConfigChanges.sh. This will bring the containers down and up.
* The steps to find and patch RT kernel for the Ubuntu 20.04 is in the [User Guide](https://open-edge-insights.github.io/uwc-docs/Pages/page_14.html).
