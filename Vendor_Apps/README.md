**Contents**

- [About tool.](#about-tool)
- [How to integrate this tool with use case.](#how-to-integrate-this-tool-with-use-case)
- [Sample Datafiles.](#sample-datafiles)
- [Configuration of the tool to be used in EtcdUi.](#configuration-of-the-tool-to-be-used-in-etcdui)
- [Running EMBPublisher](#running-embpublisher)

# About tool.
- This is a EMB publisher and Subscriber where the publish and subscribe happens on the EMB topic (eg: RT/read/flowmeter/PL0/D1 or RT/write/flowmeter/PL0/D1).
- EmbPublisher acts as a sample publisher to EII messagebus (Brokered).
- EmbSubscriber acts as a sample subscriber to the EII Messagebus (brokered).
- EmbPublisher and EmbSubscriber is used for 3 scenarios:

   a) Normal VA scenario - where the publisher topics are "BIRTH", "DATA", "DEATH". (eg: BIRTH/UWC_1/COM1,DATA/UWC_1/COM1,DEATH/UWC_1)

   b) TemplateDef VA scenario - where the publisher topic is "TemplateDef".(eg: TemplateDef )

   c) Sample publisher and subscriber. (New topic format) - where the publisher topic are "<RT|NRT>/<read|write>/<device>/<wellhead>/<data_point>"(eg: RT/read/flowmeter/PL0/D1 or RT/write/flowmeter/PL0/D1).

# How to integrate this tool with use case.
- In any of the recipe file of uwc, please add 'uwc/Vendor_Apps/EmbPublisher' and 'uwc/Vendor_Apps/EmbSubscriber' components.
- EmbPublisher and EmbSubscriber components are added to all recipes which has sparkplug in it i.e usecase number 3,4,7, 9 and 10 by default.
- Follow usual provisioning and building process by referring [UWC README](../README.md).

# Sample Datafiles.
Datafiles directory contains sample JSON payloads in three separate JSON files which are as below.

Any change needed in JSON in payload can be updated in below files.

VA_Sample_Data.json - This JSON file contains the sample data for vendor app scenario of sparkplug.

TemplateDef.json - This JSON file contains the sample data for TemplateDef scenario of sparkplug.

Publisher_content.json - This JSON file contains the sample data for read on demand and write on demand.

# Configuration of the tool to be used in EtcdUi.
Let us look at the sample configuration of EmbPublisher for "Sample publisher and subscriber" scenario.
```
{
  "config": {
    "cert_type": ["zmq"],
    "pub_name": "VA_Sample_Publisher",
    "msg_file": "Publisher_content.json",
    "publisher_topic": "RT/read/flowmeter/PL0/D1",
    "data_toggle": 1
  },
  "interfaces": {
    "Publishers": [
      {
        "AllowedClients": [
           "*"
        ],
       "EndPoint": {
          "SocketDir": "/EII/sockets",
          "SocketFile": "frontend-socket"
        },
       "Name": "VA_Sample_Publisher",
       "Topics": [
          "*"
        ],
       "Type": "zmq_ipc",
       "BrokerAppName": "ZmqBroker",
       "brokered": true
      }
    ]
  }
}

```
Sample configuration of EmbPublisher for "TemplateDef VA scenario" scenario
```
{
  "config": {
    "cert_type": ["zmq"],
    "pub_name": "VA_Sample_Publisher",
    "msg_file": "TemplateDef.json",
    "publisher_topic": "TemplateDef",
    "data_toggle": 1
  },
  "interfaces": {
    "Publishers": [
      {
        "AllowedClients": [
           "*"
        ],
       "EndPoint": {
          "SocketDir": "/EII/sockets",
          "SocketFile": "frontend-socket"
        },
       "Name": "VA_Sample_Publisher",
       "Topics": [
          "*"
        ],
       "Type": "zmq_ipc",
       "BrokerAppName": "ZmqBroker",
       "brokered": true
      }
    ]
  }
}

```

Sample configuration of EmbPublisher for "Normal VA scenario" scenario
```
{
  "config": {
    "cert_type": ["zmq"],
    "pub_name": "VA_Sample_Publisher",
    "msg_file": "VA_Sample_Data.json",
    "publisher_topic": "BIRTH/UWC_1/COM1",
    "data_toggle": 1
  },
  "interfaces": {
    "Publishers": [
      {
        "AllowedClients": [
           "*"
        ],
       "EndPoint": {
          "SocketDir": "/EII/sockets",
          "SocketFile": "frontend-socket"
        },
       "Name": "VA_Sample_Publisher",
       "Topics": [
          "*"
        ],
       "Type": "zmq_ipc",
       "BrokerAppName": "ZmqBroker",
       "brokered": true
      }
    ]
  }
}

```

- -pub_name : The name of the publisher in the interface.
- -msg_file : The sample JSON payload to be used for publishing
- -data_toggle: Change the value of data_toggle (to 0 or 1 or vice-versa) when any content of JSON payload in datafile's directory is changed.
- -publisher_topic: Topic on which the payload is to be published.

# Running EMBPublisher 

Follow the below steps to publish a JSON payload using EMBPublisher :

1) Follow usual provisioning and building process by referring [UWC README](../README.md).

2) Please check that [EtcdUI(i.e openedgeinsights/ia_etcd_ui)](../../EtcdUI/README.md) container is up and running.

     a) Open your browser and enter the address: https://< host ip >:7071/etcdkeeper/ (when EII is running in secure mode). In this case, CA cert has to be imported in the browser. For insecure mode i.e. DEV mode, it can be accessed at http://< host ip >:7070/etcdkeeper/.

     b) Username is 'root' and default password is located at ETCD_ROOT_PASSWORD key under environment section in build/provision/dep/docker-compose-provision.override.prod.yml.

3) To check the response msg , please check the logs of EmbSubscriber i.e docker logs -f ia_emb_subscriber

4) During runtime if add/change in payload is required then go to EtcdUI webpage then /EmbPublisher/config section and change "publisher_topic" field.

5) During runtime if add/change in payload is required then go to EtcdUI webpage then /EmbPublisher/config section and change "msg_file" field with the datafile containing the required payload.

6) During runtime if add/change in payload is required then open the datafile present in ./EmbPublisher/datafiles/  folder make the desired changes in file and then go to EtcdUI webpage then /EmbPublisher/config section and increment the "data_toggle" value.

7) if user wants to switch from using "Sample pub/sub" to "normal VA" to "TemplateDef VA" then go to EtcdUI webpage, then /EmbPublisher/config section and change "msg_file" field with the datafile/Json file containing the required payload and change "publisher_topic" field to the topic on which payload is to be published.

8) If the polling data is to be captured then add following topics in the "Topics" section of [EmbSubscriber/config.json](./EmbSubscriber/config.json) and then rerun provisioning and building process by referring [UWC README](../README.md).
```
                "TCP/RT/update",
                "TCP/NRT/update",
                "RTU/NRT/update",
                "RTU/RT/update"
```
  Note:- Changing topics in EtcdUi for EmbSubscriber will not work and needs change in local config.json of EmbSubscriber.






