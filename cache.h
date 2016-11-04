#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iomanip>

#define DEBUG 0

	//Struct class members to build cache. 2D array of blocks. 
		
	//Struct for a memory block.
	typedef struct {
		int validBit;		     //validBit == 1 is valid.
		int dirtyBit;
		unsigned int tag;		 //Decimal value of tag and index.	
		unsigned int index;     
	} block;
			
	//Struct to hold access statistics for cache.
	typedef struct  {
		int reads;
		int readMisses;
		int writes;
		int writeMisses;
		int missRate;
		int writeBacks;
		int swapRequests;
		int swaps;
		
	} cacheStats;	

class CacheL2 {
	public:
	int blockSize;
	int cacheSize;
	int cacheAssoc;
			
	int numSets;
	int numWays;
	unsigned int offsetBits;
	unsigned int indexBits;
	unsigned int tagBits;
	unsigned int address;
	unsigned int index;
	unsigned int tag;
	int *L2access;
	
	//block struct to hold information for each memory block in cache.
	block **cache;   			

	//cacheStats struct to hold run statistics.
	cacheStats *stats2;

	//Array to hold LRU information
	int **LRU;

	//Member functions

	CacheL2 (int, int, int);    //Constructor
	void initCache();
	void Request(char c, unsigned int address);
	void initLRU();
	void updateLRU(unsigned int index, int way);
	unsigned int getLRU(unsigned int index);
	void printState();	
	void printStats();
	void printLRU();
	//void freeMemory();

};   //Class CacheL2 ends here.

class Cache {
	public:
	int blockSize;
	int cacheSize;
	int cacheAssoc;
	int vcSize;   //=vcBlocks from main.cc
	int L2Enable;
		
	int numSets;
	int numWays;
	unsigned int offsetBits;
	unsigned int indexBits;
	unsigned int tagBits;
	unsigned int address;
	unsigned int index;
	unsigned int tag;

	unsigned int vctagBits;
	unsigned int vcindexBits;
	unsigned int vcoffsetBits;
	unsigned int vctag;
	unsigned int vcindex; 

	int accessCount;
	
	//block struct to hold information for each memory block in cache.
	block **cache;   	

	//block struct to hold VC information. 1-D
	block *vcCache;		

	//cacheStats struct to hold run statistics.
	cacheStats stats;

	//Array to hold LRU information for L1
	int **LRU;

	//Array to hold LRU information for VC
	int *vcLRU;
	
	//Member functions

	Cache (int, int, int, int, int);    //Constructor
	void initCache();
	void Request(const char *c, unsigned int address, CacheL2 L2);
	void initLRU();
	void updateLRU(unsigned int index, int way);
	unsigned int getLRU(unsigned int index);
	void printState();	
	void printStats();	
	//void printLRU();

	void initVC();
	void initvcLRU();
	void updatevcLRU(int way);
	void writeVC(unsigned int, int, CacheL2);
	unsigned int getvcLRU();
	int searchVC(unsigned int);
	void printVC();
	//void freeMemory();

};   //Class Cache ends here.


/*Header file ends here*/

