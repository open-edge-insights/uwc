################################################################################
# The source code contained or described herein and all documents related to
# the source code ("Material") are owned by Intel Corporation. Title to the
# Material remains with Intel Corporation.
#
# No license under any patent, copyright, trade secret or other intellectual
# property right is granted to or conferred upon you by disclosure or delivery of
# the Materials, either expressly, by implication, inducement, estoppel or otherwise.
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Src/utils/YamlUtil.cpp 

OBJS += \
./Src/utils/YamlUtil.o 

CPP_DEPS += \
./Src/utils/YamlUtil.d 


# Each subdirectory must supply rules for building sources it contributes
Src/utils/%.o: ../Src/utils/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++11 -I../$(PROJECT_DIR)/include -I../$(PROJECT_DIR)/../bin/yaml-cpp/include -I/usr/local/include -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

