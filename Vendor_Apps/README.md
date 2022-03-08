**Contents**

- [About tool.](#about-tool)
- [How to integrate this tool with use case.](#how-to-integrate-this-tool-with-use-case)
- [Miscellaneous Datafiles.](#miscellaneous-datafiles)
- [Configuration of the tool.](#configuration-of-the-tool)
- [Running EMBPublisher](#running-embpublisher)

# About tool.
- This is a EMB publisher and Subscriber where the publish and subscribe happens on the EMB topic(i.e RT/read/flowmeter/PL0/D1).
- EmbPublisher acts as a brokered publisher of EII messagebus.
- EmbSubscriber acts as a subscriber to the EII broker.

# How to integrate this tool with use case.
- In any of the recipe file of uwc, please add 'uwc/Vendor_Apps/EmbPublisher' and 'uwc/Vendor_Apps/EmbSubscriber' components.
- EmbPublisher and EmbSubscriber components are added to recipe to 4, 9 and 10 by default.
- Follow usual provisioning and starting process.

# Miscellaneous Datafiles.
The file containing the JSON data for publishing, which represents the single data point (files should be kept into directory named 'datafiles').

Some of the sample datafiles are:

VA_Sample_Data.json - This JSON file contains the sample data for vendor app scenario of sparkplug.
TemplateDef.json - This JSON file contains the sample data for vendor app scenario of sparkplug.
Publisher_content.json - This JSON file contains the sample data for read on demand and write on demand.

# Configuration of the tool.
Let us look at the sample configuration
```
{
  "config": {
    "cert_type": ["zmq"],
    "pub_name": "VA_Sample_Publisher",
    "msg_file": "Publisher_content.json",
    "publisher_topic": "RT/read/flowmeter/PL0/D1",
    "data_toogle": 1
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
- -data_toogle: Change the value of data_toogle when any content of datafile is changed.
- -publisher_topic: Topic on which the payload is to be published.

# Running EMBPublisher 

Follow the following steps to publish a JSON payload using EMBPublisher :

1) Follow usual provisioning and building process.

2) Please check that EtcdUI(i.e openedgeinsights/ia_etcd_ui) container is up and running.

     a) Open your browser and enter the address: https://< host ip >:7071/etcdkeeper/ (when EII is running in secure mode). In this case, CA cert has to be imported in the browser. For insecure mode i.e. DEV mode, it can be accessed at http://< host ip >:7070/etcdkeeper/.

     b) Username is 'root' and default password is located at ETCD_ROOT_PASSWORD key under environment section in build/provision/dep/docker-compose-provision.override.prod.yml.

3) To check the response msg , please the logs of EmbSubscriber i.e docker logs -f ia_emb_subscriber

4) During runtime add/change in topic is required then go to EtcdUI webpage then /EmbPublisher/config section and change "publisher_topic" field.

5) During runtime add/change in datafile is required then go to EtcdUI webpage then /EmbPublisher/config section and change "msg_file" field with the datafile containing the required payload.

6) During runtime add/change in payload is required then open the datafile present in ./EmbPublisher/datafiles/  folder make the desired changes in file and then go to EtcdUI webpage then /EmbPublisher/config section and increment the "data_toogle" value.






