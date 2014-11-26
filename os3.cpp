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
#include <vector>

using namespace std;

const size_t NPOS = string::npos;
const string READ = "r";
const string WRITE = "w";
const string READWRITE = "rw";
vector<string> filenamevector;
vector<string> modevector;
vector<uint32_t> dirvector;
uint8_t *fdata;
uint32_t curdir;
uint16_t BPB_BytsPerSec;
uint8_t BPB_SecPerClus;
uint16_t BPB_RsvdSecCnt;
uint8_t BPB_NumFATs;
uint16_t BPB_RootEntCnt;
uint16_t BPB_TotSec16;
uint16_t BPB_FATSz16;
uint32_t BPB_TotSec32;
uint32_t BPB_FATSz32;
uint32_t BPB_RootClus;
uint16_t BPB_FSInfo;
uint32_t FSI_Free_Count;
unsigned int RootDirSectors;
unsigned int FATSz;
unsigned int TotSec;
unsigned int DataSec;
unsigned int CountofClusters;
unsigned int FirstDataSector;

template<typename T, const T...tArgs>
T parseInteger(const uint8_t* const ptr);
template<typename T, typename...V>
T parseInteger(const uint8_t* const ptr, V&&...args);
bool parse_input(const string, string&);
bool parse_input(const string, string&, string&);
bool parse_input(const string, string&, string&, string&);
bool parse_input_quoted_data(const string, string&, string&, string&);
uint32_t find_dir(string, uint32_t);
void fat32_fsinfo();
// void fat32_open();
void fat32_close(string);
// void fat32_create();
// void fat32_read();
// void fat32_write();
// void fat32_rm();
void fat32_cd(string);
void fat32_ls(string);
// void fat32_mkdir();
// void fat32_rmdir();
// void fat32_size();
// void fat32_undelete();

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
		cout << "Must type program name and image file name" << endl;
		return 1;
	}
	
	int fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		cerr << "error opening file" << endl;
		exit(EXIT_FAILURE);
	}
	
	int offset = 0;
	unsigned len = 4096;
	fdata = (uint8_t*)mmap(0, len, PROT_READ, MAP_PRIVATE, fd, offset);
	
	BPB_BytsPerSec = parseInteger<uint16_t, 512, 1024, 2048, 4096>
	(fdata + 11);
	BPB_SecPerClus = parseInteger<uint8_t>
	(fdata + 13, [](uint8_t v){return v!=0;});
	BPB_RsvdSecCnt = parseInteger<uint16_t>
	(fdata + 14, [](uint16_t v){return v!=0;});
	BPB_NumFATs = parseInteger<uint8_t>
	(fdata + 16, [](uint8_t v){return v!=0;});
	BPB_RootEntCnt = parseInteger<uint16_t>(fdata + 17);
	BPB_TotSec16 = parseInteger<uint16_t>(fdata + 19);
	BPB_FATSz16 = parseInteger<uint16_t>(fdata + 22);
	BPB_TotSec32 = parseInteger<uint32_t>(fdata + 32);
	BPB_FATSz32 = parseInteger<uint32_t>(fdata + 36);
	BPB_RootClus = parseInteger<uint32_t>(fdata + 44);
	BPB_FSInfo = parseInteger<uint16_t>(fdata + 48);
	FSI_Free_Count = parseInteger<uint32_t>(fdata + BPB_FSInfo*BPB_BytsPerSec + 488);

	// done parsing the data
	
	// check whether image file is FAT32 or not
	RootDirSectors = ((BPB_RootEntCnt * 32) + (BPB_BytsPerSec - 1))
	/ BPB_BytsPerSec;
	
	if (BPB_FATSz16 != 0)
		FATSz = BPB_FATSz16;
	else
		FATSz = BPB_FATSz32;
	
	if (BPB_TotSec16 != 0)
		TotSec = BPB_TotSec16;
	else
		TotSec = BPB_TotSec32;
	
	DataSec = TotSec - (BPB_RsvdSecCnt + (BPB_NumFATs * FATSz)
	+ RootDirSectors);
	CountofClusters = DataSec / BPB_SecPerClus;
	
	if (CountofClusters < 65525)
	{
		// volume is not FAT32, terminate program
		cout << "Error: image file is not FAT32" << endl;
		return 1;
	}
	
	// calculate the first data sector
	FirstDataSector = BPB_RsvdSecCnt + (BPB_NumFATs * FATSz);
	
	// set initial directory to root directory
	curdir = BPB_RootClus;
	// FirstSectorofCluster = ((N - 2) * BPB_SecPerClus) + FirstDataSector;
	// (N = valid data cluster number)
	
	// prints values on FAQ page
	/*cout << BPB_BytsPerSec << endl << (int)BPB_SecPerClus << endl
	<< BPB_RsvdSecCnt << endl << (int)BPB_NumFATs << endl << BPB_FATSz16
	<< endl << BPB_FATSz32 << endl << BPB_RootClus << endl;*/
	
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
			fat32_fsinfo();
		}
		
		else if (arg1.compare("open") == 0)
		{
			cout << "user entered " << arg1 << endl;
			if (parse_input(inputstring, arg2, arg3))
			{
				// fat32_open();
			}
		}
		
		else if (arg1.compare("close") == 0)
		{
			cout << "user entered " << arg1 << endl;
			if (parse_input(inputstring, arg2))
			{
				fat32_close(arg2);
			}
		}
		
		else if (arg1.compare("create") == 0)
		{
			cout << "user entered " << arg1 << endl;
			if (parse_input(inputstring, arg2))
			{
				// fat32_compare();
			}
		}
		
		else if (arg1.compare("read") == 0)
		{
			cout << "user entered " << arg1 << endl;
			if (parse_input(inputstring, arg2, arg3, arg4))
			{
				// fat32_read();
			}
		}
		
		else if (arg1.compare("write") == 0)
		{
			cout << "user entered " << arg1 << endl;
			if (parse_input_quoted_data(inputstring, arg2, arg3,
			arg4))
			{
				// fat32_write();
			}
		}
		
		else if (arg1.compare("rm") == 0)
		{
			cout << "user entered " << arg1 << endl;
			if (parse_input(inputstring, arg2))
			{
				// fat32_rm();
			}
		}
		
		else if (arg1.compare("cd") == 0)
		{
			cout << "user entered " << arg1 << endl;
			if (parse_input(inputstring, arg2))
			{
				// fat32_cd();
			}
		}
		
		else if (arg1.compare("ls") == 0)
		{
			cout << "user entered " << arg1 << endl;
			if (parse_input(inputstring, arg2))
			{
				fat32_ls(arg2);
			}
		}
		
		else if (arg1.compare("mkdir") == 0)
		{
			cout << "user entered " << arg1 << endl;
			if (parse_input(inputstring, arg2))
			{
				// fat32_mkdir();
			}
		}
		
		else if (arg1.compare("rmdir") == 0)
		{
			cout << "user entered " << arg1 << endl;
			if (parse_input(inputstring, arg2))
			{
				// fat32_rmdir();
			}
		}
		
		else if (arg1.compare("size") == 0)
		{
			cout << "user entered " << arg1 << endl;
			if (parse_input(inputstring, arg2))
			{
				// fat32_size();
			}
		}
		
		else if (arg1.compare("undelete") == 0)
		{
			cout << "user entered " << arg1 << endl;
			// fat32_undelete();
		}
		
		else
		{
			cout << "\"" << arg1 << "\" is an invalid command" << endl;
		}
	}
	
	// then after that we can implement funtions to work on the data
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

bool parse_input(const string INPUTSTRING, string& arg2)
{
	size_t pos1 = INPUTSTRING.find_first_of(" ");
	size_t pos2;
	
	if (pos1 == NPOS || pos1 + 1 == INPUTSTRING.length())
	{
		cout << "No arguments, expected 1" << endl;
		return false; 
	}
	
	++pos1;
	pos2 = INPUTSTRING.find_first_of(" ", pos1);
	
	arg2 = INPUTSTRING.substr(pos1, pos2 - pos1);
	
	return true;
}

bool parse_input(const string INPUTSTRING, string& arg2, string& arg3)
{
	size_t pos1 = INPUTSTRING.find_first_of(" ");
	size_t pos2;
	
	if (pos1 == NPOS || pos1 + 1 == INPUTSTRING.length())
	{
		cout << "No arguments, expected 2" << endl;
		return false; 
	}
	
	++pos1;
	pos2 = INPUTSTRING.find_first_of(" ", pos1);
	
	if (pos2 == NPOS || pos2 + 1 == INPUTSTRING.length())
	{
		cout << "1 argument, expected 2" << endl;
		return false;
	}
	
	arg2 = INPUTSTRING.substr(pos1, pos2 - pos1);
	
	pos1 = pos2 + 1;
	pos2 = INPUTSTRING.find_first_of(" ", pos1);
	
	arg3 = INPUTSTRING.substr(pos1, pos2 - pos1);
	
	return true;
}

bool parse_input(const string INPUTSTRING, string& arg2, string& arg3,
string& arg4)
{
	size_t pos1 = INPUTSTRING.find_first_of(" ");
	size_t pos2;
	
	if (pos1 == NPOS || pos1 + 1 == INPUTSTRING.length())
	{
		cout << "No arguments, expected 3" << endl;
		return false; 
	}
	
	++pos1;
	pos2 = INPUTSTRING.find_first_of(" ", pos1);
	
	if (pos2 == NPOS || pos2 + 1 == INPUTSTRING.length())
	{
		cout << "1 argument, expected 3" << endl;
		return false;
	}
	
	arg2 = INPUTSTRING.substr(pos1, pos2 - pos1);
	
	pos1 = pos2 + 1;
	pos2 = INPUTSTRING.find_first_of(" ", pos1);
	
	if (pos2 == NPOS || pos2 + 1 == INPUTSTRING.length())
	{
		cout << "2 arguments, expected 3" << endl;
		return false;
	}
	
	arg3 = INPUTSTRING.substr(pos1, pos2 - pos1);
	
	pos1 = pos2 + 1;
	pos2 = INPUTSTRING.find_first_of(" ", pos1);
	
	arg4 = INPUTSTRING.substr(pos1, pos2 - pos1);
	
	return true;
}

bool parse_input_quoted_data(const string INPUTSTRING, string& arg2,
string& arg3, string& arg4)
{
	size_t pos1 = INPUTSTRING.find_first_of(" ");
	size_t pos2;
	
	if (pos1 == NPOS || pos1 + 1 == INPUTSTRING.length())
	{
		cout << "No arguments, expected 3" << endl;
		return false; 
	}
	
	++pos1;
	pos2 = INPUTSTRING.find_first_of(" ", pos1);
	
	if (pos2 == NPOS || pos2 + 1 == INPUTSTRING.length())
	{
		cout << "1 argument, expected 3" << endl;
		return false;
	}
	
	arg2 = INPUTSTRING.substr(pos1, pos2 - pos1);
	
	pos1 = pos2 + 1;
	pos2 = INPUTSTRING.find_first_of(" ", pos1);
	
	if (pos2 == NPOS || pos2 + 1 == INPUTSTRING.length())
	{
		cout << "2 arguments, expected 3" << endl;
		return false;
	}
	
	arg3 = INPUTSTRING.substr(pos1, pos2 - pos1);
	
	pos1 = INPUTSTRING.find_first_of("\"", pos2);
	
	if (pos1 == NPOS || pos1 + 1 == INPUTSTRING.length())
	{
		cout << "no quoted data given" << endl;
		return false;
	}
	
	++pos1;
	pos2 = INPUTSTRING.find_first_of("\"", pos1);
	
	if (pos2 == NPOS)
	{
		cout << "no quoted data given" << endl;
		return false;
	}
	
	arg4 = INPUTSTRING.substr(pos1, pos2 - pos1);
	
	return true;
}

uint32_t find_dir(string dir_name, uint32_t cluster)
{
	uint32_t returnclus = 0;
	uint32_t currentclus = cluster;
	unsigned int FirstSectorofCluster = ((cluster - 2) * BPB_SecPerClus)
	+ FirstDataSector;
	unsigned int currentstruct;
	string currentname;
	string longbuffer;
	uint8_t DIR_Attr;
	unsigned int offset = 2;
	bool eoc = false;
	
	if (cluster == BPB_RootClus)
	{
		offset = 0;
	}
	
	while (!eoc)
	{
		currentstruct = FirstSectorofCluster * BPB_BytsPerSec
		+ offset * 32;
		
		//
		// actually check the attributes of the entry structure here
		//
		
		DIR_Attr = *(fdata + currentstruct + 11);
		
		// if directory/file has long name, act accordingly
		if ((DIR_Attr & 0xF) == 0xF)
		{
			// if entry structure references a directory, get name
			if ((DIR_Attr & 0x10) == 0x10)
			{
				currentname.clear();
				longbuffer.clear();
				
				do
				{
					longbuffer.push_back
					(*(fdata + currentstruct + 30));
					longbuffer.push_back
					(*(fdata + currentstruct + 28));
					longbuffer.push_back
					(*(fdata + currentstruct + 24));
					longbuffer.push_back
					(*(fdata + currentstruct + 22));
					longbuffer.push_back
					(*(fdata + currentstruct + 20));
					longbuffer.push_back
					(*(fdata + currentstruct + 18));
					longbuffer.push_back
					(*(fdata + currentstruct + 16));
					longbuffer.push_back
					(*(fdata + currentstruct + 14));
					longbuffer.push_back
					(*(fdata + currentstruct + 9));
					longbuffer.push_back
					(*(fdata + currentstruct + 7));
					longbuffer.push_back
					(*(fdata + currentstruct + 5));
					longbuffer.push_back
					(*(fdata + currentstruct + 3));
					longbuffer.push_back
					(*(fdata + currentstruct + 1));
					
					// if not at end of cluster
					if ((offset + 1) * 32 < BPB_BytsPerSec
					* BPB_SecPerClus)
					{
						++offset;
					}
					
					// else end of cluster has been reached
					else
					{
						currentclus = parseInteger
						<uint32_t>(fdata
						+ BPB_RsvdSecCnt
						* BPB_BytsPerSec
						+ currentclus * 4);
						currentclus &= 0x0FFFFFFF;
						FirstSectorofCluster =
						((currentclus - 2)
						* BPB_SecPerClus)
						+ FirstDataSector;
						offset = 0;
					}
					
					currentstruct = FirstSectorofCluster
					* BPB_BytsPerSec + offset * 32;
					DIR_Attr = *(fdata + currentstruct + 11);
				} while ((DIR_Attr & 0xF) == 0xF);
				
				for (unsigned int i = longbuffer.size(); i > 0
				&& longbuffer[i - 1] + 1 > 1; --i)
				{
					currentname.push_back
					(longbuffer[i - 1]);
				}
				
				returnclus = parseInteger<uint16_t>
				(fdata + currentstruct + 20);
				returnclus = returnclus << 16;
				returnclus += parseInteger<uint16_t>
				(fdata + currentstruct + 26);
				
				// if dir_name has been found, return the
				// cluster number
				if (!currentname.compare(dir_name))
				{
					return returnclus;
				}
				
				// else search the directory for dir_name
				else
				{
					returnclus = find_dir(dir_name,
					returnclus);
					if (returnclus != 0)
					{
						return returnclus;
					}
				}
			}
			
			// else structure references a file, so skip ahead
			else
			{
				do
				{
					// if not at end of cluster
					if ((offset + 1) * 32 < BPB_BytsPerSec
					* BPB_SecPerClus)
					{
						++offset;
					}
					
					// else end of cluster has been reached
					else
					{
						currentclus = parseInteger
						<uint32_t>(fdata
						+ BPB_RsvdSecCnt
						* BPB_BytsPerSec
						+ currentclus * 4);
						currentclus &= 0x0FFFFFFF;
						FirstSectorofCluster =
						((currentclus - 2)
						* BPB_SecPerClus)
						+ FirstDataSector;
						offset = 0;
					}
					
					currentstruct = FirstSectorofCluster
					* BPB_BytsPerSec + offset * 32;
					DIR_Attr = *(fdata + currentstruct + 11);
				} while ((DIR_Attr & 0xF) == 0xF);
			}
		}
		
		// else directory/file has short name
		else
		{
			// if structure references a directory, check name
			if ((DIR_Attr & 0x10) == 0x10)
			{
				currentname.clear();
				
				for (unsigned int i = 0; i < 8; ++i)
				{
					currentname.push_back(*(fdata
					+ currentstruct + i));
				}
				
				while (currentname[currentname.size() - 1]
				== 0x20)
				{
					currentname.erase(currentname.end()
					- 1);
				}
				
				if (*(fdata + currentstruct + 8) != 0x20)
				{
					currentname.push_back(0x2E);
					
					for (unsigned int i = 8; i < 11; ++i)
					{
						currentname.push_back(*(fdata
						+ currentstruct + i));
					}
					
					while (currentname[currentname.size()
					- 1] == 0x20)
					{
						currentname.erase
						(currentname.end() - 1);
					}
				}
				
				returnclus = parseInteger<uint16_t>
				(fdata + currentstruct + 20);
				returnclus = returnclus << 16;
				returnclus += parseInteger<uint16_t>
				(fdata + currentstruct + 26);
				
				// if dir_name has been found, return the
				// cluster number
				if (!currentname.compare(dir_name))
				{
					return returnclus;
				}
				
				// else search the directory for dir_name
				else
				{
					returnclus = find_dir(dir_name,
					returnclus);
					if (returnclus != 0)
					{
						return returnclus;
					}
				}
			}
			
			// else structure references a file, so ignore
		}
		
		// if not at end of cluster
		if ((offset + 1) * 32 < BPB_BytsPerSec * BPB_SecPerClus)
		{
			++offset;
		}
		
		// else end of cluster has been reached
		else
		{
			currentclus = parseInteger<uint32_t>(fdata
			+ BPB_RsvdSecCnt * BPB_BytsPerSec + currentclus * 4);
			
			// if end of cluster chain is found, break from loop
			if (currentclus >= 0xFFFFFFF8)
			{
				eoc = true;
			}
			
			// else follow cluster chain to next cluster
			else
			{
				currentclus &= 0x0FFFFFFF;
				FirstSectorofCluster = ((currentclus - 2)
				* BPB_SecPerClus) + FirstDataSector;
				offset = 0;
			}
		}
	}
	
	return 0;
}

void fat32_fsinfo()
{
	//print summary of file system values

	cout << "Bytes per sector: " << BPB_BytsPerSec << endl;
	cout << "Sectors per cluster: " << int(BPB_SecPerClus) << endl;
	cout << "Total sectors: " << BPB_TotSec32 << endl;
	cout << "Number of FATs: " <<  int(BPB_NumFATs) << endl;
	cout << "Sectors per FAT: " <<  BPB_FATSz32 << endl;
	cout << "Number of free sectors: " << FSI_Free_Count << endl;
}

void fat32_close(string file_name)
{
	for (unsigned int i = 0; i < filenamevector.size(); ++i)
	{
		if (file_name.compare(filenamevector[i]) == 0)
		{
			filenamevector.erase(filenamevector.begin() + i);
			modevector.erase(modevector.begin() + i);
			dirvector.erase(dirvector.begin() + i);
			cout << file_name << " is now closed" << endl;
			return;
		}
	}
	
	cout << "could not close " << file_name << endl;
}

void fat32_cd(string dir_name)
{
	if (!dir_name.compare("\\"))
	{
		curdir = BPB_RootClus;
		return;
	}
	
	
}

void fat32_ls(string dir_name)
{
	uint32_t lsclus = 0;
	
	if (!dir_name.compare("\\"))
	{
		lsclus = BPB_RootClus;
	}
	
	else
	{
		lsclus = find_dir(dir_name, BPB_RootClus);
		
		if (lsclus == 0)
		{
			return;
		}
	}
	
	cout << lsclus << endl;
}
