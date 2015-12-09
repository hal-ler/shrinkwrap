#!/usr/bin/python

import subprocess
import sys
from signal import signal, SIGPIPE, SIG_DFL
signal(SIGPIPE,SIG_DFL) 

binaryCode = []
binaryData = []
binaryDataOffset = 0
binarySymbols = []

# Read the content of the binary into memory
def init_binary(binaryName):
    global binaryCode
    global binaryData
    global binaryDataOffset
    global binarySymbols
    getBinaryCodeCommand = "objdump -d " + binaryName
    binaryCode = subprocess.check_output(getBinaryCodeCommand, shell=True).split("\n")[:-1]
    getBinaryDataCommand = "objdump -s -j .rodata " + binaryName
    binaryDataTemp = subprocess.check_output(getBinaryDataCommand, shell=True).split("\n")[4:-1]
    for line in binaryDataTemp:
        binaryData.append(line.split(" ")[2:6])
    binaryDataOffset = int(binaryDataTemp[0].split(" ")[1], 16)
    getBinarySymbolsCommand = "nm -n " + binaryName
    binarySymbols = subprocess.check_output(getBinarySymbolsCommand, shell=True).split("\n")[:-1]

# Search for string in text, returning surrounding lines (similar to grep -A -B)
def get_lines(lines, string, before = 0, after = 0):
    result = []
    for lineCount in range(0, len(lines)):
        line = lines[lineCount]
        if string in line:
            oneResult = []
            start = max(0, lineCount - before)
            end = min(len(lines), lineCount + after + 1)
            for i in range(start, end):
                oneResult.append(lines[i])
            result.append(oneResult)
    return result

# Transform a number from the objdump memory representation to its natural form
def endian_correct(string):
    newstring = ""
    for i in range(0, len(string), 2):
        newstring = string[i] + string[i + 1] + newstring
    return newstring

# Extract all the addresses from a given range in the binary data
def get_addresses(startAddress, endAddress):
    addresses = []
    numAddresses = (endAddress - startAddress) / 8
    leftAddresses = numAddresses
    startOffset = (startAddress - binaryDataOffset) / 16
    endOffset = (endAddress - binaryDataOffset + 15) / 16
    lineOffset = ((startAddress - binaryDataOffset) / 8) - (2 * startOffset)
    for offset in range(startOffset, endOffset):
        while lineOffset < 2:
            data = binaryData[offset][lineOffset * 2: lineOffset * 2 + 2]
            entry = int(endian_correct(data[0] + data[1]), 16)
            if leftAddresses > 0:
                if entry != 0:
                    addresses.append(entry)
                leftAddresses -= 1
            else:
                break
            lineOffset += 1
        lineOffset = 0  
    return addresses

# Retrieve all the vtable map entries (LLVM variant)
def get_vtable_entries_llvm(classCount, vtableMapLists):
    # Get vtable map ranges associated with each class
    vtableMapRangesList = []
    for i in range(0, classCount):
        mapSizeAddr = int(get_lines(binarySymbols, "VTVI2c" + str(i) + "_size")[0][0].split(" ")[0], 16)
        mapSize = get_addresses(mapSizeAddr, mapSizeAddr + 8)[0]
        mapStart = int(get_lines(binarySymbols, "VTVI2c" + str(i) + "_array")[0][0].split(" ")[0], 16)
        vtableMapRangesList.append([mapStart, mapStart + 8 * mapSize])

    # Get content for all vtable maps associated with each class
    # Check that the vtable maps associated with each class are only used within the right tester
    vtableEntries = dict()
    for i in range(0, classCount):
        # Traverse all vtable maps associated with given class
            # Find content of vtable map
            vtableEntries[vtableMapLists[i][0]] = get_addresses(vtableMapRangesList[i][0], vtableMapRangesList[i][1])
    return vtableEntries

# Get the offset within the vtable which will be used at the call-site (LLVM variant)
def get_vtable_offset_llvm(callSiteInt):
    callSiteCode = get_lines(get_lines(binaryCode, hex(callSiteInt)[2:], 0, 100)[0], "callq  *")[0][0]
    offset = int(callSiteCode.split('*')[1].split('(')[0], 16)
    return [offset, True]

# Retrieve all the vtable map entries (VTV variant)
def get_vtable_entries_vtv(classCount, vtableMapLists):
    vtableEntries = dict()
    for i in range(0, classCount):
        # Traverse all vtable maps associated with given class
        for vtableMap in vtableMapLists[i]:
            vtableEntries[vtableMap] = []
            # Find call-sites where the vtable map is used
            callSites = get_lines(binaryCode, hex(vtableMap)[2:], 4, 1)
            for callSite in callSites:
                if "VLTRegisterPair" in callSite[5]:
                    vtableEntries[vtableMap].append(int(callSite[1].split("$")[1].split(",")[0], 16))
                if "VLTRegisterSet" in callSite[5]:
                    startAddress = int(callSite[0].split("$")[1].split(",")[0], 16)
                    size = int(callSite[1].split("$")[1].split(",")[0], 16)
                    vtableEntries[vtableMap] += get_addresses(startAddress, startAddress + 8 * size)
    return vtableEntries

# Get the offset within the vtable which will be used at the call-site (VTV variant)
def get_vtable_offset_vtv(callSiteInt):
    callSites = get_lines(binaryCode, hex(callSiteInt)[2:], 0, 2)
    if "add" not in callSites[0][2]:
        return [0, False]
    return [int(callSites[0][2].split("$")[1].split(",")[0], 16), True]

# Main
if len(sys.argv) < 3:
    print "Please specify binary and mode"
    exit(-1)
binaryName = sys.argv[1]
mode = sys.argv[2]

init_binary(binaryName)

# Get the start address of tester functions (and the first non-tester function)
testerList = []
for testerLines in get_lines(binarySymbols, " _Z7tester", 0, 1):
    testerList.append(int(testerLines[0].split(" ")[0], 16))
    if " _Z7tester" not in testerLines[1]:
        testerList.append(int(testerLines[1].split(" ")[0], 16))

# Get the number of classes in the binary
classCount = len(testerList) - 1
if classCount <= 0:
    print "Invalid binary"
    exit(-1)

# Get all vtable maps associated with each class
vtableMapLists = []
for i in range(0, classCount):
    vtableMapLists.append([])
    for candidateLine in get_lines(binarySymbols, "VTVI2c" + str(i)):
        if "_size" not in candidateLine[0] and "_array" not in candidateLine[0]:
            vtableMapLists[i].append(int(candidateLine[0].split(" ")[0], 16))

if mode == "llvm":
    vtableEntries = get_vtable_entries_llvm(classCount, vtableMapLists)
if mode == "vtv":
    vtableEntries = get_vtable_entries_vtv(classCount, vtableMapLists)

# Check that the vtable maps associated with each class are only used within the right tester
callSiteTargets = dict()
total = 0
count = 0
for i in range(0, classCount):
    # Traverse all vtable maps associated with given class
    for vtableMap in vtableMapLists[i]:
        # Find call-sites where the vtable map is used
        callSiteList = []
        for line in get_lines(binaryCode, hex(vtableMap)[2:]):
            callSiteList.append(line[0].split(":")[0])
        for callSite in callSiteList:
            callSiteInt = int(callSite, 16)
            # Find the tester to which the call-site corresponds to
            # Call-sites outside of testers are ignored
            expectedCallSiteType = classCount
            for j in range(0, classCount):
                if callSiteInt >= testerList[j] and callSiteInt < testerList[j + 1]:
                    expectedCallSiteType = j
            # Evaluate call-site if within the tester
            if expectedCallSiteType < classCount:
                isCorrectCallSite = True
                if mode == "llvm":
                    offset = get_vtable_offset_llvm(callSiteInt)[0]
                if mode == "vtv":
                    candidateOffset = get_vtable_offset_vtv(callSiteInt)
                    if not candidateOffset[1]:
                        continue
                    offset = candidateOffset[0]
                for vtable in vtableEntries[vtableMap]:
                    funcPtrs = get_addresses(vtable + offset, vtable + offset + 8)
                    if len(funcPtrs) == 0:
                        #print "Error call to NULL address @" + hex(callSiteInt)
                        isCorrectCallSite = False
                        continue
                    functionLines = get_lines(binarySymbols, hex(funcPtrs[0])[2:])
                    if len(functionLines) == 0:
                        #print "Error call to non-function address @" + hex(callSiteInt)
                        isCorrectCallSite = False
                        continue
                    getFuncNameCommand = "c++filt \"" + functionLines[0][0].split(" ")[2] + "\""
                    funcName = subprocess.check_output(getFuncNameCommand, shell=True)[:-1]
                    if len(funcName.split(":")) < 3:
                        #print "Error invalid function " + funcName +" @" + hex(callSiteInt)
                        isCorrectCallSite = False
                        continue
                    funcOverloadedName = funcName.split(":")[2]
                    #print funcName + " @" + hex(callSiteInt) 
                    if callSiteInt not in callSiteTargets:
                        callSiteTargets[callSiteInt] = funcOverloadedName
                    if callSiteTargets[callSiteInt] != funcOverloadedName:
                        #print "Error mismatching call " + callSiteTargets[callSiteInt] + " != " + funcOverloadedName + " @" + hex(callSiteInt)
                        isCorrectCallSite = False
                total += 1
                if isCorrectCallSite:
                    count += 1
print "Total call-site count: ", total, "Call-sites that can only target a single method family: ", count



