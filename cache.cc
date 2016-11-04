//New cache file.

#include "cache.h"

using namespace std;

/******Constructor to instantiate cache object*****/

Cache::Cache (int a, int b, int c, int d, int e) {
	blockSize = a;
	cacheSize = b;
	cacheAssoc = c;
	vcSize = d;
	L2Enable = e;

	//Cache configuration; Check for FA
	if ( cacheAssoc < (cacheSize/blockSize))
	{
		numWays = cacheAssoc;
		numSets = (cacheSize / ( blockSize * cacheAssoc));
	}
	else if ( cacheAssoc >= (cacheSize/blockSize) )
	{
		numWays = cacheAssoc;
		numSets = 1;
	}

	//Calculate number of bits in TAG, INDEX and OFFSET;check for FA first.
	
	if ( cacheAssoc < (cacheSize/blockSize))
	{
		//OFFSET num of bits
		offsetBits = log2 (blockSize);
		//INDEX num of bits
		indexBits = log2 (cacheSize / ( blockSize * cacheAssoc));	
		//TAG num of bits
		tagBits = 32 - offsetBits - indexBits;
	}
	else if ( cacheAssoc >= (cacheSize/blockSize) )
	{
		offsetBits = log2 (blockSize);
		indexBits = 0;
		tagBits = 32 - offsetBits - indexBits;
	}

	//number of bits for Victim Cache
	//Since it is fully-associative, the number of index bits = 0.
	//OFFSET num of bits
	vcoffsetBits = log2 (blockSize);

	//TAG number of bits
	vctagBits = 32 - vcoffsetBits;
	
	//Initialize 2D array of structs as main cache structure
	cache =  new block*[numSets];                   //Row Count
	for(int c=0; c<numSets; ++c)                    //Row Count
	{
		cache[c] = new block[numWays];           //Column Count 
	}
	
	//Initialize the cacheStatistics structure with zeros.
	stats = { 0, 0, 0, 0, 0, 0 };

	//Make array of dim. numSets * numWays to handle LRU.
	LRU = new int*[numSets];
	for(c=0;c<numSets;++c)
	{
		LRU[c] = new int[numWays];
	}
	
	initCache();
	
	initLRU();

	if (vcSize != 0)
		{  	
		   initVC();
		   initvcLRU();
		}

	accessCount = 0;
}  //Constructor ends here.


//Initialize the cache; Make all block invalid.
void Cache::initCache()
{
		int i, j;

		for(i=0;i<numSets;i++)
			for(j=0;j<numWays;j++)
			{
				cache[i][j].validBit = 0;
			}
} //Init Cache ends here.


void Cache::initLRU()
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

} //initLRU ends here.


void Cache::printState()
{
	int i,j;
	
	cout << '\n' ;
	cout << "===== L1 Contents =====" << endl;

	for(i=0;i<numSets;i++)  {
		cout << "set  " << dec << i << ":\t";
		for(j=0; j<numWays;j++) {			
			cout << hex << ( cache[i][LRU[i][j]].tag << (tagBits + indexBits + offsetBits) );
			if (cache[i][LRU[i][j]].dirtyBit == 1 ) cout << " D\t";
			else cout << '\t';
		}
	cout << '\n' ;		
	}
}  //printState ends here

void Cache::printStats()
{	
	double swapRequestRate = double (stats.swapRequests)/(stats.reads + stats.writes);
	double L1VCMissRate = double(stats.readMisses + stats.writeMisses - stats.swaps)/(stats.reads + stats.writes);

	streamsize default_prec = cout.precision();
	
	cout << '\n';
	cout << "===== Simulation Results =====" << '\n' 
	<< "a. number of L1 reads:\t\t\t" << dec << stats.reads << '\n'
	<< "b. number of L1 read misses:\t\t" << dec << stats.readMisses << '\n'
	<< "c. number of L1 writes:\t\t\t" << dec << stats.writes << '\n'
	<< "d. number of L1 write misses:\t\t" << dec << stats.writeMisses << '\n'
	<< "e. number of swap requests:\t\t" << dec << stats.swapRequests << endl;
	cout.precision(4);
	cout.setf(ios::fixed, ios::floatfield);
	cout << "f. swap request rate:\t\t\t" << swapRequestRate << endl;
	cout.unsetf(ios::floatfield);
	//<< "f. swap request rate:\t\t\t" << setprecision(4) << swapRequestRate << '\n'
	cout << "g. number of swaps:\t\t\t" << dec << stats.swaps << endl;
	cout.setf(ios::fixed, ios::floatfield);
	cout << "h. combined L1+VC miss rate:\t\t" << L1VCMissRate << '\n';
	cout.unsetf(ios::floatfield);
	//<< "h. combined L1+VC miss rate:\t\t" << setprecision(4) << L1VCMissRate << '\n'
	cout << "i. number writebacks from L1/VC:\t" << dec << stats.writeBacks << endl;

	cout.precision(default_prec);
} //printStats ends here

void Cache::updateLRU(unsigned int index, int way)
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

	for(i=k;i>0;i--)
	{
		//Shift the array elements upto the way#
		LRU[index][i] = LRU[index][i-1]; 
	}

	LRU[index][0] = temp;         //Make MRU

} //Update LRU ends here

unsigned int Cache::getLRU(unsigned int index)
{
	unsigned int a;

	a = LRU[index][numWays-1];

	return a;  //Returns the way# which is the current LRU way.
} //getLRU ends here

int Cache::searchVC(unsigned int address)
{
	vctag = address >> (vcoffsetBits);

	int i, found, a;
	a=0;

	for(i=0;i<vcSize;i++)
	{
		if( (vcCache[i].tag == vctag) && (vcCache[i].validBit == 1) )
		{
			found = i; a=1; break;
		}

	}

	if(a == 1)
	{
		return found;
	}
	else if(a == 0)
	return -1;

} //SearchVC ends here

void Cache::initVC()
{
	vcCache = new block[vcSize];

	//Make all VC blocks invalid initially.
	for (int i=0;i<vcSize;i++)
	{
		vcCache[i].validBit = 0;
	}
} //initVC ends here


void Cache::initvcLRU()
{
	vcLRU = new int[vcSize];

	//Make Way0 as the LRU initially, and Way(vcSize-1) as MRU
	int k = vcSize - 1;
	for(int i=0;i<vcSize;i++)
	{
		vcLRU[i] = k;
		k--;
	}
} //initvcLRU ends here

void Cache::updatevcLRU(int way)
{
	int i, k, temp;

	for(i=0;i<vcSize;i++)
	{
		if ( vcLRU[i] == way)
		{ k = i; break;	}
	}

	temp = vcLRU[k];

	for(i=k;i>0;i--)
	{
		vcLRU[i] = vcLRU[i-1];
	}

	vcLRU[0] = temp;
} //updatevcLRU ends here

unsigned int Cache::getvcLRU()
{
	unsigned int a;
	a = vcLRU[vcSize -1];

	return a;  //Returns way# which has the current LRU block.
}

void Cache::writeVC(unsigned int address, int dirtyBit, CacheL2 L2)
{
	unsigned int vclruBlock, vcInvalid, vcEmpty;

	vctag = address >> (vcoffsetBits);

	//Check for invalid block
	for(int i=0;i<vcSize;i++)
	{
		if (vcCache[i].validBit == 0)
		{
			vcInvalid = 1;  
			vcEmpty = i; break;
		}
		else vcInvalid = 0;
	}

	//There is space in the VC
	if (vcInvalid == 1)
	{
		//Write the recieved block.
		vcCache[vcEmpty].tag = vctag;
		vcCache[vcEmpty].validBit = 1;
		vcCache[vcEmpty].dirtyBit = dirtyBit;
		vcCache[vcEmpty].index = 0;

		//Update the VC
		updatevcLRU(vcEmpty);
	}

	//VC is full. Evict the LRU block from VC, write back to L2 if needed.
	else if (vcInvalid == 0)
	{
		vclruBlock = getvcLRU();

		if(vcCache[vclruBlock].dirtyBit == 1)
		{
			unsigned int wbAddress;
			stats.writeBacks++;
		
			//Address of block from VC to be sent to L2.
			wbAddress = (vcCache[vclruBlock].tag) << vcoffsetBits; 

			if( L2Enable == 1 )
				L2.Request('w', wbAddress);

			//Write the received block
			vcCache[vclruBlock].tag = vctag;
			vcCache[vclruBlock].validBit = 1;
			vcCache[vclruBlock].index = 0;
			vcCache[vclruBlock].dirtyBit = dirtyBit;

			//Update VC LRU
			updatevcLRU(vclruBlock);
		}

		else if(vcCache[vclruBlock].dirtyBit == 0)
		{
			vcCache[vclruBlock].tag = vctag;
			vcCache[vclruBlock].validBit = 1;
			vcCache[vclruBlock].index = 0;
			vcCache[vclruBlock].dirtyBit = dirtyBit;

			//Update VC LRU
			updatevcLRU(vclruBlock);
		}

	}
	
} //write VC ends here

void Cache::printVC()
{
	int i;

	cout << '\n' ;
	cout << "===== VC Contents =====" << endl;
	cout << "set 0:" << "   ";

	for(i=0;i<vcSize;i++)
	{
		cout << hex << vcCache[vcLRU[i]].tag;
		if (vcCache[vcLRU[i]].dirtyBit == 1) cout << " D";
		cout << "   ";
	}

	cout << '\n' ;
} //printVC ends here.

//Cache functionality implementation
	
/* Operation and address passed from main
Entry funtion for cache operation.
Request( char *op, int address, CacheL2 obj)
*/

void Cache::Request(const char *c, unsigned int address, CacheL2 L2)  
{
	//Parse address into tag, index. Remove offset.
	 			
	//Index
	unsigned int mask = 4294967295;
	unsigned int a; 	
	
	int i;
	int foundWay;
	int invalidBlock;
	int hit;
	int readOut, writeOut;
	unsigned int lruBlock;    //Get which #way block is LRU at particular index

	accessCount++;
////*************************************** READ request processing *********************************************************////

if( *c == 'r')   //Read request
{
	a = address >> offsetBits;
	mask = 4294967295;
	mask = mask >> (tagBits + offsetBits);
	if (cacheAssoc < (cacheSize/blockSize) )
	{
		index = (address >> offsetBits) & mask;
	}
	else if (cacheAssoc >= (cacheSize/blockSize) )
	{
		index = 0;
	}
	//Tag
	tag = a >> indexBits;
	//Update read count stat
	stats.reads ++;

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
	stats.readMisses++;

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
		
		if( L2Enable == 1 )
			L2.Request('r', address);
				
		cache[index][foundWay].tag = tag;
		cache[index][foundWay].validBit = 1;
		cache[index][foundWay].dirtyBit = 0;
		cache[index][foundWay].index = index;

		//Update LRU
		updateLRU(index, foundWay);
	} 

	//There is no invalid block. Cache is full.
	else if (invalidBlock == 0)
	{
		int vcFound;
		unsigned int swapTemp;
		unsigned int swapAddressL1;
		int swapL1dirtyBit;
		int vcHitWay;
		unsigned int evictAddress;

		//Get which #way block is LRU at particular index. 
		lruBlock = getLRU(index);  //lruBlock holds the Way#

		//There is a VC along with L1
		if (vcSize != 0)
		{		
			vcHitWay = searchVC(address);
			stats.swapRequests++;

			//Swap if VC has the required block.
			/*Steps to swap blocks in L1 and VC
			1. Make L1 address
			2. Store L1 address and dirty bit in temp
			3. Make VC address
			4. Send VC to L1 alongwith dirty bit
			5. Send L1 to VC alongwith dirty bit, write at same swap location.
			6. Update both LRU's to make them the MRU
			*/

			//Swap processing as VC has the required block.
			if (vcHitWay != -1)
			{
				//Update swaps coount
				stats.swaps++;
				//Make L1 address
				swapAddressL1 = (cache[index][lruBlock].tag << (indexBits + offsetBits)) + (cache[index][lruBlock].index << offsetBits);

				//Store L1 address in temp
				swapTemp = swapAddressL1;
				swapL1dirtyBit = cache[index][lruBlock].dirtyBit;

				//Get address from VC and write it in L1. We need the way# where block was found in VC.s				

				cache[index][lruBlock].dirtyBit = vcCache[vcHitWay].dirtyBit;
				cache[index][lruBlock].tag = ((vcCache[vcHitWay].tag) << vcoffsetBits) >> (offsetBits + indexBits);
				cache[index][lruBlock].validBit = 1;
				cache[index][lruBlock].index = index;

				//Write L1 evicted block to VC
				vcCache[vcHitWay].tag = swapAddressL1 >> (vcoffsetBits);
				vcCache[vcHitWay].dirtyBit = swapL1dirtyBit;
				vcCache[vcHitWay].validBit = 1;
				vcCache[vcHitWay].index = 0;

				//Update both the LRU's
				updateLRU(index, lruBlock);
				updatevcLRU(vcHitWay);
			}

			else if (vcHitWay == -1)
			{
				//Address of block to be evicted from L1
				evictAddress = (cache[index][lruBlock].tag << (indexBits + offsetBits)) + (cache[index][lruBlock].index << offsetBits); 

				writeVC(evictAddress, cache[index][lruBlock].dirtyBit, L2);

				if ( L2Enable == 1 )
			       { L2.Request('r', address);   }
			
			//Bring in the block.
			cache[index][lruBlock].tag = tag;
			cache[index][lruBlock].validBit = 1; 
			cache[index][lruBlock].dirtyBit = 0;
			cache[index][lruBlock].index = index;
			//update LRU to make this block the MRU.	
			updateLRU(index, lruBlock);

			}
		} //End of VC present conditions 	
		
		else if( vcSize == 0 )				
		{	
		//Check dirty bit, then WB/evict LRU block
		//Then Bring in the block from next level.

		if( (cache[index][lruBlock].dirtyBit == 1) )  // Check dirty bit. Issue Write if true.
		{	

			//Update write back count.
			stats.writeBacks++;

			//Generate address to write back to L2.
			int wbAddress;
			wbAddress = (cache[index][lruBlock].tag << (offsetBits + indexBits)) + (cache[index][lruBlock].index << offsetBits); 
			
			//Write Back to next level since block is dirty.
			writeOut = 1;
			
			if(L2Enable == 1 )                    
				L2.Request('w', wbAddress);	

			//Now issue a read to next level. Write and then Read.
			if(L2Enable == 1)
			 	L2.Request('r', address);					

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
		
			if(L2Enable == 1 )
				L2.Request('r', address);

			cache[index][lruBlock].tag = tag;
			cache[index][lruBlock].validBit = 1; 
			cache[index][lruBlock].dirtyBit = 0;
			cache[index][lruBlock].index = index;

			//update LRU to make this block the MRU.
			updateLRU(index, lruBlock);
		}

		} //End of VC absent
	} //End of invalid loop
} //End of read miss loop	
} //End of read request loop	

////*************************************** WRITE request processing *********************************************************////

else if( *c == 'w' )   //Write request
{
	//update write count
	stats.writes++;
	a = address >> offsetBits;
	mask = 4294967295;
	mask = mask >> (tagBits + offsetBits);
	if (cacheAssoc < (cacheSize/blockSize))
	{
		index = (address >> offsetBits) & mask;
    }
	else if (cacheAssoc >= (cacheSize/blockSize))
	{
		index = 0;
	}
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
	stats.writeMisses++;

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
		if(L2Enable == 1)
			L2.Request('r', address);	
	
		cache[index][foundWay].tag = tag;
		cache[index][foundWay].validBit = 1;		
		cache[index][foundWay].dirtyBit = 1;
		cache[index][foundWay].index = index;	

		updateLRU(index, foundWay);
	} 
	
	//There is no invalid block. The cache set is full.
	//Evict LRU block. If dirty=1, WB. Issue read to next level. Update LRU.
	else if (invalidBlock == 0 )
	{
		int vcFound;
		unsigned int swapTemp;
		unsigned int swapAddressL1;
		int swapL1dirtyBit;
		int vcHitWay;
		unsigned int evictAddress;

		//Get which #way block is LRU at particular index. 
		lruBlock = getLRU(index);  //lruBlock holds the Way#

		if (vcSize != 0)
		{		
			stats.swapRequests++;
			vcHitWay = searchVC(address);

			if (vcHitWay != -1)  
			{
				stats.swaps++;
				//Make L1 address
				swapAddressL1 = (cache[index][lruBlock].tag << (indexBits + offsetBits)) + (cache[index][lruBlock].index << (offsetBits));

				//Store L1 address in temp
				swapTemp = swapAddressL1;
				swapL1dirtyBit = cache[index][lruBlock].dirtyBit;

				//Get address from VC and write it in L1. We need the way# where block was found in VC.s				

				//Set dirty bit since we are writing to L1.
				cache[index][lruBlock].dirtyBit = 1;
				cache[index][lruBlock].tag = tag;
				cache[index][lruBlock].validBit = 1;
				cache[index][lruBlock].index = index;

				//Write L1 evicted block to VC
				vcCache[vcHitWay].tag = swapAddressL1 >> (vcoffsetBits);
				vcCache[vcHitWay].dirtyBit = swapL1dirtyBit;
				vcCache[vcHitWay].validBit = 1;
				vcCache[vcHitWay].index = 0;

			
				//Update both the LRU's
				updateLRU(index, lruBlock);
				updatevcLRU(vcHitWay);
			}

			//VC does not have the required block. Send evicted block to VC.
			else if (vcHitWay == -1)
			{
				
				evictAddress = (cache[index][lruBlock].tag << (indexBits + offsetBits)) + (cache[index][lruBlock].index << (offsetBits));
				writeVC(evictAddress, cache[index][lruBlock].dirtyBit, L2);
				
				if ( L2Enable == 1 )
			        L2.Request('r', address);
			
			//Bring in the block.
			cache[index][lruBlock].tag = tag;
			cache[index][lruBlock].validBit = 1; 
			cache[index][lruBlock].dirtyBit = 1;
			cache[index][lruBlock].index = index;
			//update LRU to make this block the MRU.	
			updateLRU(index, lruBlock);

			}
		}	

	else if (vcSize == 0)
	{	
		//Check dirty bit, then WB/evict LRU block.
		//Then Bring in from next level.
		if((cache[index][lruBlock].dirtyBit == 1))  // Check dirty bit. Issue Write to next level.
		{	
			//Issue a write to the next level.
			writeOut = 1;	

			//Update writebacks count
			stats.writeBacks ++;	

			int wbAddress;
			wbAddress = (cache[index][lruBlock].tag << (offsetBits + indexBits)) + (cache[index][lruBlock].index << offsetBits); 

			if( L2Enable == 1 )
				L2.Request('w', wbAddress);	

			//Issue a read to next level.
			if( L2Enable == 1 )
				L2.Request('r', address);
			
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
			if(L2Enable == 1 )
				L2.Request('r', address);

			cache[index][lruBlock].tag = tag;
			cache[index][lruBlock].validBit = 1; 
			cache[index][lruBlock].dirtyBit = 1;
			cache[index][lruBlock].index = index;
			
			//Update LRU
			updateLRU(index, lruBlock);
		}
	} //End of VC absent

	} //End of invalid loop
} //End of write miss loop	

} //End of write request loop

}//End of Request loop.
