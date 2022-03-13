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

#------------------------------------------------------------------
# copy_external_certs_to_provision
#
# Description:
#        Copy external certificates to common directoty required for sparkplug-bridge container
# Return:
#        None
# Usage:
#        copy_external_certs_to_provision
#------------------------------------------------------------------
copy_external_certs_to_provision()
{
	if [ -d ${Current_Dir}/tmp_certs ]; then
        echo "${GREEN}Copying required certs for sparkplug-bridge.${NC}"
		cd ${Current_Dir}
		rm -rf ${eii_build_dir}/Certificates/scada_ext_certs
		mkdir -p ${eii_build_dir}/Certificates/scada_ext_certs
		cp ${Current_Dir}/tmp_certs/ca/*  ${eii_build_dir}/Certificates/scada_ext_certs/cacert.pem
		check_for_errors "$?" "Incorrect certificate path is given in command line...Please provide the required command line arguments and re-run the script" \
		            "${GREEN}"".${NC}"
		cp ${Current_Dir}/tmp_certs/client_crt/*  ${eii_build_dir}/Certificates/scada_ext_certs/mymqttcerts_client_certificate.pem
		check_for_errors "$?" "Incorrect certificate path is given in command line...Please provide the required command line arguments and re-run the script" \
		            "${GREEN}"".${NC}"
		cp ${Current_Dir}/tmp_certs/client_key/*  ${eii_build_dir}/Certificates/scada_ext_certs/mymqttcerts_client_key.pem
		check_for_errors "$?" "Incorrect certificate path is given in command line...Please provide the required command line arguments and re-run the script" \
		            "${GREEN}Certificates are successfully copied in required directory.${NC}"
		chown -R 1999:1999 ${eii_build_dir}/Certificates/scada_ext_certs
		echo "${GREEN}Done.${NC}"		
		rm -rf ${Current_Dir}/tmp_certs
		return 0
	else
		echo "${GREEN}No need to configure certificates for sparkplug-bridge container${NC}"
		echo "${INFO}******************* Script END *****************************${NC}"
    	fi	
}

echo "${GREEN}============================= Script 2.2 START ============================================${NC}"
copy_external_certs_to_provision
echo "${GREEN}============================= Script 2.2 END ============================================${NC}"
exit 0
