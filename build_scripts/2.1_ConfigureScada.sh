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
source ./uwc_common_lib.sh
Current_Dir=$(pwd)
eii_build_dir="$Current_Dir/../../build"
RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
MAGENTA=$(tput setaf 5)
NC=$(tput sgr0)

# vars
CA=""
CLIENT_CERT=""
CLIENT_KEY=""
IS_TLS=""
BROKER_HOST=""
BROKER_PORT=""
QOS=""
MODE=""

#------------------------------------------------------------------
# configure_external_certs
#
# Description:
#        This function is used to copy certificates in local directory 
# Return:
#        None
# Usage:
#        configure_external_certs
#------------------------------------------------------------------
configure_external_certs()
{
    echo "${GREEN}Copying required certs for sparkplug-bridge.${NC}"
	cd ${Current_Dir} && rm -rf tmp_certs && mkdir tmp_certs
	mkdir -p ${Current_Dir}/tmp_certs/ca
	mkdir -p ${Current_Dir}/tmp_certs/client_crt
	mkdir -p ${Current_Dir}/tmp_certs/client_key
	cp $CA  ${Current_Dir}/tmp_certs/ca/
	check_for_errors "$?" "Incorrect certificate path is given in CA path...Please provide the correct path and re-run the script" \
                     "${GREEN}"".${NC}"
	cp $CLIENT_CERT  ${Current_Dir}/tmp_certs/client_crt/
	check_for_errors "$?" "Incorrect certificate path is given in Client certificate  path...Please provide the correct path and re-run the script" \
                    "${GREEN}"".${NC}"
	cp $CLIENT_KEY  ${Current_Dir}/tmp_certs/client_key/
	check_for_errors "$?" "Incorrect certificate path is given in Client key certificate  path...Please provide the correct path and re-run the script" \
                    "${GREEN}Certificates are successfully copied in required directory.${NC}"
	echo "${GREEN}Done.${NC}"
	return
}

#------------------------------------------------------------------
# print_all_args
#
# Description:
#        This function is used to print all given values on console 
# Return:
#        None
# Usage:
#        print_all_args
#------------------------------------------------------------------
print_all_args()
{
    echo "${GREEN}==========================================${NC}"
	echo "${GREEN}Given values..${NC}"
	if [ ! -z $IS_TLS ]; then echo "${INFO}--isTLS = $IS_TLS";fi
	if [ ! -z $CA ]; then echo "${INFO}--cafile = $CA";fi
	if [ ! -z $CLIENT_CERT ]; then echo "${INFO}--crtfile = $CLIENT_CERT";fi
	if [ ! -z $CLIENT_KEY ]; then echo "${INFO}--keyFile = $CLIENT_KEY";fi
	if [ ! -z $BROKER_HOST ]; then echo "${INFO}--brokerAddr = $BROKER_HOST";fi
	if [ ! -z $BROKER_PORT ]; then echo "${INFO}--brokerPort = $BROKER_PORT";fi
	if [ ! -z $QOS ]; then echo "${INFO}--qos = $QOS";fi
	echo "${GREEN}==========================================${NC}"
}

#------------------------------------------------------------------
# create_sparkplug-bridge_config file
#
# Description:
#        This function is used create sparkplug-bridge_config.yml file required for sparkplug-bridge container
# Return:
#        None
# Usage:
#       create_sparkplug-bridge_config file
#------------------------------------------------------------------
create_sparkplug-bridge_config_file()
{
rm -rf /opt/intel/eii/uwc_data/sparkplug-bridge/sparkplug-bridge_config.yml
cat > /opt/intel/eii/uwc_data/sparkplug-bridge/sparkplug-bridge_config.yml << ENDOFFILE
---
# sparkplug-bridge config parameter file

isTLS: $IS_TLS
mqttServerAddrSCADA: "$BROKER_HOST"
mqttServerPortSCADA: $BROKER_PORT 
qos: $QOS
ENDOFFILE
}

#------------------------------------------------------------------
# get_user_inputs
#
# Description:
#        This function is used get required config from end user 
# Return:
#        None
# Usage:
#        get_user_inputs
#------------------------------------------------------------------
get_user_inputs()
{
	re='^[0-9]+$'
	echo "${INFO}Enter the follwing parameters required for sparkplug-bridge container..${NC}"
	while :
	do 
		echo "Is TLS required for sparkplug-bridge (yes/no)"
		read TLS
		if [ -z $TLS ];then
			echo "${RED}Empty value entered.${NC}"
		elif [ $TLS != "yes" ] && [ $TLS != "no" ] ;then
			echo "${RED}Invalid value entered. Allowed values (yes/no)${NC}"
		elif [ $TLS == "yes" ];then
			IS_TLS=true
			echo "Enter the CA certificate full path including file name (e.g. <Work_Dir>/IEdgeInsights/build/Certificates/rootca/cacert.pem):"
			read CA 
				if [ -z $CA ]; then 
					echo "${RED}Error:: Empty value entered..${NC}" 
					echo "${RED}Kindly enter correct values and re-run the script..${NC}" 
					exit 1; 
				fi 
			echo "Enter the client certificate full path including file name (e.g. <Work_Dir>/IEdgeInsights/build/Certificates/mymqttcerts/mymqttcerts_client_certificate.pem ):"
			read CLIENT_CERT
				if [ -z $CLIENT_CERT ]; then 
					echo "${RED}Error:: Empty value entered..${NC}" 
					echo "${RED}Kindly enter correct values and re-run the script..${NC}"
					exit 1; 
				fi
			echo "Enter the client key certificate full path including file name (e.g. <Work_Dir>/IEdgeInsights/build/Certificates/mymqttcerts/mymqttcerts_client_key.pem ):"
			read CLIENT_KEY
				if [ -z $CLIENT_KEY ]; then 
					echo "${RED}Error:: Empty value entered..${NC}" 
					echo "${RED}Kindly enter correct values and re-run the script..${NC}" 
					exit 1; 
				fi
			break
		elif [ $TLS == "no" ];then
			IS_TLS=false
			break
		fi
	done

	while :
	do 
		echo "Enter the external broker address/hostname (e.g. 192.168.0.5 or dummyhost.com):"
		read BROKER_HOST
		if [ -z $BROKER_HOST ];then
			echo "${RED}Empty value entered for broker address.${NC}"
		else
			break
		fi
	done

	while :
	do 
		echo "Enter the external broker port number:"
		read BROKER_PORT
		if [ -z $BROKER_PORT ];then
			echo "${RED}Empty value entered for broker port number.${NC}"
		elif ! [[ $BROKER_PORT =~ $re ]];then
			echo "${RED}Not a number.${NC}"
		else
			break
		fi
	done


	while :
	do 
		echo "Enter the QOS for sparkplug (between 0 to 2):"
		read QOS
		if [ -z $QOS ];then
			echo "${RED}Empty value entered for qos.${NC}"
		elif ! [[ $QOS =~ $re ]];then
			echo "${RED}Not a number.${NC}"
		elif [ $QOS -lt 0 ] || [ $QOS -gt 2 ];then
			echo "${RED}Invalid value entered for QOS. Allowed values 0 - 2${NC}"
		else
			break
		fi
	done
	print_all_args
}

#------------------------------------------------------------------
# parse_command_line_args
#
# Description:
#        This function is used to Parse command line arguments passed to this script
# Return:
#        None
# Usage:
#        parse_command_line_args <list of arguments>
#------------------------------------------------------------------
parse_command_line_args()
{       
    re='^[0-9]+$'
    echo "${INFO}Reading the command line args of 2.1_ConfigureScada.sh ...${NC}"
    for ARGUMENT in "$@"
    do
        KEY=$(echo $ARGUMENT | cut -f1 -d=)
        VALUE=$(echo $ARGUMENT | cut -f2 -d=)   

        echo ${GREEN}$KEY "=" $VALUE${NC}
        echo "${GREEN}==========================================${NC}"

        case "$KEY" in
        	--deployMode)   MODE=${VALUE} ;;
            --isTLS)        IS_TLS=${VALUE} ;;
            --caFile)       CA=${VALUE} ;;
            --crtFile)      CLIENT_CERT=${VALUE} ;;
            --keyFile)      CLIENT_KEY=${VALUE} ;;
            --brokerAddr)   BROKER_HOST=${VALUE};;
            --brokerPort)   BROKER_PORT=${VALUE};;
            --qos)          QOS=${VALUE};; 
        esac    
    done    
    if [ "$IS_TLS" == "1" ] || [ "$IS_TLS" == "true" ] ||  [ "$IS_TLS" == "yes" ] ;then
   		if [ -z "$CA" ]; then 
   			echo "Enter the CA certificate full path including file name (e.g. <Work_Dir>/IEdgeInsights/build/Certificates/rootca/cacert.pem):"
			read CA
			if [ -z "$CA" ]; then 
				echo "${RED}Error:: Empty value entered..${NC}" 
				echo "${RED}Kindly enter correct values and re-run the script..${NC}" 
				exit 1 
			fi 
   		fi

   		if [ -z "$CLIENT_CERT" ]; then 
   			echo "Enter the client certificate full path including file name (e.g. <Work_Dir>/IEdgeInsights/build/Certificates/mymqttcerts/mymqttcerts_client_certificate.pem):"
			read CLIENT_CERT   
			if [ -z "$CLIENT_CERT" ]; then 
				echo "${RED}Error:: Empty value entered..${NC}"; 
				echo "${RED}Kindly enter correct values and re-run the script..${NC}";
				exit 1 
			fi
   		fi

   		if [ -z "$CLIENT_KEY" ]; then 
   			echo "Enter the client key certificate full path including file name (e.g. <Work_Dir>/IEdgeInsights/build/Certificates/mymqttcerts/mymqttcerts_client_key.pem):"
			read CLIENT_KEY
   			if [ -z "$CLIENT_KEY" ]; then 
   				echo "${RED}Error:: Empty value entered..${NC}" 
   				echo "${RED}Kindly enter correct values and re-run the script..${NC}" 
   				exit 1 
   			fi
   		fi
    else 
        if [ ! -z "$CA" ] || [ ! -z "$CLIENT_CERT" ] || [ ! -z "$CLIENT_KEY" ]; then 
            echo "${RED}Certs not required when TLS Mode is disabled. Please see Usage.${NC}"
            exit 1
        fi
    fi

    if [ -z "${QOS}" ];then
		echo "${RED}Empty value entered for qos.${NC}"
		echo "Enter the QOS for sparkplug (between 0 to 2):"
		read QOS
		if [ -z "$QOS" ]; then 
			echo "${RED}Error:: Empty value entered..${NC}"; 
			echo "${RED}Kindly enter correct values and re-run the script..${NC}"; 
			exit 1; 
		fi 
	fi

	if ! [[ "${QOS}" =~ $re ]];then
		echo "${RED}Invalid QOS. Not a number.${NC}"
		exit 1
	fi

	if [[ "$QOS" -lt 0 ]] || [[ "$QOS" -gt 2 ]];then
		echo "${RED}Invalid value entered for QOS. Allowed values 0 - 2${NC}"
		exit 1		
	fi 

    if [ -z "$BROKER_HOST" ];then
		echo "${RED}Empty value entered for broker address.${NC}"
		echo "Enter the external broker address/hostname (e.g. 192.168.0.5 or dummyhost.com):"
		read BROKER_HOST 
		if [ -z "$BROKER_HOST" ]; then 
			echo "${RED}Error:: Empty value entered..${NC}" 
			echo "${RED}Kindly enter correct values and re-run the script..${NC}"
			exit 1 
		fi 		
	fi

	if [ -z "$BROKER_PORT" ];then
		echo "${RED}Empty value entered for broker port number.${NC}"
		echo "Enter the external broker port number:"
		read BROKER_PORT 
		if [ -z "$BROKER_HOST" ]; then 
			echo "${RED}Error:: Empty value entered..${NC}" 
			echo "${RED}Kindly enter correct values and re-run the script..${NC}"
			exit 1
		fi 
	fi
	
	if ! [[ "$BROKER_PORT" =~ $re ]];then
		echo "${RED}Invalid BROKER_PORT. Not a number.${NC}"
		exit 1
    fi

}

echo "${GREEN}============================= Script 2.1 START ============================================${NC}"
if [ -z "$1" ]; then
	echo "Interactive mode"
	get_user_inputs	
elif [ "$1" == "--deployModeInteract=dev" ]; then 
	echo "Deployment Mode is dev in Interactive user input"
	MODE=dev
	get_user_inputs
else
	echo "Non Interactive mode"
	parse_command_line_args "$@"
fi
if [ "$IS_TLS" == "1" ] || [ "$IS_TLS" == "true" ] ||  [ "$IS_TLS" == "yes" ] ;then
	configure_external_certs
else
	echo "Cert configuration is not required.."
fi
create_sparkplug-bridge_config_file
cd "${Current_Dir}"
echo "${GREEN}============================= Script 2.1 END ============================================${NC}"
exit 0
