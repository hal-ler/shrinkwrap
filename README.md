# ShrinkWrap
Everything related to the ShrinkWrap paper from ACSAC 2015

Paper : https://www.acsac.org/2015/openconf/modules/request.php?module=oc_program&action=summary.php&id=96
Archived PDF of paper: http://www.cs.vu.nl/~herbertb/papers/shrinkwrap_acsac2015.pdf

## Patches:  
0001-Fixed-call-site-type-inference.patch : fixes call-site type inference in VTV (GCC 4.9.2).	
0002-Extended-protection.patch : applies the extended policy to VTV (after patch 0001).  
0003-Fine-grained-protection.patch : applies the fine-grained policy to VTV (after patch 0002).  
0004-LibVTV-extension-for-micro-benchark.patch : generates debug output for VTV (for microbenchmark) (GCC 4.9.2).  
clang-cfi-debug.diff : generates debug output for CFI-VPTR (for microbenchmark) (LLVM 3.8, not tested recently).  

## Microbenchmark - classtester:  
Build the microbenchmark using "make".  
Run the microbenchmark using:  
  * "make -f Makefile-gen prepare" : generate class hierarchies  
  * "make -f Makefile-gen gen": compiles all class hierarchies  
  * "make -f Makefile-gen check": run time compiled class hierarchies and observe behavior  

Configurations:  
  * codegenerator.cpp - defines  
    * CLASSES : maximum number of classes in the hierarchy (recommended =<6)  
    * PARENTS_PER_CLASS : maximum number of direct base classes for each class (recommended <= 3)  
    * RANDOM_VARIANTS : the number of random variants to used when radomness is involved  
    * DO_DIAMOND : allow/disallow diamond inheritance  
    * DO_ALLOVERRIDE : sub-classes override all base class methods (none are overrided by default)  
    * DO_RANDOMOVERRIDE : sub-classes randomly chose to override or not each base class method  
  * Makefile-gen - VARIANT variable  
    * Choose the compiler/vtable protection to test.  
    * vtv : VTV from GCC  
    * llvm : Clang CFI-VPTR  
  * Makefile-gen - checkers in czero target  
    * TYPECHECKER : checks that each call-site uses the right vtable set (optional, slow).  
    * ILLEGALCHECKER : checks that no call-site can target different method families (by name) (optional, slow).  
    * MAPCHECKER : checks if each vtable set at every call-site has all its elements used (baseline, fast).  
    * MAPEVAL : counts up the vtables targets covered from each set across all samples (recommended, fast).  

## Proof of concepts:  
poc.cpp : attack hijacking vtable pointer protected by GCC VTV.  
poc-clang-cfi.cpp : attack hijacking vtable pointer protected by Clang CFI-VPTR.  

## Updating microbenchmark to support custom vtable protection  
MAPCHECKER : Every check should print the following items on a line separated by spaces.  
  * Instruction pointer at check/call-site (or another unique identifier for location).  
  * Vtable pointer being used (or another unique identifier).  
  * Pointer to allowed vtable set (or another unique identifier for set).  
  * Size of allowed vtable set.  

TYPECHECKER : Access a specifically named variable at every virtual call-site.
  * VTV<classname> mangled C++ variable, where classname is the static class type of the call-site.
  * Alternatively just update the script with specific static binary analysis.  

ILLEGALCHECKER : Update the script to extract the information statically.  
  * get_vtable_entries_X : Extract the vtable entries corresponding to a given set (identified by class index).  
  * get_vtable_offset_X : Extract vtable offset used by a particular call-site.  
