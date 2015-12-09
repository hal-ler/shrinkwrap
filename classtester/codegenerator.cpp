#include <set>
#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdlib.h>

// Configuration of class hierarchies
#define CLASSES 5
#define PARENTS_PER_CLASS 3
// Number of variants to generate for random characteristics
#define RANDOM_VARIANTS 10

// Define if diamond inheritance is desired
#define DO_DIAMOND

// Configuration for overriding options (default is no override)
//#define DO_ALLOVERRIDE
//#define DO_RANDOMOVERRIDE

#if defined(DO_DIAMOND) && defined(DO_RANDOMOVERRIDE)
#error "Cannot do random override in case of diamond inheritance."
#endif

#if defined(DO_ALLOVERRIDE) && defined(DO_RANDOMOVERRIDE)
#error "Cannot do all and random override at the same time."
#endif

using namespace std;

typedef vector<int> parentVectorTy;
typedef set<int> parentSetTy;
typedef vector<string> stringVectorTy;

// Configuration for a given class within the hierarchy
// directParents is the list of parent classes by ID
// virtual parents within directParents are signalled by having CLASSES added to their ID
// allVirtualParents and allNonVirtualParents are the sets of parent classes by ID
struct classConfiguration
{
    parentVectorTy directParents;
    parentSetTy allNonVirtualParents;
    parentSetTy allVirtualParents;
    classConfiguration()
    {
        directParents.clear();
        allNonVirtualParents.clear();
        allVirtualParents.clear();
    }
    classConfiguration(classConfiguration *cc)
    {
        directParents.insert(
            directParents.begin(),
            cc->directParents.begin(),
            cc->directParents.end());
        allNonVirtualParents.insert(
            cc->allNonVirtualParents.begin(),
            cc->allNonVirtualParents.end());
        allVirtualParents.insert(
            cc->allVirtualParents.begin(),
            cc->allVirtualParents.end());;
    }
    // Ordering function to elliminate repetitions while generating combinations
    unsigned int getOrder()
    {
        unsigned int result = 0;
        for (int parent : directParents)
        {
            int entry = (parent % CLASSES + 1);
            entry *= (parent / CLASSES) + 1;
            result += entry;
            result *= (2 * (CLASSES + 1));
        }
        return result;
    }
};

// Configuration of the class hierarchy as a list of class configurations
typedef vector<classConfiguration*> configurationTy;
// List of class configurations generated via variations
typedef vector<classConfiguration*> classSolutionVectorTy;

// Generate a new class configuration by adding the given parent if feasible
// Returns NULL if parent cannot be added to the existing configuration
classConfiguration *checkAndGenerateInheritance(configurationTy &configuration, classConfiguration *currentClass, int parent, bool doVirtual)
{
    // Direct parents cannot be added a second time
    for (int directParent : currentClass->directParents)
        if (directParent % CLASSES == parent)
            return NULL;

    // Non-virtual copies of existing virtual parents cannot be added directly
    if (!doVirtual)
    {
        if(currentClass->allVirtualParents.count(parent))
            return NULL;
    }
    // Non-virtual parents cannot be added directly
    if(currentClass->allNonVirtualParents.count(parent))
        return NULL;
    // Existing direct parents cannot conflict with the parents inherited transitively from new candidate
    for (int directParent : currentClass->directParents)
        // Existing non-virtual direct parent cannot also be inherited
        if (directParent < CLASSES)
        {
            if (configuration[parent]->allNonVirtualParents.count(directParent % CLASSES))
                return NULL;
            if (configuration[parent]->allVirtualParents.count(directParent % CLASSES))
                return NULL;
        }
        else 
        {
            // Existing virtual direct parent cannot also be inherited as non-virtual parent
            if (configuration[parent]->allNonVirtualParents.count(directParent % CLASSES))
                return NULL;
        }

    // Generate new configuration
    classConfiguration *candidateClass = new classConfiguration(currentClass);
    // Add direct parent / Offset class ID in case of virtual inheritance
    if (!doVirtual)
        candidateClass->directParents.push_back(parent);
    else
        candidateClass->directParents.push_back(parent + CLASSES);
    
    // Track the total parent count to detect diamond inheritance
    int sizeBefore = candidateClass->allNonVirtualParents.size() +
                     candidateClass->allVirtualParents.size();
    
    // Update parent lists (both direct and transitively inherited)
    candidateClass->allNonVirtualParents.insert(
        configuration[parent]->allNonVirtualParents.begin(),
        configuration[parent]->allNonVirtualParents.end());
    candidateClass->allVirtualParents.insert(
        configuration[parent]->allVirtualParents.begin(),
        configuration[parent]->allVirtualParents.end());
    if (!doVirtual)
        candidateClass->allNonVirtualParents.insert(parent);
    else
        candidateClass->allVirtualParents.insert(parent);
    
    // Tracke the changes in the parent count
    int sizeAfter = candidateClass->allNonVirtualParents.size() +
                     candidateClass->allVirtualParents.size();
    int sizeAdded = configuration[parent]->allNonVirtualParents.size() +
                    configuration[parent]->allVirtualParents.size() + 1;

#ifndef DO_DIAMOND
    // Check the intersection of the sets to detect diamond inheritance
    // Use the union size to short-cut the intersection
    if (sizeBefore + sizeAdded != sizeAfter)
    {
        delete candidateClass;
        return NULL;
    }
#endif
    return candidateClass;
}

// Recursive function to generate all valid configurations for a given class
// Based on variation generation logic + validation
void generateClassConfigurations(configurationTy &configuration, classConfiguration *currentClass, classSolutionVectorTy &solutions)
{
    solutions.push_back(currentClass);
    if (currentClass->directParents.size() == PARENTS_PER_CLASS)
        return;
    for (int candidateParent = 0; candidateParent < configuration.size(); ++candidateParent)
    {
        classConfiguration *newClass;
        newClass = checkAndGenerateInheritance(configuration, currentClass, candidateParent, false);
        if (newClass)
            generateClassConfigurations(configuration, newClass, solutions);
        newClass = checkAndGenerateInheritance(configuration, currentClass, candidateParent, true);
        if (newClass)
            generateClassConfigurations(configuration, newClass, solutions);
    }
}

// Validation function for a class hierarchy
// A class hierarchy is invalid if not all classes form a connected component
// Uses breadth-first search with non-directional edges
bool checkHierarchyConfiguration(configurationTy &configuration)
{
    if (configuration.size() == 0)
        return false;
    list<int> bfslist;
    set<int> visited;
    visited.insert(0);
    bfslist.push_back(0);
    while (bfslist.size() > 0)
    {
        int current = bfslist.front();
        bfslist.pop_front();
        for (int parent : configuration[current]->allNonVirtualParents)
            if (visited.count(parent) == 0)
            {
                bfslist.push_back(parent);
                visited.insert(parent);
            }
        for (int parent : configuration[current]->allVirtualParents)
            if (visited.count(parent) == 0)
            {
                bfslist.push_back(parent);
                visited.insert(parent);
            }
        int pos = 0;
        for (classConfiguration *classConfig : configuration)
        {
            if (classConfig->allNonVirtualParents.count(current) > 0 && visited.count(pos) == 0)
            {
                bfslist.push_back(pos);
                visited.insert(pos);
            }
            if (classConfig->allVirtualParents.count(current) > 0 && visited.count(pos) == 0)
            {
                bfslist.push_back(pos);
                visited.insert(pos);
            }
            pos++;
        }
    }
    return visited.size() == configuration.size();
}


// Recursive function to generate all valid casting chains for a given class to one of its parents
void accumulateAllCastStrings(configurationTy &configuration, int classId, int targetClassId, stringVectorTy &stringVector)
{
    // Target reached on this path, add casting string to solution
    if (classId == targetClassId)
    {
        stringstream cast;
        cast << "(c" << classId << "*)";
        stringVector.push_back(cast.str());
        return;
    }
    // Accumulate potential casting strings along each direct parent
    stringVectorTy parentStringVector;
    for (int parentId : configuration[classId]->directParents)
    {
        accumulateAllCastStrings(configuration, parentId % CLASSES, targetClassId, parentStringVector);
    }
    // Add current class to the casting chain before saving them to the solutions
    for (string parentString : parentStringVector)
    {
        stringstream cast;
        cast << "(c" << classId << "*)";
        stringVector.push_back(parentString + cast.str());
    }
    return;
}

// Count the number of times a given parent class is inherited into the current class non-virtually
int checkParentCount(configurationTy &configuration, int classId, int parentId)
{
    if (classId == parentId)
        return 1;
    int count = 0;
    for (int directParentId : configuration[classId]->directParents)
    {
        count += checkParentCount(configuration, directParentId % CLASSES, parentId);
    }
    return count;
}

// Count the number of times a given parent class is inherited into the current class virtually
int checkNonVirtualParentCount(configurationTy &configuration, int classId, int parentId)
{
    if (classId == parentId)
        return 1;
    classId %= CLASSES;
    int count = 0;
    for (int directParentId : configuration[classId]->directParents)
    {
        count += checkNonVirtualParentCount(configuration, directParentId, parentId);
    }
    return count;
}

// Print current class hierarchy into autogenerated source file
void printHierarchyConfiguration(configurationTy &configuration, int *count)
{
    ofstream sourceFile;
    stringstream fileName;
    fileName << "autogen-sources/source-" << *count << ".cpp";
    sourceFile.open(fileName.str().c_str());
    int pos = 0;
    // Generate code correponding to each class
    for (classConfiguration *currentClass : configuration)
    {
        parentSetTy allParents;
        allParents.clear();
        allParents.insert(currentClass->allNonVirtualParents.begin(),
                          currentClass->allNonVirtualParents.end());
        allParents.insert(currentClass->allVirtualParents.begin(),
                          currentClass->allVirtualParents.end());
        // Forward declaration of class
        sourceFile << "struct c" << pos << ";" << endl;
        // Forward declararion of tester
        sourceFile << "void __attribute__ ((noinline)) tester" << pos << "(c" << pos << "* p);" << endl;
        // Declaration of class with its direct parents
        sourceFile << "struct c" << pos;
        if (currentClass->directParents.size() > 0)
            sourceFile << " : ";
        for (int parent :  currentClass->directParents)
        {
            if (parent >= CLASSES)
                sourceFile << "virtual ";
            sourceFile << "c" << (parent % CLASSES);
            if (parent != currentClass->directParents.back())
                sourceFile << ", ";
        }
        sourceFile << endl;
        sourceFile << "{" << endl;
        // Boolean to signal that this part of the object is still active
        sourceFile << "bool active" << pos << ";" << endl;
        // Set active in constructor
        sourceFile << "c" << pos << "() : active" << pos << "(true) {}" << endl;
        // Destructor calling the tester of itself and its parent classes
        sourceFile << "virtual ~c" << pos << "()" << endl;
        sourceFile << "{" << endl;
        sourceFile << "tester" << pos << "(this);" << endl;
        for (int parent : allParents)
        {
            stringVectorTy castStringVector;
            accumulateAllCastStrings(configuration, pos, parent, castStringVector);
            int count = 0;
            for (string castString : castStringVector)
            {
                sourceFile << "c" << parent << " *p" << parent << "_" << count << " = ";
                sourceFile << castString << "(this);" << endl;
                sourceFile << "tester" << parent << "(p" << parent << "_" << count << ");" << endl;
                ++count;
            }
        }
        // Reset active in destructor
        sourceFile << "active" << pos << " = false;" << endl;
        sourceFile << "}" << endl;
        // Class-specific virtual method
        sourceFile << "virtual void f" << pos << "(){}" << endl;
#if defined(DO_RANDOMOVERRIDE) || defined(DO_ALLOVERRIDE)
        // Optional overriding of parent methods
        for (int parent : allParents)
#ifdef DO_RANDOMOVERRIDE
            if (rand() % 2 == 0)
#endif
                sourceFile << "virtual void f" << parent << "(){}" << endl;
#endif
        sourceFile << "};" << endl;
        // Tester for the class calling its accesible methods
        // Only call non-ambiguous methods which are inherited a single time
        sourceFile << "void __attribute__ ((noinline)) tester" << pos << "(c" << pos << "* p)" << endl;
        sourceFile << "{" << endl;
        sourceFile << "p->f" << pos << "();" << endl;
        for (int parent : currentClass->allNonVirtualParents)
            if (checkParentCount(configuration, pos, parent) == 1)
            {
                sourceFile << "if (p->active" << parent << ")" << endl;
                sourceFile << "p->f" << parent << "();" << endl;
            }
        for (int parent : currentClass->allVirtualParents)
            if (checkNonVirtualParentCount(configuration, pos, parent) == 0)
            {
                sourceFile << "if (p->active" << parent << ")" << endl;
                sourceFile << "p->f" << parent << "();" << endl;
            }
        sourceFile << "}" << endl;
        pos++;
    }
    // Helper to disable loop unrolling and keep program structure simple
    sourceFile << "int __attribute__ ((noinline)) inc(int v) {return ++v;}" << endl;
    // Main function
    sourceFile << "int main()" << endl << "{" << endl;
    // For each class create each possible object instance of it and test them
    pos = 0;
    for (classConfiguration *currentClass : configuration)
    {
        sourceFile << "c" << pos << "* ptrs" << pos << "[" << CLASSES * CLASSES << "];" << endl;
        int childPos = 0;
        int count = 0;
        for (classConfiguration *childClass : configuration)
        {
            if (childPos != pos &&
                childClass->allNonVirtualParents.count(pos) == 0 &&
                childClass->allVirtualParents.count(pos) == 0
            )
            {
                ++childPos;
                continue;
            }
            // Generate instance of child class and cast it to desired class along every possible chain
            stringVectorTy castStringVector;
            accumulateAllCastStrings(configuration, childPos, pos, castStringVector);
            for (string castString : castStringVector)
            {
                sourceFile << "ptrs" << pos << "[" << count << "] = " << castString << "(new c" << childPos << "());" << endl;
                ++count;
            }
            ++childPos;
        }
        // Call tester and destructor on every generated instance of class
        sourceFile << "for (int i=0;i<" << count << ";i=inc(i))" << endl;
        sourceFile << "{" << endl;
        sourceFile << "tester" << pos << "(ptrs" << pos << "[i]);" << endl;
        sourceFile << "delete ptrs" << pos << "[i];" << endl;
        sourceFile << "}" << endl;
        pos++;
    }
    sourceFile << "return 0;" << endl << "}" << endl;
    sourceFile.close();
}

// Recursive function to generate all valid class hierarchy configurations
// Based on combination generation logic + validation
void generateHierarchyConfigurations(configurationTy &configuration, int *count)
{
    // Print hierarchy if it is valid
    if (checkHierarchyConfiguration(configuration))
    {
#if !defined(DO_RANDOMOVERRIDE)
        ++(*count);
        printHierarchyConfiguration(configuration, count);
#else
        // Generate multiple variants for random characteristics
        for (int i = 0; i < RANDOM_VARIANTS; ++i)
        {
            printConfiguration(configuration, count);
            ++(*count);
        }
#endif
    }
    // Hierarchy full, stop
    if (configuration.size() == CLASSES)
        return;

    // Generate all configurations for the next class in the hierarchy
    classSolutionVectorTy solutions;
    classConfiguration *initialClass = new classConfiguration();
    generateClassConfigurations(configuration, initialClass, solutions);
    
    // Find the order of the last class in the hierarchy
    unsigned int previousOrder;
    if (configuration.size () > 0)
        previousOrder = configuration.back()->getOrder();
    else
        previousOrder = 0;
    // Generate a new class hierarchy for each new class configuration as long as the order is increasing
    for (classConfiguration *currentClass :  solutions)
    {
        unsigned int order = currentClass->getOrder();
        if (order < previousOrder)
        {
            delete currentClass;
            continue;
        }
        configuration.push_back(currentClass);
        generateHierarchyConfigurations(configuration, count);
        configuration.pop_back();
        delete currentClass;
    }
}

// Main
int main(int argc, char **argv)
{
    configurationTy configuration;
    int count = 0;
    generateHierarchyConfigurations(configuration, &count);
    cout << "Solutions found: " << count << endl;
    return 0;
}
