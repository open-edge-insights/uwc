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
//	"strings"
//	"io/ioutil"
	
)

var wg sync.WaitGroup

func cbFunc(key string, value map[string]interface{}, user_data interface{}) {
	fmt.Printf("Callback triggered for key %s, value obtained %v with user_data %v\n", key, value, user_data)
	os.Exit(1)
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
	intval, err := time.ParseDuration(appConfig["interval"].(string))
	if err != nil {
		fmt.Printf("-- Error in interval conversion : %v\n", err)
		return
	}
	msg_file := "/datafiles/" + appConfig["msg_file"].(string)
	//input, err := ioutil.ReadFile(msg_file)
	msg_value := appConfig["data_change"].(string)
	//msg_datatype= appConfig["dataType"]
		//var new_datatype
	// msg_datatype_type := reflect.TypeOf(msg_datatype).(string)
	// if msg_datatype_type == "int"{
	// 	var new_datatype="  \"dataType\":" + "\"" + msg_datatype.(int)+"\","
	// }else{
	// 	var new_datatype="  \"dataType\":" + "\"" + msg_datatype.(string)+"\","
	// }
	// new_datatype:="  \"dataType\":" + "\"" + msg_datatype+"\","
	// lines := strings.Split(string(input), "\n")
	// new_value := "  \"value\":" + "\"" + msg_value	+"\","


	// output := ""
	// for i,line := range lines {
	// 	if (strings.Contains(line, "value") && !strings.Contains(appConfig["msg_file"].(string),"TemplateDef")) {
	// 		i = i +10
	// 		line= new_value
	// 	}
	// 	if (strings.Contains(line, "dataType") && !strings.Contains(appConfig["msg_file"].(string),"TemplateDef")) {
	// 		i = i +10
	// 		line= new_datatype
	// 	}
	// 	output = output + "\n" + line
	// }

	// err = ioutil.WriteFile(msg_file, []byte(output), 0644)
	// if err != nil {
	// 	fmt.Printf("-- Failed to parse config: %v\n",err)
	// 	return
	// }
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

	topics, err := pubctx.GetTopics()
	if err != nil {
		fmt.Printf("Error: %v to GetTopics", err)
		return
	}

	for _, tpName := range topics {
		wg.Add(1)
		go publisher_function(config, tpName, msgData, intval,msg_value)
	}

	var watchUserData interface{} = "watch"
	watchObj, err := configmgr.GetWatchObj()
	if err != nil {
		fmt.Println("Failed to fetch watch object")
	}

    // Watch the key "/EmbPublisher" for any changes, cbFunc will be called with updated value
	watchObj.Watch("/EmbPublisher", cbFunc, watchUserData)


	wg.Wait()
}


func publisher_function(config map[string]interface{}, topic string, msgData map[string]interface{}, intval time.Duration,msg_value string) {
	defer wg.Done()
	fmt.Printf("-- Initializing message bus context:%v\n", config)
	client, err := eiimsgbus.NewMsgbusClient(config)
	if err != nil {
		fmt.Printf("-- Error initializing message bus context: %v\n", err)
		return
	}
	defer client.Close()

	fmt.Printf("-- Creating publisher for topic %s\n", topic)
	publisher, err := client.NewPublisher(topic)
	if err != nil {
		fmt.Printf("-- Error initializing publisher : %v\n", err)
		return
	}
	defer publisher.Close()
    msgData["topic"] = topic
	fmt.Printf("data is  %s\n", msg_value)
	fmt.Printf("HEREEE %s\n", msgData["value"])
	err = publisher.Publish(msgData)
	if err != nil {
		fmt.Printf("-- Failed to publish message: %v\n", err)
		return
	}
	time.Sleep(intval)
}
