#!/bin/bash
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

Current_Dir=$(pwd)
RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
MAGENTA=$(tput setaf 5)
NC=$(tput sgr0)

eii_build_dir="$Current_Dir/../../build"

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT

source uwc_common_lib.sh

function ctrl_c()
{
        echo "** Trapped CTRL-C"
		echo "cleanup..."
		rm -rf $Current_Dir/temp1
		echo "Exiting the script..."
		exit 0
}

#------------------------------------------------------------------
# modifying_env
#
# Description:
#        Adding UWC env variables.
# Return:
#        None
# 
#------------------------------------------------------------------

modifying_env()
{
	echo "Addding UWC specific env variables"
	result=`grep -Fi 'env for UWC' ../../build/.env`
	if [ -z "${result}" ]; then
	    cat ../.env >> ../../build/.env
       
	fi
    result=`grep -F 'uwc/'  ../../.gitignore`
	if [ -z "${result}" ]; then
	    sed -i '$a uwc/' ../../.gitignore
	fi	
  
}

set_dev_mode()
{
   if [ "$1" == "true" ]; then
	   echo "${INFO}Setting dev mode to true ${NC}"    
	   sed -i 's/DEV_MODE=false/DEV_MODE=true/g' $eii_build_dir/.env
	   if [ "$?" -ne "0" ]; then
			echo "${RED}Failed to set dev mode."
			echo "${GREEN}Kinldy set DEV_MODE to false manualy in .env file and then re--run this script"
			exit 1
	   else
			echo "${GREEN}Dev Mode is set to true ${NC}"
	   fi
	else
		echo "${INFO}Setting dev mode to false ${NC}"    
		sed -i 's/DEV_MODE=true/DEV_MODE=false/g' $eii_build_dir/.env
		if [ "$?" -ne "0" ]; then
			echo "${RED}Failed to set dev mode."
			echo "${GREEN}Kinldy set DEV_MODE to false manualy in .env file and then re--run this script"
			exit 1
	   else
			echo "${GREEN}Dev Mode is set to false ${NC}"
	   fi
	fi
	sed -i 's/MQTT_PROTOCOL=ssl/MQTT_PROTOCOL=tcp/g' $eii_build_dir/.env
}

#------------------------------------------------------------------
# generate_unit_test_report
#
# Description:
#        the Edge Insights Software is installed as a systemd service so that it comes up automatically
#        on machine boot and starts the analytics process.
# Return:
#        None
# Usage:
#        generate_unit_test_report
#-----------------------------------------------------------------
generate_unit_test_report()
{
    cd "${eii_build_dir}"
    docker-compose -f docker-compose-build.yml build 
    if [ "$?" -eq "0" ];then
	echo "*****************************************************************"
        echo "${GREEN}Unit test containers built successfully.${NC}"
    else
        echo "${RED}Building unit test containers failed.${NC}"
	echo "*****************************************************************"
        exit 1
    fi
    docker-compose up -d
    if [ "$?" -eq "0" ];then
        echo "*****************************************************************"
        echo "${GREEN}Successfully deployed unit test containers.${NC}"
    else
        echo "${RED}Installation of unit test conatiners is failed.${NC}"
        echo "*****************************************************************"
        exit 1
    fi    
    return 0
}

create_test_dir()
{
	cd "${eii_build_dir}"
	rm -rf unit_test_reports
	mkdir -p unit_test_reports/modbus-tcp-master
	mkdir -p unit_test_reports/modbus-rtu-master
	mkdir -p unit_test_reports/mqtt-bridge
	mkdir -p unit_test_reports/sparkplug-bridge
	mkdir -p unit_test_reports/kpi-tactic
	mkdir -p unit_test_reports/uwc-util
	chown -R $SUDO_USER:$SUDO_USER unit_test_reports
	chmod -R 777 unit_test_reports
	cd "${eii_build_dir}"
}

function cleanup()
{
	cd $Current_Dir
	chown -R $SUDO_USER:$SUDO_USER ${eii_build_dir}/unit_test_reports
	docker stop $(docker ps -a -q)
	sed -i 's/MQTT_PROTOCOL=tcp/MQTT_PROTOCOL=ssl/g' $eii_build_dir/.env
}

mqtt_certs()
{
    echo "${GREEN} Genrating certs for mqtt${NC}"	
    mkdir ./temp ./temp/client ./temp/server

    openssl req -config ../../uwc/Others/mqtt_certs/mqtt_cert_openssl_config -new -newkey rsa:3072 -keyout  ./temp/client/key.pem -out ./temp/client/req.pem -days 3650 -outform PEM -subj /CN=mymqttcerts/O=client/L=$$$/ -nodes

    openssl req -config ../../uwc/Others/mqtt_certs/mqtt_cert_openssl_config -new -newkey rsa:3072 -keyout  ./temp/server/key.pem -out ./temp/server/req.pem -days 3650 -outform PEM -subj /CN=mymqttcerts/O=server/L=$$$/ -nodes

    openssl ca -days 3650 -cert ../../build/provision/rootca/cacert.pem -keyfile ../../build/provision/rootca/cakey.pem -in ./temp/server/req.pem -out ./temp/mymqttcerts_server_certificate.pem  -outdir ../../build/provision/rootca/certs -notext -batch -extensions server_extensions -config ../../uwc/Others/mqtt_certs/mqtt_cert_openssl_config

    openssl ca -days 3650 -cert ../../build/provision/rootca/cacert.pem -keyfile ../../build/provision/rootca/cakey.pem -in ./temp/client/req.pem -out ./temp/mymqttcerts_client_certificate.pem -outdir ../../build/provision/rootca/certs -notext -batch -extensions client_extensions -config ../../uwc/Others/mqtt_certs/mqtt_cert_openssl_config

    mkdir ../../build/provision/Certificates/mymqttcerts
    cp -rf ./temp/mymqttcerts_server_certificate.pem ../../build/provision/Certificates/mymqttcerts/mymqttcerts_server_certificate.pem
    cp -rf ./temp/mymqttcerts_client_certificate.pem ../../build/provision/Certificates/mymqttcerts/mymqttcerts_client_certificate.pem
    cp -rf ./temp/server/key.pem  ../../build/provision/Certificates/mymqttcerts/mymqttcerts_server_key.pem
    cp -rf ./temp/client/key.pem ../../build/provision/Certificates/mymqttcerts/mymqttcerts_client_key.pem
   
    sudo chown -R eiiuser:eiiuser ../../build/provision/Certificates/mymqttcerts/ 
    rm -rf ./temp
    return 0
}

#------------------------------------------------------------------
# eii_provision
#
# Description:
#        Performs prvovisioning as per docker-compose.yml file.
# Return:
#        None
# Usage:
#       eii_provision
#------------------------------------------------------------------
eii_provision()
{
	docker stop $(docker ps -a -q)
    if [ -d "${eii_build_dir}/provision/" ];then
        cp -f ${eii_build_dir}/../uwc/docker-compose_UT.yml ${eii_build_dir}/docker-compose.yml
        cp -f ${eii_build_dir}/../uwc/eii_config_UT.json ${eii_build_dir}/provision/config/eii_config.json
        cd "${eii_build_dir}/provision/"
    else
        echo "${RED}ERROR: ${eii_build_dir}/provision/ is not present.${NC}"
        exit 1 # terminate and indicate error
    fi

    ./provision.sh ../docker-compose.yml 
    check_for_errors "$?" "Provisioning is failed. Please check logs" \
                    "${GREEN}Provisioning is done successfully.${NC}"
    echo "${GREEN}>>>>>${NC}"
    echo "${GREEN}For building & running UWC services,run 03_Build_Run_UWC.sh script${NC}"

    return 0
    echo "${GREEN}>>>>>${NC}"
    echo "${GREEN}For building & running UWC services,run 03_Build_Run_UWC.sh script${NC}"
    return 0
}

#------------------------------------------------------------------
# create_sparkplug-bridge_config_file_ut
#
# Description:
#        This function is used create sparkplug-bridge_config.yml file required for sparkplug-bridge container for unit testing
# Return:
#        None
# Usage:
#       create_sparkplug-bridge_config_file_ut
#------------------------------------------------------------------
create_sparkplug-bridge_config_file_ut()
{
rm -rf /opt/intel/eii/uwc_data/sparkplug-bridge/sparkplug-bridge_config.yml
cat > /opt/intel/eii/uwc_data/sparkplug-bridge/sparkplug-bridge_config.yml << ENDOFFILE
---
# sparkplug-bridge config parameter file

isTLS: false
mqttServerAddrSCADA: "127.0.0.1"
mqttServerPortSCADA: 11883 
qos: 1
ENDOFFILE
}


export DOCKER_CONTENT_TRUST=1
export DOCKER_BUILDKIT=1
check_root_user
check_internet_connection
modifying_env
docker_verify
docker_compose_verify
create_test_dir
set_dev_mode "false"
eii_provision
mqtt_certs
create_sparkplug-bridge_config_file_ut
generate_unit_test_report

echo "Generating code coverage report..."
echo "It may take few minutes ..."
### To sleep for 2 min: ##
sleep 90s
cleanup
exit 0
