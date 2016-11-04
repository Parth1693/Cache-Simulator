//L2 cache description

#include "cache.h"

using namespace std;
		
/******Constructor to instantiate cache object*****/
CacheL2::CacheL2 (int a, int b, int c) {
	blockSize = a;
	cacheSize = b;
	cacheAssoc = c;

	//Cache configuration
	if ( cacheSize !=0 )
	{
		numWays = cacheAssoc;
		numSets = (cacheSize / ( blockSize * cacheAssoc));
	}
	else
	{
		numWays = 1;
		numSets = 1;
	}

	//Calculate number of bits in TAG, INDEX and OFFSET
	
	if ( cacheSize !=0 )
	{
		//OFFSET num of bits
		offsetBits = log2 (blockSize);
		//INDEX num of bits
		indexBits = log2 (cacheSize / ( blockSize * cacheAssoc));	
		//TAG num of bits
		tagBits = 32 - offsetBits - indexBits;
	}
	else {
		offsetBits = 4;
		indexBits = 1;
		tagBits = 1;
	}
	
	//Initialize 2D array of structs as main cache structure
	cache =  new block*[numSets];                   //Row Count
	for(int c=0; c<numSets; ++c)                    //Row Count
	{
		cache[c] = new block[numWays];           //Column Count 
	}

	//Make array of dim. numSets * numWays to handle LRU.
	LRU = new int*[numSets];
	for(c=0;c<numSets;++c)
	{
		LRU[c] = new int[numWays];
	}
	
	initCache();
	
	initLRU();

}  //Constructor ends here.
	

//Initialize the cache; Make all block invalid.
void CacheL2::initCache()
	{
		int i, j;

		for(i=0;i<numSets;i++)
			for(j=0;j<numWays;j++)
			{
				cache[i][j].validBit = 0;
			}
		
		stats2 = new cacheStats[2];
		stats2[0] = {0,0,0,0,0,0,0,0};

		L2access =  new int[2];
		
		L2access[0] = 0;
	}

/* To initialize the LRU array.
The size of each LRU counter is equal to the number of ways in the cache.
By default, make Way #0 as LRU, Way #1 as LRU-1 and so on.
Args are LRU array, number of sets, and number of ways
*/

void CacheL2::initLRU()
{
	int i, j, k;
	
	for(i=0;i<numSets;i++)
	{
		k = numWays - 1;
		for(j=0;j<numWays;j++)
		{
			LRU[i][j] = k;
			k--;
		}
	}
}
/*Function to print out the contents of the cache at the end of a trace run.
While putting out the cache contents on the screen, MRU first followed by next frequent members till the LRU.
Output should be in HEX. Dirt bit wriiten as 'D'
*/

void CacheL2::printState()
{
	int i,j;
	
	cout << '\n';
	cout << "===== L2 Contents =====" << endl;

	for(i=0;i<numSets;i++)  {
		cout << "set  " << dec << i << ":\t";
		for(j=0; j<numWays;j++) {			
			cout << hex << ( cache[i][LRU[i][j]].tag << (tagBits + indexBits + offsetBits) );
			if (cache[i][LRU[i][j]].dirtyBit == 1 ) cout << " D\t";
			else cout << '\t';
		}
	cout << endl;		
	}
}

void CacheL2::printStats()
{	 
	double L2MissRate = double(stats2[0].readMisses)/(stats2[0].reads);

	streamsize default_prec = cout.precision();

	cout << "j. number of L2 reads:\t\t\t" << dec << stats2[0].reads << '\n'
	<< "k. number of L2 read misses:\t\t" << dec << stats2[0].readMisses << '\n'
	<< "l. number of L2 writes:\t\t\t" << dec << stats2[0].writes << '\n'
	<< "m. number of L2 write misses:\t\t" << dec << stats2[0].writeMisses << '\n';
	if ( cacheSize != 0)
	{	cout << "n. L2 miss rate:\t\t\t" << setprecision(4) << L2MissRate << '\n'; }
	else { cout << "n. L2 miss rate:\t\t\t" <<  "0.0000" << '\n'; }
	cout << "o. number of writebacks from L2:\t" << dec << stats2[0].writeBacks << '\n';

	cout.precision(default_prec);
}

//LRU functionality array
/*
We store the #way number which is MRU, LRU etc
LRU[0] : MRU
LRU[numWays -1] : LRU
Something is accessed, shift down the present elements by 1. LRU[i+1]=LRU[i];
To evict a block, choose LRU[numWays - 1], which is the last element;
To update, move recently accessed block to front, rest all shift down by one place in array.
LRU[numSets][numWays];
Access LRU[index][0:numWays-1]; //For each set
API: updateLRU(index);
getLRU(index);
*/

/* To update the LRU array.
Index is the set number which is being accessed.
Way is the way# which is to be made as the MRU
*/

void CacheL2::updateLRU(unsigned int index, int way)
{
	int i, k, temp;

	//Search LRU[][]

	for(i=0;i<numWays;i++)
	{
		if(LRU[index][i] == way)
		{
			k = i; 
			break;
		}
	}

	temp = LRU[index][k];
	//cout << "k is " << k << " temp is " << temp << endl;

	for(i=k;i>0;i--)
	{
		//Shift the array elements upto the way#
		LRU[index][i] = LRU[index][i-1]; 
	}

	LRU[index][0] = temp;         //Make MRU
}

/* To read the current LRU element.
This is the last array index i.e. LRU[index][ways - 1];
*/

unsigned int CacheL2::getLRU(unsigned int index)
{
	unsigned int a;

	a = LRU[index][numWays-1];

	return a;  //Returns the way# which is the current LRU way.
}

//Cache functionality implementation
/* Operation and address passed from main
Entry funtion for cache operation.
Request( char op, int address)
*/

void CacheL2::Request(char c, unsigned int address)  {
	
	//Parse address into tag, index. Remove offset.
	 
	//cout << "Address is " << address << endl;
	 			
	//Index
	unsigned int mask = 4294967295;
	unsigned int a; 	
	
	int i;
	int foundWay;
	int invalidBlock;
	int hit;
	int readOut, writeOut;
	unsigned int lruBlock;    //Get which #way block is LRU at particular index

	L2access[0]++;

////*************************************** READ request processing *********************************************************////

if( c == 'r')   //Read request
{
	a = address >> offsetBits;
	mask = 4294967295;
	mask = mask >> (tagBits + offsetBits);
	index = (address >> offsetBits) & mask;
	//Tag
	tag = a >> indexBits;		

	//Update read count stat
	stats2[0].reads =  stats2[0].reads + 1;

	//Search the cache: Search particular cache index set. Consider Validbit also.
	for(int i=0;i<numWays;i++)
	{
		//Condition for Hit
		if( (cache[index][i].validBit == 1) && (cache[index][i].tag == tag) )   
//Read Hit
		{	hit = 1; 
			foundWay = i;	
			break;   }
		else hit = 0;
	}

	//foundWay holds the way# where block was found.
	if(hit == 1)    
	{	
		//Update LRU for particular index; make it MRU
		updateLRU(index, foundWay);
	}

//Read Miss

else if(hit == 0) 
	{
	// update miss count 
	stats2[0].readMisses++;

	//Determine if there is Invalid Block in that particular set. 
	for(i=0;i<numWays;i++)
	{
		if( (cache[index][i].validBit == 0) )     //Condition for Invalid block i.e. there is empty space in cache set
		{	invalidBlock = 1;
			foundWay = i;
			break;	}
		else invalidBlock = 0;
	}	
	
	//There is invalid block in the set.
	//Issue read to next level. Bring in the requested block in cache. foundWay holds the Way# which is empty.
	if( invalidBlock == 1 )
	{
		//Issue read to next level
		readOut = 1;  
				
		cache[index][foundWay].tag = tag;
		cache[index][foundWay].validBit = 1;
		cache[index][foundWay].dirtyBit = 0;
		cache[index][foundWay].index = index;
		//Update LRU
		updateLRU(index, foundWay);
	} 

	//There is no invalid block. Cache is full.
	//Evict LRU block. If dirty=1, issue Write to next level.
	else if (invalidBlock == 0)
	{
		//Get which #way block is LRU at particular index. 
		lruBlock = getLRU(index);  //lruBlock holds the Way#
		
		//Check dirty bit, then WB/evict LRU block
		//Then Bring in the block from next level.

		if( (cache[index][lruBlock].dirtyBit == 1) )  // Check dirty bit. Issue Write if true.
		{	

			//Update write back count.
			stats2[0].writeBacks++;

			//Generate address to write back to L2.
			int wbAddress;
			wbAddress = (cache[index][lruBlock].tag << (offsetBits + indexBits)) + (cache[index][lruBlock].index << offsetBits); 
			
			//Write Back to next level since block is dirty.
			writeOut = 1;
			
			//Now issue a read to next level. Write and then Read.
			readOut = 1;
		

			//Bring in the block.
			cache[index][lruBlock].tag = tag;
			cache[index][lruBlock].validBit = 1; 
			cache[index][lruBlock].dirtyBit = 0;
			cache[index][lruBlock].index = index;
			//update LRU to make this block the MRU.	
			updateLRU(index, lruBlock);
		}

		else if((cache[index][lruBlock].dirtyBit == 0)) 
		//Only Bring in the block from next level.
		{
			//Issue a read to next level.
			readOut = 1;

			cache[index][lruBlock].tag = tag;
			cache[index][lruBlock].validBit = 1; 
			cache[index][lruBlock].dirtyBit = 0;
			cache[index][lruBlock].index = index;
			//update LRU to make this block the MRU.
			updateLRU(index, lruBlock);
		}
		
	}
	
	}  //End Read Miss loop
		 	
}  //End of Read request loop				
	
////*************************************** WRITE request processing *********************************************************////

else if( c == 'w' )   //Write request
{
	//update write count
	stats2[0].writes++;

	a = address >> offsetBits;
	mask = 4294967295;
	mask = mask >> (tagBits + offsetBits);
	index = (address >> offsetBits) & mask;
	//Tag
	tag = a >> indexBits;	

//Search the cache: Search particular cache index set.
	for(int i=0;i<numWays;i++)
	{
		if( (cache[index][i].validBit == 1) && (cache[index][i].tag == tag) )   //Condition for Hit
		{	hit = 1;  foundWay = i;
			break;	}	
		else hit = 0;
	}
	
	//foundWay holds the #Way where block is present.
	//Make the block dirty.
//Write hit	
	if(hit == 1)
	{
		cache[index][foundWay].tag = tag;
		cache[index][foundWay].validBit = 1; 
		cache[index][foundWay].dirtyBit = 1;
		cache[index][foundWay].index = index;
	
		//Update LRU
		updateLRU(index, foundWay);
	}
	
//Write Miss
	else if(hit == 0) 
	{

	//Update Write miss count 
	stats2[0].writeMisses++;

	//Determine if there is Invalid Block in that particular set. 
	for(i=0;i<numWays;i++)
	{
		if((cache[index][i].validBit == 0))     //Condition for Invalid block i.e. empty space in cache set
		{	invalidBlock = 1;
			foundWay = i;   break;	}
		else invalidBlock = 0;
	}		
	
	//If there is invalid block, bring in the block in that #way.
	// Set Dirty Bit=1.
	if( invalidBlock == 1 )
	{
		//Issue a read to the next level.
		readOut = 1;
	
		cache[index][foundWay].tag = tag;
		cache[index][foundWay].validBit = 1;
		cache[index][foundWay].index = index;
		cache[index][foundWay].dirtyBit = 1;	
	
		//printLRU();
		updateLRU(index, foundWay);
	} 
	
	//There is no invalid block. The cache set is full.
	//Evict LRU block. If dirty=1, WB. Issue read to next level. Update LRU.
	else if (invalidBlock == 0 )
	{
		lruBlock = getLRU(index);  //lruBlock holds the Way# which is LRU.
		
		//Check dirty bit, then WB/evict LRU block.
		//Then Bring in from next level.
		if((cache[index][lruBlock].dirtyBit == 1))  // Check dirty bit. Issue Write to next level.
		{	
			//Issue a write to the next level.
			writeOut = 1;	

			//Update writebacks count
			stats2[0].writeBacks ++;	

			int wbAddress;
			wbAddress = (cache[index][lruBlock].tag << (offsetBits + indexBits)) + (cache[index][lruBlock].index << offsetBits); 

			//Issue a read to next level.
			readOut = 1;
			
			cache[index][lruBlock].tag = tag;
			cache[index][lruBlock].validBit = 1; 
			cache[index][lruBlock].dirtyBit = 1;
			cache[index][lruBlock].index = index;
			
			//Update LRU
			updateLRU(index, lruBlock);
		}

		else if ((cache[index][lruBlock].dirtyBit == 0)) 
		//Issue a read to next level.	
		//Only Bring in the block. Set dirty bit = 1;

		{
			//Issue a read to next level.
			readOut = 1;

			cache[index][lruBlock].tag = tag;
			cache[index][lruBlock].validBit = 1; 
			cache[index][lruBlock].dirtyBit = 1;
			cache[index][lruBlock].index = index;
			
			//Update LRU
			updateLRU(index, lruBlock);
		}		
	
	}

	} 	//end Write Miss loop

  }   //End of Write request loop

} //end of Request function.

