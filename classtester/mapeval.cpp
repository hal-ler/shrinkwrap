#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char **argv)
{
    unsigned long allTotal = 0;
    unsigned long allCovered = 0;
    // Read all call-site reports
    while (!cin.eof())
    {
        char line[1024];
        cin.getline (line ,1024);
        unsigned long total;
        unsigned long covered;
        int num = sscanf(line, "%lu %lu", &total, &covered);
        // Skip invalid lines
        if (num != 2)
            continue;
        allTotal += total;
        allCovered += covered;
    }
    cerr << "Total number of entries found in VTable sets: " << allTotal << " Total number of entries used from VTable sets: " << allCovered << endl;
    return 0;
}
