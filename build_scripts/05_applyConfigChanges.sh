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
	
source uwc_common_lib.sh	

apply_config_changes()
{
	cd ${Current_Dir}
	echo "${GREEN}Stopping all the runninng containers to apply new changes.${NC}" 
	docker-compose down
	if [ "$?" -eq "0" ];then
		echo "${GREEN} Successfully stopped all running containers.${NC}"
	else
		echo "${RED}Failed to stop running containers.${NC}"
	fi
        docker-compose up -d ia_configmgr_agent
        sleep 10	
	docker-compose up -d
	if [ "$?" -eq "0" ];then
		echo "${GREEN}Successfully started all running containers with updated changes.${NC}"
	else
		echo "${RED}Failed to start running containers.${NC}"
	fi
}

function harden()
{
	docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 mqtt-bridge
	docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 modbus-tcp-master
	docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 mqtt_container
	docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 modbus-rtu-master
	docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 ia_etcd
	docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 ia_etcd_provision
	docker ps -q --filter "name=sparkplug-bridge" | grep -q . && docker container update --pids-limit=100 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 sparkplug-bridge
	# Increase the pid limit of KPI to 500 to process larger number of threads.
	docker ps -q --filter "name=kpi-tactic-app" | grep -q . && docker container update --pids-limit=500 --restart=on-failure:5 --cpu-shares 512 -m 1G --memory-swap -1 kpi-tactic-app
}

export DOCKER_CONTENT_TRUST=1
export DOCKER_BUILDKIT=1
check_root_user
apply_config_changes
harden
