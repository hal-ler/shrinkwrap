#!/usr/bin/python

import subprocess
import sys

if len(sys.argv) < 2:
    print "Please specify binary"
    exit(-1)
binaryName = sys.argv[1]

# Get the start address of tester functions (and the first non-tester function)
getTesterListCommand = "nm -n " + binaryName + " | grep -A 1 \"T _Z7tester\" | cut -d \" \" -f 1"
testerListTemp = subprocess.check_output(getTesterListCommand, shell=True)

testerList = []
for tester in testerListTemp.split():
    testerList.append(int(tester, 16))

# Get the number of classes in the binary
classCount = len(testerList) - 1
if classCount <= 0:
    print "Invalid binary"
    exit(-1)

# Get all vtable maps associated with each class
vtableMapLists = []
for i in range(0, classCount):
    getMapsCommand = "nm -n " + binaryName + " | grep \"VTVI2c" + str(i) +"\" | cut -d \" \" -f 1"
    mapList = subprocess.check_output(getMapsCommand, shell=True)
    vtableMapLists.append(mapList.split())

# Check that the vtable maps associated with each class are only used within the right tester
for i in range(0, classCount):
    # Traverse all vtable maps associated with given class
    for vtableMap in vtableMapLists[i]:
        # Find call-sites where the vtable map is used
        getCallsitesCommand = "objdump -d " + binaryName + " | grep " + hex(int(vtableMap, 16))[2:] +" | cut -d \":\" -f 1"
        callSiteList = subprocess.check_output(getCallsitesCommand, shell=True)
        for callSite in callSiteList.split():
            callSiteInt = int(callSite, 16)
            # Find the tester to which the call-site corresponds to
            # Call-sites outside of testers are ignored
            expectedCallSiteType = classCount
            for j in range(0, classCount):
                if callSiteInt >= testerList[j] and callSiteInt < testerList[j + 1]:
                    expectedCallSiteType = j
            # Report an error if the call-site is within the wrong tester
            if  expectedCallSiteType < classCount and expectedCallSiteType != i:
                print "Call-site uses wrong type at: 0x" + callSite
                print "Expected c" + str(expectedCallSiteType) + ", found c" + str(i)
            #    exit(-1)




