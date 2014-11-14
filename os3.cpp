#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/mman.h>
#include <cstdlib>
#include <sys/stat.h> 
#include <fcntl.h>
#include <sstream>
#include <stdexcept> 

using namespace std;


//parsea the data here
template<typename T>
T ParseInteger(const uint8_t* const ptr) 
{ 
	 T val = 0; 
	 
	 for (size_t i=0; i<sizeof(T); ++i) { 
	 val |= static_cast<T>(static_cast<T>(ptr[i]) << (i*8)); 
	 } 
	 
	 return val; 
}

template<typename T, const T...tArgs> 
T parseInteger(const uint8_t* const ptr) 
{ 
	 T val = 0; 
	 for (size_t i=0; i<sizeof(T); ++i) { 
	 val |= static_cast<T>(static_cast<T>(ptr[i]) << (i*8)); 
	 } 
	 
	 constexpr auto valid_vals = initializer_list<T>({tArgs...}); 
	 
	 if (valid_vals.size() == 0) return val; 
	 
	 for (auto &x: valid_vals) 
	 if (x == val) return val; 
	 
	 throw exception(); 
}

 
template<typename T, typename...V> 
T parseInteger(const uint8_t* const ptr, V&&...args) 
{ 
	 T val = parseInteger<T>(ptr); 
	 
	 for (auto j: {args...}) 
	 if (!j(val)) { 
	 stringstream ss; 
	 ss << "error parsing value" << val << endl; 
	 throw runtime_error(ss.str()); 
	 } 
	 
	 return val; 
} 

//done parsing the data

//start fsinfo

void fsinfo(uint16_t BPB_BytsPerSec, uint8_t BPB_SecPerClus, uint32_t BPB_TotSec32, uint8_t BPB_NumFATs, uint32_t BPB_FATSz32, uint32_t FSI_Free_Count)
{
	//print following things

		//Bytes per Sector
			cout << "The Bytes per Sector are: " << BPB_BytsPerSec << endl;
		//Sectors per Cluster
			cout << "The Sectors per Cluster are: " << int(BPB_SecPerClus) << endl;
		//Total Sectors
			cout << "The Total Sectors are: " << BPB_TotSec32 << endl;
		//Number of FATs
			cout << "The Number of FATs are: " <<  int(BPB_NumFATs) << endl;
		//Sectors per FAT
			cout << "The Sectors per FAT are: " <<  BPB_FATSz32 << endl;
		//Number of free sectors
			cout << "The Number of Free Sectors are: " << FSI_Free_Count << endl;
}
//end fsinfo


//****main starts here****
int main(int argc, char *argv[])
{

if(argc != 2){
	cout << "ya dun goofed!" << endl;
	cout << "The consequences will never be the same!" << endl;
	return 1;
}

int fd = open(argv[1], O_RDONLY);
if (fd < 0) { 
 cerr << "error opening file" << endl; 
 exit(EXIT_FAILURE); 
} 
 
int offset = 0; 
unsigned len = 4096; 
auto fdata = (uint8_t*)mmap 
 (0, len, PROT_READ, MAP_PRIVATE, fd, offset); 

uint16_t BPB_BytsPerSec = parseInteger<uint16_t, 512, 1024, 2048, 4096>(fdata + 11);
uint8_t BPB_SecPerClus = parseInteger<uint8_t>(fdata + 13, [](uint8_t v){return v!=0;});
uint32_t BPB_TotSec32 = parseInteger<uint32_t>(fdata + 32, [](uint32_t v){return v!=0;});
uint8_t BPB_NumFATs = parseInteger<uint8_t>(fdata + 16, [](uint8_t v){return v!=0;});
uint32_t BPB_FATSz32 = ParseInteger<uint32_t>(fdata + 36);
uint16_t BPB_FSInfo = ParseInteger<uint16_t>(fdata + 48);
uint32_t FSI_Free_Count = ParseInteger<uint32_t>(fdata + BPB_FSInfo*BPB_BytsPerSec + 488);

fsinfo( BPB_BytsPerSec, BPB_SecPerClus, BPB_TotSec32, BPB_NumFATs, BPB_FATSz32, FSI_Free_Count);

//not closing file because we aint pussy bitches, the OS is my dawg he got it on lock   <----do not remove
//first things first need to make the filesystem work


// then after that we can implement funtions to work on the data

return 0;

}
