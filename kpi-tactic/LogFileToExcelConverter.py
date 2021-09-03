####################################################################################################################
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
###############################################################################

import xlsxwriter
import re

mydict = {'pollSeq': 0,
          "pollTopic": 0,
          "pollRT": 0,
          "pollStatus": 0,
          "pollValue": 0,
          "pollError": 0,
          "tsPollingTime": 0,
          "pollReqRcvdInStack": 0,
          "pollReqSentByStack": 0,
          "pollRespRcvdByStack": 0,
          "pollRespPostedByStack": 0,
          "pollRespPostedToEII": 0,
          "pollDataRcvdInApp": 0,
          "wrReqCreation": 0,
          "wrSeq": 0,
          "wrRspTopic": 0,
          "wrOpRT": 0,
          "wrRspStatus": 0,
          "wrRspError": 0,
          "wrReqRcvdInExport": 0,
          "wrReqPublishOnEII": 0,
          "wrReqRcvdByModbus": 0,
          "wrReqRcvdInStack": 0,
          "wrReqSentByStack": 0,
          "wrRespRcvdByStack": 0,
          "wrRespPostedByStack": 0,
          "wrRespPostedToEII": 0,
          "wrRespRcvdInApp": 0,
          "pollDataRcvdInExport": 0,
          "pollDataPostedToMQTT": 0,
          "wrRespRcvdInExport": 0,
          "wrRespPostedToMQTT": 0}

currposrow = 0
currposcol = 0
rowpos = 1
isadded = False
count = 1


def calculatetimeonfield(sheet):
    global currposcol
    global rowpos
    global isadded
    datadict = {}
    datadict["TotalCtrlLoopTime"] = mydict["wrRespRcvdInApp"] - mydict["tsPollingTime"]
    datadict["TotalPollTime"] = mydict["pollDataRcvdInApp"] - mydict["tsPollingTime"]
    datadict["TotalWrTime"] = mydict["wrRespRcvdInApp"] - mydict["pollDataRcvdInApp"]
    datadict["WrReqInitTime"] = mydict["wrReqCreation"] - mydict["pollDataRcvdInApp"]
    datadict["TotalWrReqProcessTime"] = mydict["wrReqSentByStack"] - mydict["wrReqCreation"]
    datadict["TotalWrRespProcessTime"] = mydict["wrRespRcvdInApp"] - mydict["wrRespRcvdByStack"]
    datadict["PollReqModbusTime"] = mydict["pollReqSentByStack"] - mydict["tsPollingTime"]
    datadict["PollDeviceTime"] = mydict["pollRespRcvdByStack"] - mydict["pollReqSentByStack"]
    datadict["PollRespModbusTime"] = mydict["pollRespPostedToEII"] - mydict["pollRespRcvdByStack"]
    datadict["WrReqModbusTime"] = mydict["wrReqSentByStack"] - mydict["wrReqRcvdByModbus"]
    datadict["WrDeviceTime"] = mydict["wrRespRcvdByStack"] - mydict["wrReqSentByStack"]
    datadict["WrRespModbusTime"] = mydict["wrRespPostedToEII"] - mydict["wrRespRcvdByStack"]
    datadict["PollDataTransitTime"] = mydict["pollDataRcvdInApp"] - mydict["pollRespPostedToEII"]
    datadict["WrReqDataTransitTime"] = mydict["wrReqRcvdByModbus"] - mydict["wrReqCreation"]
    datadict["WrRespDataTransitTime"] = mydict["wrRespRcvdInApp"] - mydict["wrRespPostedToEII"]

    for x in datadict:
        if (isadded == False):
            sheet.write(currposrow, currposcol, x)
        sheet.write(rowpos, currposcol, datadict[x])
        currposcol += 1


def parsestring(line, sheet):
    global currposrow
    global currposcol
    global rowpos
    global isadded
    global mydict


    finalstring = line 
    list = finalstring.split(',')
    if len(list) > 1:
        for st in range(len(list)):
            inrlist = list[st].split(':')
            temp = inrlist[1].replace('"', '')
            print(temp)
            check = re.search("^\d*$",temp)
            if( check ):
                print("done")
                if(len(inrlist[1].replace('"', ''))> 1000):
                    mydict[inrlist[0].replace('"', '')] = int(temp)/1000
                else:
                    mydict[inrlist[0].replace('"', '')] = int(temp)
            else: 
                mydict[(inrlist[0].replace('"', '')).replace(' ','')] = temp                               


        for x in mydict:
            if (isadded == False):
                sheet.write(currposrow, currposcol, x)
            sheet.write(rowpos, currposcol, mydict[x])
            currposcol += 1

        calculatetimeonfield(sheet)
        for x in mydict:

            mydict[x] = 0

        rowpos += 1
        currposrow = 0
        currposcol = 0
        isadded = True


def readfile(filetoread, sheet):
    global count
    while True:
        line = filetoread.readline()
        if not line:
            print('Completed.')
            break
        parsestring(line, sheet)
        print('Working...'+str(count))
        count = count+1


if __name__ == '__main__':
    file = open("AnalysisKPIApp.log")
    wb = xlsxwriter.Workbook('AnalysisFile.xlsx')
    sheet = wb.add_worksheet("Analysis")
    readfile(file, sheet)
    wb.close()
    file.close()
