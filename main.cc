//Main function entry point

#include "cache.h"

using namespace std;

int main(int argc, char *argv[])
{	
	//char a[11];
	char mystr[20];
	char *p;
	string line;
	int i;
	ifstream traceFile;
	string addressFull;
	unsigned int address;

	p = &mystr[0];

	//Command line arguments
	int blockSize = 0;
	int L1cacheSize = 0;
	int L1cacheAssoc = 0;
	int vcBlocks = 0;
	int L2cacheSize = 0;
	int L2cacheAssoc = 0;
	string fileName;

	//Enable signals to be passed to cache L1
	int vcEnable = 0;
	int L2Enable = 0;
	
	if(argc<8){
	cout << "Wrong args." << endl;  
	}
	else{
		blockSize = atoi(argv[1]);	
		L1cacheSize = atoi(argv[2]);		
		L1cacheAssoc = atoi(argv[3]);
		vcBlocks = atoi(argv[4]);
		L2cacheSize = atoi(argv[5]);
		L2cacheAssoc = atoi(argv[6]);
		fileName = argv[7];
	}

	cout << "===== Simulator configuration =====" << "\n";	
	cout << "BLOCKSIZE:\t\t" << blockSize << "\n" 
	     << "L1_SIZE:\t\t" << L1cacheSize <<"\n"
	     << "L1_ASSOC:\t\t" << L1cacheAssoc << "\n"
	     << "VC_NUM_BLOCKS:\t\t" << vcBlocks << "\n" 
	     << "L2_SIZE:\t\t" << L2cacheSize <<"\n"
	     << "L2_ASSOC:\t\t" << L2cacheAssoc << "\n"
	     << "trace_file:\t\t" << fileName << endl;

	if( vcBlocks != 0 )
		{  vcEnable = 1;  }
	else vcEnable = 0;
	if( L2cacheSize != 0 )
		{ L2Enable = 1;	}
	else L2Enable = 0;

	
	//Instantiate cache L1 object with blockSize, cacheSize and cacheAssoc parameters
	 Cache L1 (blockSize, L1cacheSize, L1cacheAssoc, vcBlocks, L2Enable); 


	//Instantiate cache L2 object with blockSize, cacheSize and cacheAssoc parameters
	 CacheL2 L2 (blockSize, L2cacheSize, L2cacheAssoc); 	

	strncpy(mystr, fileName.c_str(), fileName.length());
	mystr[fileName.length()]=0;
	
	/*for(i=0;i<sizeof(mystr);i++)
	{
		cout << mystr[i];
	}
	cout << endl;*/
	
	//Open trace file for reading
	traceFile.open(p);

	//Read r/w address from trace file
	if(traceFile.is_open())
	{
		string sub1, sub2;
		do
		{
			getline(traceFile, addressFull);
			istringstream s(addressFull);
	
			s >> sub1; s >> sub2;
			
			istringstream buffer(sub2);			
			
			buffer >> hex >> address;
			
			//Send r/w address to L1 cache
			L1.Request(sub1.c_str(), address, L2);

		} while (!traceFile.eof());		
	} 

	else cout << "Unable to open input trace file.\n";
	
	traceFile.close();

	L1.printState();

	if ( vcBlocks != 0)
		L1.printVC();
	
 	if(L2cacheSize != 0 )
	  L2.printState();
	
	L1.printStats();
	
	L2.printStats();

	if ( L2cacheSize == 0 )
	cout << "p. total memory traffic:\t\t" << dec << (L1.stats.readMisses + L1.stats.writeMisses + L1.stats.writeBacks - L1.stats.swaps) << '\n';
	else if ( L2cacheSize != 0)
	cout << "p. total memory traffic:\t\t" << dec << (L2.stats2[0].readMisses + L2.stats2[0].writeMisses + L2.stats2[0].writeBacks) << '\n';	
}
