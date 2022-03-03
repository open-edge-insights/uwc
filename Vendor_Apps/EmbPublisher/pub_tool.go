/*
Copyright (c) 2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

package main

import (
	eiicfgmgr "github.com/open-edge-insights/eii-configmgr-go/eiiconfigmgr"
	eiimsgbus "github.com/open-edge-insights/eii-messagebus-go/eiimsgbus"
	"encoding/json"
	"fmt"
	"sync"
	"time"
	"os"
	"strings"
	

)

var wg sync.WaitGroup
func cbFunc(key string, value map[string]interface{}, user_data interface{}) {
	fmt.Printf("Callback triggered for key %s, value obtained %v with user_data %v\n", key, value, user_data)
	os.Exit(0)
}

func main() {
    configmgr, err := eiicfgmgr.ConfigManager()
	if err != nil {
		fmt.Printf("Config Manager initialization failed...")
		return
	}
	appConfig, err := configmgr.GetAppConfig()
	if err != nil {
		fmt.Errorf("Error in getting application config: %v", err)
		return
	}
	pubctx, err := configmgr.GetPublisherByName(appConfig["pub_name"].(string))
	if err != nil {
		fmt.Printf("GetPublisherByName failed with error:%v", err)
		return
	}
	config, err := pubctx.GetMsgbusConfig()
	if err != nil {
		fmt.Printf("GetMsgbusConfig failed with error:%v", err)
		return
	}
	msg_file := "/datafiles/" + appConfig["msg_file"].(string)
	msg, err := eiimsgbus.ReadJsonConfig(msg_file)
	if err != nil {
		fmt.Printf("-- Failed to parse config: %v\n", err)
		return
	}

	buffer, err := json.Marshal(msg)

	if err != nil {
		fmt.Printf("-- Failed to Marshal the message : %v\n", err)
		return
	} else {
		fmt.Printf("-- message size is: %v\n", len(buffer))
	}

	var msgData map[string]interface{}
	if err := json.Unmarshal(buffer, &msgData); err != nil {
		fmt.Printf("-- Failed to Unmarshal the message : %v\n", err)
		return
	}

	topic:= appConfig["publisher_topic"].(string)
	wg.Add(1)
	fmt.Println("yellow...1")
	fmt.Println("Publishing on topic%s\n",topic)
	go publisher_function(config, topic, msgData)
	

	var watchPrefixUserData interface{} = ""
	watchObj, err := configmgr.GetWatchObj()
	if err != nil {
		fmt.Println("Failed to fetch watch object")
	}

    // Watch the key "/EmbPublisher" for any changes, cbFunc will be called with updated value
	watchObj.WatchPrefix("/EmbPublisher", cbFunc, watchPrefixUserData)


	wg.Wait()
}


func publisher_function(config map[string]interface{}, embtopic string, msgData map[string]interface{}) {
	defer wg.Done()
	fmt.Printf("-- Initializing message bus context:%v\n", config)
	client, err := eiimsgbus.NewMsgbusClient(config)
	if err != nil {
		fmt.Printf("-- Error initializing message bus context: %v\n", err)
		return
	}
	defer client.Close()

	fmt.Printf("-- Creating publisher for topic %s\n", embtopic)
	publisher, err := client.NewPublisher(embtopic)
	if err != nil {
		fmt.Printf("-- Error initializing publisher : %v\n", err)
		return
	}
	defer publisher.Close()
	// in case of Sample publisher
	if((strings.HasPrefix(embtopic, "RT")) || (strings.HasPrefix(embtopic, "NRT"))){
		// spliting the topic (eg: RT/read/flowmeter/PL0/D13) based on /
		sub_string := strings.Split(embtopic, "/")
		mqttTopic := "/" + sub_string[2]+"/"+sub_string[3] +"/" +sub_string[4]+"/" +sub_string[1]
    	msgData["sourcetopic"] = mqttTopic
	}else{// in case of VA
		msgData["data_topic"] = embtopic
	}
	fmt.Printf("Data to be published is  %s\n", msgData)
	time.Sleep(5 * time.Second)
	err = publisher.Publish(msgData)
	if err != nil {
		fmt.Printf("-- Failed to publish message: %v\n", err)
		return
	}		
	for {
		// Waiting for the messages
		time.Sleep(1 * time.Second)
	}
}
