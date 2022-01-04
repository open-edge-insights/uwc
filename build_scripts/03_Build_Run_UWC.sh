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
resume_insights="${HOME}/.resume_process"

eii_build_dir="$Current_Dir/../../build"
source uwc_common_lib.sh

#------------------------------------------------------------------
# build_run_UWC
#
# Description:
#        This fun will install all UWC containers
# Return:
#        None
# Usage:
#        build_run_UWC
#-----------------------------------------------------------------
build_run_UWC()
{
    cd "${eii_build_dir}"
    if [ -e ./setenv ]; then
        source ./setenv
    fi 
    if [[ $preBuild == "true" ]]; then
        echo "Using pre-build images for building"
	# Checking if ia_telegraf is part of the recipe use case selected.
        is_ia_telegraf_present=`grep "ia_telegraf" "docker-compose-build.yml"`
        if [ ! -z "$is_ia_telegraf_present" ]; then
	# Checking if emb_publisher is part of the recipe use case selected
            is_emb_publisher_present=`grep "emb_publisher" "docker-compose-build.yml"`
            if [ ! -z "$is_emb_publisher_present" ]; then
                docker-compose -f docker-compose-build.yml build ia_eiibase ia_common ia_telegraf emb_publisher 
                if [ "$?" -eq "0" ];then
	            echo "*****************************************************************"
                    echo "${GREEN}UWC containers built successfully.${NC}"
                else
                    echo "${RED}Failed to built  UWC containers.${NC}"
	            echo "*****************************************************************"
                    exit 1
                fi
            else 
                docker-compose -f docker-compose-build.yml build ia_eiibase ia_common ia_telegraf 
                if [ "$?" -eq "0" ];then
	            echo "*****************************************************************"
                    echo "${GREEN}UWC containers built successfully.${NC}"
                else
                    echo "${RED}Failed to built  UWC containers.${NC}"
	            echo "*****************************************************************"
                    exit 1
                fi                    
            fi  
        fi     
     
        docker-compose up -d
        if [ "$?" -eq "0" ];then
            echo "*****************************************************************"
            echo "${GREEN}Installed UWC containers successfully.${NC}"
        else
            echo "${RED}Failed to install UWC containers.${NC}"
            echo "*****************************************************************"
            exit 1
        fi         
    else 
        echo "Building images locally"
        docker-compose -f docker-compose-build.yml build 
        if [ "$?" -eq "0" ];then
	    echo "*****************************************************************"
            echo "${GREEN}UWC containers built successfully.${NC}"
        else
            echo "${RED}Failed to built  UWC containers.${NC}"
	    echo "*****************************************************************"
            exit 1
        fi
        docker-compose up -d ia_configmgr_agent
        sleep 10
        cd "${Current_Dir}"
        if [[ "$deployMode" == "prod" ]]; then
            mqtt_certs
        fi
        cd "${eii_build_dir}"
        docker-compose up -d

        if [ "$?" -eq "0" ];then
            echo "*****************************************************************"
            echo "${GREEN}Installed UWC containers successfully.${NC}"
        else
            echo "${RED}Failed to install UWC containers.${NC}"
            echo "*****************************************************************"
            exit 1
        fi
    fi
    return 0
}

verify_container()
{
    cd "${eii_build_dir}"
    echo "*****************************************************************"
    echo "${GREEN}Below is the containers deployed status.${NC}"
    echo "*****************************************************************"
    docker ps
    return 0
}

function harden()
{
	docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 mqtt-bridge
	docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 modbus-tcp-master
	docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 mqtt_container
	docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 modbus-rtu-master
	#docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 ia_etcd
	#docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 ia_etcd_provision
	docker ps -q --filter "name=sparkplug-bridge" | grep -q . && docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 sparkplug-bridge
	# Increase pid limit for KPI to 500 for processing larger count of threads.
	docker ps -q --filter "name=kpi-tactic-app" | grep -q . && docker container update --pids-limit=500 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 kpi-tactic-app
}
mqtt_certs()
{
    echo "${GREEN} Genrating certs for mqtt${NC}"	
    mkdir ./temp ./temp/client ./temp/server
    
    openssl req -config ../Others/mqtt_certs/openssl.cnf -new -newkey rsa:3072 -keyout  ./temp/client/key.pem -out ./temp/client/req.pem -days 3650 -outform PEM -subj /CN=mymqttcerts/O=client/L=$$$/ -nodes

    openssl req -config ../Others/mqtt_certs/openssl.cnf -new -newkey rsa:3072 -keyout  ./temp/server/key.pem -out ./temp/server/req.pem -days 3650 -outform PEM -subj /CN=mymqttcerts/O=server/L=$$$/ -nodes

    openssl ca -days 3650 -cert ../../build/Certificates/rootca/cacert.pem -keyfile ../../build/Certificates/rootca/cakey.pem -in ./temp/server/req.pem -out ./temp/mymqttcerts_server_certificate.pem  -outdir ../../build/Certificates/rootca/certs -notext -batch -extensions server_extensions -config ../Others/mqtt_certs/openssl.cnf

    openssl ca -days 3650 -cert ../../build/Certificates/rootca/cacert.pem -keyfile ../../build/Certificates/rootca/cakey.pem -in ./temp/client/req.pem -out ./temp/mymqttcerts_client_certificate.pem -outdir ../../build/Certificates/rootca/certs -notext -batch -extensions client_extensions -config ../Others/mqtt_certs/openssl.cnf
    if [ -e ../../build/Certificates/mymqttcerts ];then
        rm -r ../../build/Certificates/mymqttcerts
        
    fi
    mkdir ../../build/Certificates/mymqttcerts
    cp -rf ./temp/mymqttcerts_server_certificate.pem ../../build/Certificates/mymqttcerts/mymqttcerts_server_certificate.pem
    cp -rf ./temp/mymqttcerts_client_certificate.pem ../../build/Certificates/mymqttcerts/mymqttcerts_client_certificate.pem
    cp -rf ./temp/server/key.pem  ../../build/Certificates/mymqttcerts/mymqttcerts_server_key.pem
    cp -rf ./temp/client/key.pem ../../build/Certificates/mymqttcerts/mymqttcerts_client_key.pem
    sudo chown -R 1999:1999 ../../build/Certificates/mymqttcerts/ 
    rm -rf ./temp
    return 0
}
function main()
{
    echo "${INFO}Deployment started${NC}"
    STARTTIME=$(date +%s)
    check_root_user
    check_internet_connection
    docker_verify
    docker_compose_verify
    build_run_UWC
    verify_container
    harden
    echo "${INFO}Deployment Completed${NC}"
    ENDTIME=$(date +%s)
    ELAPSEDTIME=$(( ${ENDTIME} - ${STARTTIME} ))
    echo "${GREEN}Total Elapsed time is : $(( ${ELAPSEDTIME} / 60 )) minutes ${NC}"
}

export DOCKER_CONTENT_TRUST=1
export DOCKER_BUILDKIT=1
main

cd "${Current_Dir}"
exit 0
