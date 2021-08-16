# Universal Wellpad Controller (UWC)

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
2. [Install generic pre-requisites](#install-generic-pre-requisites)
3. [Install UWC specific pre-requisites](#install-UWC-specific-pre-requisites)
4. [Provision EII and UWC services](#Provision-EII-and-UWC-services)
5. [Build and Run all EII and UWC services](#build-and-run-all-eii-and-uwc-services)
6. [Verify container status](#verify-container-status)
7. [Apply configuration changes](#apply-configuration-changes)
8. [Uninstallation script](#uninstallation-script)
9. [Data Persistence feature](#data-persistence-feature)
10. [Sample DB Publisher](#sample-db-publisher)
11. [Unit Tests](#unit-tests)
12. [Debugging steps](#debugging-steps) 
13. [Troubleshooting](#troubleshooting)

## Directory details
The directory comprises of following:
* modbus-master :
  This directory contains the modbus TCP1 and RTU1 containers sources and docker file for building the container. It also has the ingredient docker-compose.yml & config.json for modbus TCP & RTU services.
  For detail, please refer to `README-modbus-master.md` file of modbus-master folder
* mqtt-bridge :
  This directory contains the mqtt-bridge container sources and docker file for building the container. It also has the ingredient docker-compose.yml & config.json for mqtt-bridge service. 
  For detail, please refer to `README-mqtt-bridge.md` file of mqtt-bridge folder
* MQTT:
  This directory contains the mqtt container sources and docker file for building the container. It also has the ingredient docker-compose.yml & config.json for MQTT internal broker service.
  For detail, please refer to `README-MQTT.md` file of MQTT folder
* sparkplug-bridge:
  This directory contains the sparkplug-bridge sources and docker file for building the container. It also has the ingredient docker-compose.yml & config.json for SPARKPLUG-BRIDGE service.
  For detail, please refer to `README_sparkplug_bridge.md` file of sparkplug-bridge folder
* uwc-common:
  This directory contains sources for uwc common library container & Dockerfile to install all the dependencies and libraries needed by all the containers. For detail, please refer to `README-UWC_Common.md` file of uwc_common folder
* kpi-tactic:
  This directory contains the kpi-tactic container sources and docker file for building the container 
  For detail, please refer to `README-kpi-tactic.md` file of kpi-tactic folder
* Others:
  This directory contains configurations for ETCD required during provisioning
* uwc_common:
  This directory contains common dockefiles for UWC
* eii_configs:
  This directory contains the config files specific to UWC which would replace the default EII config files that come as part of cloning the ingredient EII git repos.
* uwc_recipes:
  This directory contains all the recipe use cases of UWC. (UWC services in different combinations).
## Install generic pre-requisites

1. Follow the steps in the section `EII-Prerequisites-Installation` of `<working-directory>/IEdgeInsights/README.md`  to install all the pre-requisites.

  ```sh
    $ <working-dir>/IEdgeInsights/build
    $ sudo ./pre_requisites.sh --help
  ```
     Note - If the error "Docker CE installation step is failed" is seen while running pre-requisite.sh script on a fresh system then kindly re-run the pre_requisite.sh script again.
            This is a known bug in docker community for Docker CE.

2. If the required UWC code base is not yet repo synched or (git repositories cloned), then kindly follow the repo or git steps from  `<working-directory>/IEdgeInsights/../.repo/manifests/README.md` to repo sync or git clone the codebase.

3. [optional] Steps to Apply RT Patch.

  * If user wants to install RT_patch they need to disable secure boot as per limitation of ubuntu.

  * Please go through the links https://mirrors.edge.kernel.org/pub/linux/kernel/ or https://mirrors.edge.kernel.org/pub/linux/kernel/projects/rt/ to find the exact RT Patch for your system and install it manually.    
    
## Install UWC specific pre-requisites
All the below UWC specific scripts need to be run from the directory `IEdgeInsights\uwc`:
  ```sh
    $ cd <working-dir>/IEdgeInsights/uwc/build_scripts
    $ sudo ./01_uwc_pre_requisites.sh
  ```

## Provision EII and UWC services
Runs the builder script enabling to choose the UWC recipe needed for the use case. Next it prompts the user to select `develeopment mode` or `production mode` & prepares the set up to run in the selected mode. Finally it does the provisioning of EII & UWC services based on the recipe & mode selected. 
```sh
    $ cd <working-dir>/IEdgeInsights/uwc/build_scripts
    $ sudo ./02_provision_UWC.sh
```
Note: Above example will execute the script in interactive mode. 
The script will run in non-interactive mode when the command line arguments are provided. The help option describes all command line arguments.
```sh
    $ cd <working-dir>/IEdgeInsights/uwc/build_scripts
    $ sudo ./02_provision_UWC.sh --help
```

## Build and Run all EII and UWC services
Builds all the micro-services of the recipe & runs them as containers in background (daemon process).
```sh
  $ cd <working-dir>/IEdgeInsights/uwc/build_scripts
  $ sudo ./03_Build_Run_UWC.sh 
```

## Verify container status
```
Execute below command on terminal to verify container status.
sudo docker ps
```

## Apply configuration changes
If any configuration changes are done to UWC YML files, then run this script which will bring down the containers & bring them up back. Note that the services are not built here.
  ```sh
  $ cd <working-dir>/IEdgeInsights/uwc/build_scripts
  $ 05_applyConfigChanges.sh
  ```

## Uninstallation script
Used to uninstall & remove the UWC installation.
  ```sh
  $ cd <working-dir>/IEdgeInsights/uwc/build_scripts
  $ 04_uninstall_UWC.sh
  ```

  ## Data Persistence feature
  Data Persistence feature in UWC enables the user to save the response JSON received from the end device (or simulator) regarding the data points in database (InfluxDB).
  The response received in JSON format is converted to metrics inside Telegraf micro service before passing the data to InfluxDB for storage. 
  The data can be stored in the database by adding a field called "dataPersist" in the data points YML configuration files (flowmeter_datapoints.yml or iou_datapoints.yml). The possible values for this include "true" or "false" (Boolean values).
  Data is stored in DB if the dataPersist field value is true and not stored if the value is false. Also if the field "dataPersist" is skipped, then  the datapoint is not stored.

  ## Notes
  * Note that any string value that is present in the JSON response payload which is publiched to telegraf needs to have all the string fields listed in the telegraf configuration file for enabling the metrics convertion feature of Telegraf to take effect. 
  
  * The retention period is configurable and can be configured in the config.json file of InfluxDBConnector micro-service -`https://github.com/open-edge-insights/eii-influxdb-connector/blob/master/config.json`.
    The default retention period is set to 24 hours. Although the field "retention" in config.json is set to 23 hours, it includes a default shard duration of 1 hour which totally accounts to 24 hours of data retention.
    Refer influxDB documentation "https://www.influxdata.com/blog/influxdb-shards-retention-policies/" for more details.


## Sample DB Publisher

A sample database publisher which publishes sample JSON data onto the EII MessageBus ZMQ broker. The JSON payload published on ZMQ broker is subscribed by the Telegraf & written to the influx database based on the "dataPersist" flag being true or false in input Json payload. (Writes to InfluxDB if "dataPersist" flag is true and doesn't write to DB if the flag is false). This DB publisher app is containerized & doesn't get involved with any of the UWC services.

Following are the steps for running it.

1. Make sure [EmbPublisher APP](../tools/EmbPublisher/) is present after the repo is cloned.

2. The UWC Embpublisher's config.json and the json input file is present in [eii_configs](./eii_configs/service_specific_cfgs/EmbPublisher/) .

3. User can modify the [UWC_Sample_DB_Publisher_config.json](./eii_configs/service_specific_cfgs/EmbPublisher/UWC_Sample_DB_Publisher_config.json) according to requirement. Just make sure if the string key is to be added in sample json payload then please add the string key in [Telegraf_devmode.conf](./eii_configs/service_specific_cfgs/Telegraf/Telegraf_devmode.conf) or [Telegraf.conf](./eii_configs/service_specific_cfgs/Telegraf/Telegraf.conf) json_string_fields section.

```yaml
<string_key>: <value_key>
```

4. After editing make sure that the [config.json](./eii_configs/service_specific_cfgs/EmbPublisher/config.json) "msg_file" filed is pointing to required file.

```yaml
"msg_file": "UWC_Sample_DB_Publisher_config.json"
```
5. Run the [01_uwc_pre_requisites.sh](./build_scripts/01_uwc_pre_requisites.sh) script.

## Note: 

Please re-run the 01_uwc_pre_requisites.sh script after making any changes to the above mentioned config files.

## Unit Tests
All the UWC modules have unit tests enabled in production mode. In order to run the unit tests, follow the below steps:
```sh
$ cd <working-dir>/IEdgeInsights/uwc/build_scripts
$ sudo ./06_UnitTestRun.sh "false"
```
Now check for the unit test reports for all services of UWC in "<working-dir>/IEdgeInsights/build/unit_test_reports/".

## Debugging steps
```
1. Checking container logs 
   Syntax - sudo docker logs <container_name>
   E.g. To check modbus-tcp-container logs execute "sudo docker logs modbus-tcp-container" command.
2. Command to check logs inside the container "sudo docker exec -it <container_name> bash"
3. Use "cat <log_file_name>" to see log file inside the container
4. Copying logs from container to host machine
   Syntax - docker cp <container_name>:<file to copy from container> <file to be copied i.e. host directory>
5. To check the IP address of machine, use "ifconfig" command.
6. For Modbus RTU, to check attached COM port for serial communication, use "dmesg | grep tty" command.
7. The container logs for each of the UWC micro-service is volume mounted in the directory "/opt/intel/eii/container_logs/", with a separate sub-directory for each micro-service.
8. Also the UWC configuration files can be edited in the sub-directory "/opt/intel/eii/uwc_data", if there is a need to change the configurations.

## Redirect docker logs to file including errors
docker logs modbus-tcp-container > docker.log 2>&1
```

1. ETCD UI is available on `http://localhost:7070/etcdkeeper/` URL for dev mode & `https://localhost:7071/etcdkeeper/` for prod mode . (username - root , password- eii123)
2. EII Message Bus related interface configurations (including secrets) for all UWC & EII containers are stored in ia_etcd server, which can be accessed using EtcdUI as mentioned above.

## Notes
*  If docker-compose.yml is modified then bring down the containers & then bring them up as mentioned above.
*  If previous containers are running on deploy machine, then stop those containers using command for bringing down containers as mentioned above.
*  Can use MQTT.FX or any other MQTT client to verify the flow or all 6 operations for RT/Non-RT, read/write,polling operations

## Troubleshooting
```
1. Follow Method 2 from here https://www.thegeekdiary.com/how-to-configure-docker-to-use-proxy/ to set proxy for docker.
2. In prod mode, the "Certificates" directory in "<working-dir>/IEdgeInsights/build/provision" needs 'sudo su" to be accessed. i.e. to open Certificates folder do the following:
```sh
  $ cd <working-dir>/IEdgeInsights/build/provision
  $ sudo su
  $ cd Certificates
  # After accessing Certificates, enter "exit" command & terminal would return back to normal mode.
  $ exit
```
3. Troubleshooting steps:
1. If KPI-Tactic Application is seen crashing, container restarting or AnalysisKPI.log files not getting generated after building the UWC containers then as a troubleshooting step kindly run the 05_applyConfigChanges.sh, which would bring the containers down & up.
