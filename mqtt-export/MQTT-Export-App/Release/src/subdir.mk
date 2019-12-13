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
../src/ConfigManager.cpp \
../src/MQTTCallback.cpp \
../src/MQTTHandler.cpp \
../src/MQTT_Export.cpp \
../src/TopicMapper.cpp \
../src/EISMsgbusHandler.cpp 

OBJS += \
./src/ConfigManager.o \
./src/MQTTCallback.o \
./src/MQTTHandler.o \
./src/MQTT_Export.o \
./src/TopicMapper.o \
./src/EISMsgbusHandler.o 

CPP_DEPS += \
./src/ConfigManager.d \
./src/MQTTCallback.d \
./src/MQTTHandler.d \
./src/MQTT_Export.d \
./src/TopicMapper.d \
./src/EISMsgbusHandler.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -lrt -std=c++11 -fpermissive -I../$(PROJECT_DIR)/include -I/usr/local/include -O3 -Wall -c -fmessage-length=0 -fPIE -O2 -D_FORTIFY_SOURCE=2 -static -fvisibility=hidden -fvisibility-inlines-hidden -Wformat -Wformat-security -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

