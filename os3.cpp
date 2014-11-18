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

template<typename T, const T...tArgs>
T parseInteger(const uint8_t* const ptr);
template<typename T, typename...V>
T parseInteger(const uint8_t* const ptr, V&&...args);
void fsinfo(uint16_t, uint8_t, uint32_t, uint8_t, uint32_t, uint32_t);

//****main starts here****
int main(int argc, char *argv[])
{
	string inputstring;
	string arg1;
	string arg2;
	string arg3;
	string arg4;
	
	if(argc != 2){
		cout << "Improper number of arguments" << endl;
		cout << "Must type program name followed by image file name" << endl;
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
	uint32_t BPB_FATSz32 = parseInteger<uint32_t>(fdata + 36);
	uint16_t BPB_FSInfo = parseInteger<uint16_t>(fdata + 48);
	uint32_t FSI_Free_Count = parseInteger<uint32_t>(fdata + BPB_FSInfo*BPB_BytsPerSec + 488);

	// done parsing the data
	
	//first things first need to make the filesystem work
	
	while (1)
	{
		cout << "[" << argv[1] << "]> ";
		getline(cin, inputstring);
		
		arg1 = inputstring.substr(0, inputstring.find_first_of(" "));
		
		if (arg1.compare("q") == 0)
		{
			return 0;
		}
		
		if (arg1.compare("fsinfo") == 0)
		{
			fsinfo( BPB_BytsPerSec, BPB_SecPerClus, BPB_TotSec32,
			BPB_NumFATs, BPB_FATSz32, FSI_Free_Count);
		}
		
		else if (arg1.compare("open") == 0)
		{
			cout << "user entered " << arg1 << endl;
		}
		
		else if (arg1.compare("close") == 0)
		{
			cout << "user entered " << arg1 << endl;
		}
		
		else if (arg1.compare("create") == 0)
		{
			cout << "user entered " << arg1 << endl;
		}
		
		else if (arg1.compare("read") == 0)
		{
			cout << "user entered " << arg1 << endl;
		}
		
		else if (arg1.compare("write") == 0)
		{
			cout << "user entered " << arg1 << endl;
		}
		
		else if (arg1.compare("rm") == 0)
		{
			cout << "user entered " << arg1 << endl;
		}
		
		else if (arg1.compare("cd") == 0)
		{
			cout << "user entered " << arg1 << endl;
		}
		
		else if (arg1.compare("ls") == 0)
		{
			cout << "user entered " << arg1 << endl;
		}
		
		else if (arg1.compare("mkdir") == 0)
		{
			cout << "user entered " << arg1 << endl;
		}
		
		else if (arg1.compare("rmdir") == 0)
		{
			cout << "user entered " << arg1 << endl;
		}
		
		else if (arg1.compare("size") == 0)
		{
			cout << "user entered " << arg1 << endl;
		}
		
		else if (arg1.compare("undelete") == 0)
		{
			cout << "user entered " << arg1 << endl;
		}
		
		else
		{
			cout << arg1 << " is an invalid command" << endl;
		}
	}
	
	// then after that we can implement funtions to work on the data
}

// We didn't need the first ParseInteger (with the capital "P"), since the
// second one has a case for when no valid values are specified.

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

void fsinfo(uint16_t BPB_BytsPerSec, uint8_t BPB_SecPerClus, uint32_t BPB_TotSec32, uint8_t BPB_NumFATs, uint32_t BPB_FATSz32, uint32_t FSI_Free_Count)
{
	//print summary of file system values

	cout << "Bytes per sector: " << BPB_BytsPerSec << endl;
	cout << "Sectors per cluster: " << int(BPB_SecPerClus) << endl;
	cout << "Total sectors: " << BPB_TotSec32 << endl;
	cout << "Number of FATs: " <<  int(BPB_NumFATs) << endl;
	cout << "Sectors per FAT: " <<  BPB_FATSz32 << endl;
	cout << "Number of free sectors: " << FSI_Free_Count << endl;
}
//end fsinfo
