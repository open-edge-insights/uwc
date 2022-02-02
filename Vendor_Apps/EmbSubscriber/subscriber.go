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
	"flag"
	eiimsgbus "github.com/open-edge-insights/eii-messagebus-go/eiimsgbus"
	eiicfgmgr "github.com/open-edge-insights/eii-configmgr-go/eiiconfigmgr"
	"github.com/golang/glog"
	"os"
)

type msgbusSubscriber struct {
	msgBusSubMap       map[*eiimsgbus.Subscriber]string // eii messagebus subscriber handels for topic prefix
	confMgr            *eiicfgmgr.ConfigMgr             // Config manager reference
	msgBusClient       []*eiimsgbus.MsgbusClient        // list of message bus client
}

// Creates the subscriber for each topic
func (subObj *msgbusSubscriber) startSubscribers(subTopics []string, subConfig map[string]interface{}) error {
	msgBusClient, err := eiimsgbus.NewMsgbusClient(subConfig)
	if err != nil {
		glog.Errorf("-- Error initializing message bus context: %v\n", err)
		os.Exit(1)
	}

	for _, topic := range subTopics {
		glog.Infof("Creating subscriber for a topic %v", topic)
		sub, err := msgBusClient.NewSubscriber(topic)
		if err != nil {
			glog.Errorf("Error creating subscriber:%v", err)
			return err
		}
		subObj.msgBusSubMap[sub] = topic
	}
	subObj.msgBusClient = append(subObj.msgBusClient, msgBusClient)
	return nil
}

// Will put each subscriber into receiving mode.
func (subObj *msgbusSubscriber) receiveFromAllTopics() error {
	done := make(chan bool)
	for sub, topic := range subObj.msgBusSubMap {
		glog.Infof("Starting subscriber loop, for a topic %v", topic)
		go processMsg(sub)
	}
	<-done
	return nil
}

func processMsg(sub *eiimsgbus.Subscriber) {
	msgCount := make(map[string]int)
	for {
		select {
		case msg := <-sub.MessageChannel:
			glog.Infof("-- Received Message from topic %v : %v \n",msg.Name, msg.Data)
			msgCount[msg.Name] += 1
			glog.Infof("number of message recieved %v for topic %v", msgCount[msg.Name], msg.Name)
		case err := <-sub.ErrorChannel:
			glog.Errorf("-- Error receiving message: %v\n", err)
		}

	}

}

// StopAllSubscribers function to close all subscriber objects
func (subObj *msgbusSubscriber) stopAllSubscribers() {
	for sub, _ := range subObj.msgBusSubMap {
		sub.Close()
	}
}

// StopClient function will stop client
func (subObj *msgbusSubscriber) stopClient() {
	for _ , client := range subObj.msgBusClient {
		client.Close()
	}
}

func main() {
	var subObj msgbusSubscriber
	var err error
	flag.Parse()
	subObj.msgBusSubMap = make(map[*eiimsgbus.Subscriber]string)
	subObj.confMgr, err = eiicfgmgr.ConfigManager()
	if err != nil {
		glog.Errorf("Config Manager initialization failed...")
		os.Exit(1)
	}
	flag.Set("logtostderr", "true")
	flag.Set("stderrthreshold", os.Getenv("GO_LOG_LEVEL"))
	flag.Set("v", os.Getenv("GO_VERBOSE"))

	numOfSubscribers, err := subObj.confMgr.GetNumSubscribers()
	if err != nil {
		glog.Errorf("Error occured with error:%v", err)
		os.Exit(1)
	}
	for subIndex := 0; subIndex < numOfSubscribers; subIndex++ {
		subCtx, err := subObj.confMgr.GetSubscriberByIndex(subIndex)
		if err != nil {
			glog.Errorf("Error occured with error:%v", err)
			os.Exit(1)
		}
		topics, err := subCtx.GetTopics()
		if err != nil {
			glog.Errorf("Failed to fetch topics : %v", err)
			os.Exit(1)
		}
		config, err := subCtx.GetMsgbusConfig()
		if err != nil {
			glog.Errorf("Error while getting eii message bus config: %v\n", err)
			os.Exit(1)
		}
		subCtx.Destroy()
		if config != nil {
			subObj.startSubscribers(topics, config)
		}
	}
	subObj.receiveFromAllTopics()
}
