#include <set>
#include <map>
#include <iostream>
#include <fstream>

using namespace std;

// Set of observed addresses
typedef set<unsigned long> addressSetTy;

// Configuration of observed behavior for given VTable map
struct mapSettingTy
{
    // Address of map
    unsigned long address;
    // CLaimed size of map
    int size;
    // Observed addresses within map
    addressSetTy observedContents;
};

// Mapping between call-sites and the VTable maps used by them
typedef map<unsigned long, mapSettingTy*> callSiteMapTy;
callSiteMapTy callSiteMap;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cerr << "Missing argument: end address of call-sites." << endl;
        exit(-1);
    }
    // Only consider call-sites up until endAddress within the binary
    // This ensures that only call-sites from within testers are used
    unsigned long endAddress;
    sscanf(argv[1], "%lx", &endAddress);
    
    // Read all call-site reports
    callSiteMap.clear();
    while (!cin.eof())
    {
        char line[1024];
        cin.getline (line ,1024);
        unsigned long callSite;
        unsigned long vtable;
        unsigned long map;
        int size;
        int num = sscanf(line, "%lx %lx %lx %d", &callSite, &vtable, &map, &size);
        // Skip invalid lines
        if (num != 4)
            continue;
        // Skip undesirable call-sites
        if (callSite >= endAddress)
            continue;
        // Add information for the call-site
        mapSettingTy *mapSetting;
        if (callSiteMap.count(callSite) == 0)
        {
            mapSetting = new mapSettingTy();
            mapSetting->address = map;
            mapSetting->size = size;
            callSiteMap[callSite] = mapSetting;
        }
        else
            mapSetting = callSiteMap[callSite];
        mapSetting->observedContents.insert(vtable);
    }
    // Check all call-sites
    unsigned long total = 0;
    unsigned long covered = 0;
    for (auto &callSiteMapEntry : callSiteMap)
    {
        // Report error if map at call-site has unused entries
        mapSettingTy *mapSetting = callSiteMapEntry.second;
        total += mapSetting->size;
        covered += mapSetting->observedContents.size();
        if (mapSetting->size != mapSetting->observedContents.size())
        {
            cerr << "Not all map entries used" << endl;
            cerr << "Callsite: 0x" << hex << callSiteMapEntry.first << dec << endl;
            cerr << "Map: 0x" << hex << mapSetting->address << dec << endl;
            cerr << "Map size: " << mapSetting->size << endl;
            cerr << "Entries used: " << mapSetting->observedContents.size() << endl;
            cerr << "Entries: ";
            for (unsigned long entry : mapSetting->observedContents)
            {
                cerr << "0x" << hex << entry << dec << ", ";
            }
            cerr << endl;
        }
    }
    cerr << total << " " << covered << endl;
    return 0;
}
