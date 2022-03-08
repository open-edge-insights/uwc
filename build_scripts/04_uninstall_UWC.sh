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

Current_Dir=$(pwd)/../../build/
RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
MAGENTA=$(tput setaf 5)
NC=$(tput sgr0)
SUCCESS='\033[0;32m'
INFO='\033[0;34m'

DOCKER_REPO="deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"
DOCKER_GPG_KEY="0EBFCD88"
EII_USER_NAME=eiiuser
EII_UID=5319
EII_INSTALL_PATH=/opt/intel/eii

source uwc_common_lib.sh

clean_exit()
{
    echo -e "${ERROR}------------------------------Uninstallation failed. Exiting--------------------------------${NC}"
    exit 1
}

del_file() 
{
    if [[ -f $1 ]]; then
        rm -rf $1
        if [[ $? -ne 0 ]]; then
            clean_exit
        fi
    fi 
}

del_dir()
{
    if [[ -d $1 ]]; then
        rm -rf $1
        if [[ $? -ne 0 ]]; then
            clean_exit
        fi
    fi
}

function uninstall_docker_compose()
{
    	# UNINSTALLING DOCKER-COMPOSE
    	echo -e "${INFO}--------------------------------------Uninstalling Docker-compose--------------------------------${NC}"
    	rm -rf $(which docker-compose)

	    pip3 show docker-compose 1>/dev/null
	    if [ $? == 0 ]; then
		    pip3 uninstall -y docker-compose >>/dev/null
	    else
	        echo "" 
	    fi

        #pip3 uninstall -y docker-compose >>/dev/null
        echo -e "${SUCCESS}---------------------------Docker-compose uninstalled successfully-------------------------------${NC}"   
}

function uninstall_docker()
{
    # UNINSTALLING DOCKER 
    echo -e "${INFO}---------------------------------------Uninstalling Docker---------------------------------------${NC}"
    
    dpkg --purge --force-all docker-ce docker-ce-cli containerd.io
    apt-get purge -y docker docker.io
    
    # Removing Docker GPG and removing the repository from sources
    apt-key del $DOCKER_GPG_KEY
    add-apt-repository --remove "$DOCKER_REPO"
    echo -e "${SUCCESS}-------------------------------Docker uninstalled successfully-----------------------------------${NC}" 
}
 
function removeUWCConfig
{
    #Delete UWC config
    echo -e "${INFO}---------------------------------------Removing UWC config-----------------------${NC}"
    del_dir /opt/intel/eii/uwc_data
    del_dir /opt/intel/eii/uwc_data/common_config
    del_dir /opt/intel/eii/container_logs
    echo -e "${SUCCESS}-------------------------------Removed all UWC config------------------------------------${NC}"
    
    #RESET THE PROXY SETTING 
    echo -e "${INFO}---------------------------------------Resetting proxy setting-----------------------------------${NC}"
    del_file /etc/docker/daemon.json
    del_file /etc/systemd/system/docker.service.d/http-proxy.conf
    del_file /etc/systemd/system/docker.service.d/https-proxy.conf      
    del_file $HOME/.docker/config.json
    del_file /etc/systemd/system/docker.service.d
    echo -e "${SUCCESS}-------------------------------Proxy setting reset to default------------------------------------${NC}"
}

uninstall_containers()
{
    echo -e "${INFO}Bringing down all running containers from this machine${NC}"
    docker-compose down
    echo -e "${INFO}Removing all containers from this machine${NC}"
	docker rm -f $(docker ps -a -q)
	docker system prune -af --volumes
    
	if [ "$?" -ne "0" ];then
		echo "${RED}Failed to uninstall UWC contatiners.${NC}"
		exit 1
	else
		echo "${GREEN}Removed all the UWC containers.${NC}"
	fi
	return 0
}

delete_file()
{
    echo -e "${INFO}---------------------------------------Deleting setenv file-----------------------${NC}"
    del_file ./setenv
}

cd ${Current_Dir}
check_root_user
uninstall_containers
uninstall_docker
uninstall_docker_compose
removeUWCConfig
delete_file

echo ""
echo -e "${GREEN}---------------------------------------Uninstallation Completed-----------------------------------${NC}"
