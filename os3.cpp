#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/mman.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h> 
#include <fcntl.h>
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
vector<unsigned int> structvector;
vector<uint32_t> fstclusvector;
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
bool is_valid_short_name(string);
bool is_valid_long_name(string);
string get_upper(string);
string get_condensed_short_name(string);
string get_condensed_long_name(string);
unsigned int find_file_in_curdir(string);
uint32_t find_dir(string, uint32_t);
void print_current_path();
void fat32_fsinfo();
void fat32_open(string, string);
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

int main(int argc, char *argv[])
{
	struct stat fdatastat;
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
	
	if (fstat(fd, &fdatastat) < 0)
	{
		cerr << "error obtaining file statistics" << endl;
		exit(EXIT_FAILURE);
	}
	
	unsigned int len = fdatastat.st_size;
	fdata = (uint8_t*)mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
	
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
		cout << "[" << argv[1];
		print_current_path();
		cout << "]> ";
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
				fat32_open(arg2, arg3);
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
			if (parse_input(inputstring, arg2))
			{
				fat32_cd(arg2);
			}
		}
		
		else if (arg1.compare("ls") == 0)
		{
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

//
// *** parse_input(2) ***
//
// Parses user input stored in "INPUTSTRING" and places the second argument
// into "arg2." Returns true if the argument is parsed successfully. Otherwise,
// prints an error message and returns false.
//
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

//
// *** parse_input(3) ***
//
// Parses user input stored in "INPUTSTRING" and places the second and third
// arguments into "arg2" and "arg3," respectively. Returns true if both
// arguments are parsed successfully. Otherwise, prints an error message and
// returns false.
//
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

//
// *** parse_input(4) ***
//
// Parses user input stored in "INPUTSTRING" and places the second, third, and
// fourth arguments into "arg2," "arg3," and "arg4," respectively. Returns true
// if all three arguments are parsed successfully. Otherwise, prints an error
// message and returns false.
//
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

//
// *** parse_input_quoted_data ***
//
// Parses user input stored in "INPUTSTRING" and places the second, third, and
// fourth arguments into "arg2," "arg3," and "arg4," respectively. The fourth
// argument must be a string surrounded by quotes. Returns true if all three
// arguments are parsed successfully. Otherwise, prints an error message and
// returns false.
//
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

//
// *** is_valid_short_name ***
//
// Determines whether "filename" could be turned into a valid FAT32 short name
// if given as user input.
//
bool is_valid_short_name(string filename)
{
	unsigned int period = filename.find_first_of('.');
	
	// name cannot be zero-length, signify an empty structure, start with
	// a space, or start with a period
	if (filename.empty() || filename[0] == 0 || filename[0] == 0xE5
	|| filename[0] == 0x20 || period == 0)
	{
		return false;
	}
	
	// if name does not contain a period
	if (period == NPOS)
	{
		// without file extension, name can't be more than 8 characters
		if (filename.size() > 8)
		{
			return false;
		}
		
		// check each character in filename
		for (unsigned int i = 0; i < filename.size(); ++i)
		{
			// if a character value is below 0x20, it can only be
			// 0x05 in the first position of the filename
			if (filename[i] < 0x20)
			{
				if (filename[i] == 0x05)
				{
					if (i != 0)
					{
						return false;
					}
				}
				
				else
				{
					return false;
				}
			}
			
			// check for other invalid characters
			else if (filename[i] == 0x22 || filename[i] == 0x2A
			|| filename[i] == 0x2B || filename[i] == 0x2C
			|| filename[i] == 0x2F || filename[i] == 0x3A
			|| filename[i] == 0x3B || filename[i] == 0x3C
			|| filename[i] == 0x3D || filename[i] == 0x3E
			|| filename[i] == 0x3F || filename[i] == 0x5B
			|| filename[i] == 0x5C || filename[i] == 0x5D
			|| filename[i] == 0x7C)
			{
				return false;
			}
		}
	}
	
	// else if name does contain a period
	else
	{
		// main part of filename can't be longer than 8 characters and
		// extension can't be longer than 3 characters
		if (period > 8 || filename.size() > period + 4)
		{
			return false;
		}
		
		// check main part of filename
		for (unsigned int i = 0; i < period; ++i)
		{
			// if a character value is below 0x20, it can only be
			// 0x05 in the first position of the filename
			if (filename[i] < 0x20)
			{
				if (filename[i] == 0x05)
				{
					if (i != 0)
					{
						return false;
					}
				}
				
				else
				{
					return false;
				}
			}
			
			// check for other invalid characters
			else if (filename[i] == 0x22 || filename[i] == 0x2A
			|| filename[i] == 0x2B || filename[i] == 0x2C
			|| filename[i] == 0x2F || filename[i] == 0x3A
			|| filename[i] == 0x3B || filename[i] == 0x3C
			|| filename[i] == 0x3D || filename[i] == 0x3E
			|| filename[i] == 0x3F || filename[i] == 0x5B
			|| filename[i] == 0x5C || filename[i] == 0x5D
			|| filename[i] == 0x7C)
			{
				return false;
			}
		}
		
		// check filename extension
		for (unsigned int i = period + 1; i < filename.size(); ++i)
		{
			if (filename[i] == 0x22 || filename[i] == 0x2A
			|| filename[i] == 0x2B || filename[i] == 0x2C
			|| filename[i] == 0x2E || filename[i] == 0x2F
			|| filename[i] == 0x3A || filename[i] == 0x3B
			|| filename[i] == 0x3C || filename[i] == 0x3D
			|| filename[i] == 0x3E || filename[i] == 0x3F
			|| filename[i] == 0x5B || filename[i] == 0x5C
			|| filename[i] == 0x5D || filename[i] == 0x7C
			|| filename[i] < 0x20)
			{
				return false;
			}
		}
	}
	
	return true;
}

//
// *** is_valid_long_name ***
//
// Determines whether "filename" is a valid FAT32 long name.
//
bool is_valid_long_name(string filename)
{
	// name cannot be zero-length, be longer than 255 characters, or
	// signify an empty structure
	if (filename.empty() || filename[0] == 0 || filename[0] == 0xE5)
	{
		return false;
	}
	
	// check each character in filename
	for (unsigned int i = 0; i < filename.size(); ++i)
	{
		// if a character value is below 0x20, it can only be
		// 0x05 in the first position of the filename
		if (filename[i] < 0x20)
		{
			if (filename[i] == 0x05)
			{
				if (i != 0)
				{
					return false;
				}
			}
			
			else
			{
				return false;
			}
		}
		
		// check for other invalid characters
		else if (filename[i] == 0x22 || filename[i] == 0x2A
		|| filename[i] == 0x2F || filename[i] == 0x3A
		|| filename[i] == 0x3C || filename[i] == 0x3E
		|| filename[i] == 0x3F || filename[i] == 0x5C
		|| filename[i] == 0x7C)
		{
			return false;
		}
	}
	
	// filename cannot consist only of characters that would be ignored
	if (get_condensed_long_name(filename).size() == 0)
	{
		return false;
	}
	
	return true;
}

//
// *** get_upper ***
//
// Returns a copy of "s" that has had all lower case letters changed into
// upper case letters.
//
string get_upper(string s)
{
	for (unsigned int i = 0; i < s.size(); ++i)
	{
		if (s[i] >= 0x61 && s[i] <= 0x7A)
		{
			s[i] -= 0x20;
		}
	}
	
	return s;
}

//
// *** get_condensed_short_name ***
//
// Takes in a string "s" that can be turned into a valid FAT32 short name and
// returns a copy of "s" that has no trailing period.
//
string get_condensed_short_name(string s)
{
	// remove trailing period if one exists
	if (s.back() == 0x2E)
	{
		s.pop_back();
	}
	
	return s;
}

//
// *** get_condensed_long_name ***
//
// Takes in a string "s" that is a valid FAT32 long name and returns a copy of
// "s" that has no leading and trailing spaces and no trailing periods.
//
string get_condensed_long_name(string s)
{
	// remove leading spaces
	while (s[0] == 0x20)
	{
		s.erase(s.begin());
	}
	
	// remove trailing spaces and periods
	while (s.back() == 0x20 || s.back() == 0x2E)
	{
		s.pop_back();
	}
	
	return s;
}

//
// *** find_file_in_curdir ***
//
// Searches the current directory for a file with a name that
// matches "file_name" and returns the location of the file's entry structure if
// it is found. "file_name" is expected to contain no lower case letters and no
// trailing periods. If looking for a long name, "file_name" is also expected to
// contain no leading or trailing spaces. Returns 0 if file cannot be found.
//
unsigned int find_file_in_curdir(string file_name)
{
	uint32_t currentclus = curdir;
	uint32_t entryclus;
	unsigned int FirstSectorofCluster = ((curdir - 2) * BPB_SecPerClus)
	+ FirstDataSector;
	unsigned int currentstruct;
	string currentname;
	string longbuffer;
	uint8_t DIR_Attr;
	unsigned int offset = 2;
	
	if (currentclus == BPB_RootClus)
	{
		offset = 0;
	}
	
	while (1)
	{
		currentstruct = FirstSectorofCluster * BPB_BytsPerSec
		+ offset * 32;
		
		// if end of cluster found, return from function
		if (parseInteger<uint8_t>(fdata + currentstruct) == 0)
		{
			return 0;
		}
		
		DIR_Attr = parseInteger<uint8_t>(fdata + currentstruct + 11);
		
		// if directory entry is free, ignore it
		if (parseInteger<uint8_t>(fdata + currentstruct) == 0xE5)
		{
			
		}
		
		// else if directory/file has long name, act accordingly
		else if ((DIR_Attr & 0xF) == 0xF)
		{
			currentname.clear();
			longbuffer.clear();
			
			// read long name into longbuffer in reverse
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
			
			// if long name belongs to file, check name
			if ((DIR_Attr & 0x10) == 0)
			{
				// reverse string to get correct order
				for (unsigned int i = longbuffer.size(); i > 0
				&& longbuffer[i - 1] + 1 > 1; --i)
				{
					currentname.push_back
					(longbuffer[i - 1]);
				}
				
				// get cluster number stored in entry structure
				entryclus = parseInteger<uint16_t>
				(fdata + currentstruct + 20);
				entryclus = entryclus << 16;
				entryclus += parseInteger<uint16_t>
				(fdata + currentstruct + 26);
				entryclus &= 0x0FFFFFFF;
				
				// if cluster number is invalid, ignore entry
				if (entryclus < 2 || entryclus >= 0x0FFFFFF7)
				{
					
				}
				
				// else if file_name has been found, return the
				// entry structure location
				else if (!file_name.compare(get_upper
				(get_condensed_long_name(currentname))))
				{
					return currentstruct;
				}
			}
			
			// else structure references a directory, so ignore
		}
		
		// else directory/file has short name
		else
		{
			// if structure references a file, check name
			if ((DIR_Attr & 0x10) == 0)
			{
				currentname.clear();
				
				for (unsigned int i = 0; i < 8; ++i)
				{
					currentname.push_back
					(*(fdata + currentstruct + i));
				}
				
				while (currentname.back() == 0x20)
				{
					currentname.pop_back();
				}
				
				currentname.push_back(0x2E);
				
				for (unsigned int i = 8; i < 11; ++i)
				{
					currentname.push_back
					(*(fdata + currentstruct + i));
				}
				
				while (currentname.back() == 0x20)
				{
					currentname.pop_back();
				}
				
				entryclus = parseInteger<uint16_t>
				(fdata + currentstruct + 20);
				entryclus = entryclus << 16;
				entryclus += parseInteger<uint16_t>
				(fdata + currentstruct + 26);
				entryclus &= 0x0FFFFFFF;
				
				// if cluster number is invalid, ignore entry
				if (entryclus < 2 || entryclus >= 0x0FFFFFF7)
				{
					
				}
				
				// else if file_name has been found, return the
				// entry structure location
				else if (!file_name.compare
				(get_condensed_short_name(currentname)))
				{
					return currentstruct;
				}
			}
			
			// else structure references a directory, so ignore
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
			currentclus &= 0x0FFFFFFF;
			
			// if end of cluster chain is found, return 0
			if (currentclus >= 0x0FFFFFF8)
			{
				return 0;
			}
			
			// else follow cluster chain to next cluster
			else
			{
				FirstSectorofCluster = ((currentclus - 2)
				* BPB_SecPerClus) + FirstDataSector;
				offset = 0;
			}
		}
	}
}

//
// *** find_dir ***
//
// Recursively searches a directory, whose first cluster number is stored in
// "cluster," and all of its subdirectories for a directory with a name that
// matches "dir_name" and returns the first cluster number of that directory if
// it is found. "dir_name" is expected to contain no lower case letters and no
// trailing periods. If looking for a long name, "dir_name" is also expected to
// contain no leading or trailing spaces. Returns 0 if directory cannot be
// found.
//
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
	
	if (cluster == BPB_RootClus)
	{
		offset = 0;
	}
	
	while (1)
	{
		currentstruct = FirstSectorofCluster * BPB_BytsPerSec
		+ offset * 32;
		
		// if end of cluster found, return from function
		if (parseInteger<uint8_t>(fdata + currentstruct) == 0)
		{
			return 0;
		}
		
		DIR_Attr = parseInteger<uint8_t>(fdata + currentstruct + 11);
		
		// if directory entry is free, ignore it
		if (parseInteger<uint8_t>(fdata + currentstruct) == 0xE5)
		{
			
		}
		
		// else if directory/file has long name, act accordingly
		else if ((DIR_Attr & 0xF) == 0xF)
		{
			currentname.clear();
			longbuffer.clear();
			
			// read long name into longbuffer in reverse
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
			
			// if long name belongs to directory, check name
			if ((DIR_Attr & 0x10) == 0x10)
			{
				// reverse string to get correct order
				for (unsigned int i = longbuffer.size(); i > 0
				&& longbuffer[i - 1] + 1 > 1; --i)
				{
					currentname.push_back
					(longbuffer[i - 1]);
				}
				
				// get cluster number stored in entry structure
				returnclus = parseInteger<uint16_t>
				(fdata + currentstruct + 20);
				returnclus = returnclus << 16;
				returnclus += parseInteger<uint16_t>
				(fdata + currentstruct + 26);
				returnclus &= 0x0FFFFFFF;
				
				// if cluster number is invalid, ignore entry
				if (returnclus < 2 || returnclus >= 0x0FFFFFF7)
				{
					
				}
				
				// else if dir_name has been found, return the
				// cluster number
				else if (!dir_name.compare(get_upper
				(get_condensed_long_name(currentname))))
				{
					return returnclus;
				}
				
				// else search the directory for dir_name
				else
				{
					returnclus = find_dir
					(dir_name, returnclus);
					if (returnclus != 0)
					{
						return returnclus;
					}
				}
			}
			
			// else structure references a file, so ignore
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
					currentname.push_back
					(*(fdata + currentstruct + i));
				}
				
				while (currentname.back() == 0x20)
				{
					currentname.pop_back();
				}
				
				currentname.push_back(0x2E);
				
				for (unsigned int i = 8; i < 11; ++i)
				{
					currentname.push_back
					(*(fdata + currentstruct + i));
				}
				
				while (currentname.back() == 0x20)
				{
					currentname.pop_back();
				}
				
				returnclus = parseInteger<uint16_t>
				(fdata + currentstruct + 20);
				returnclus = returnclus << 16;
				returnclus += parseInteger<uint16_t>
				(fdata + currentstruct + 26);
				returnclus &= 0x0FFFFFFF;
				
				// if cluster number is invalid, ignore entry
				if (returnclus < 2 || returnclus >= 0x0FFFFFF7)
				{
					
				}
				
				// else if dir_name has been found, return the
				// cluster number
				else if (!dir_name.compare
				(get_condensed_short_name(currentname)))
				{
					return returnclus;
				}
				
				// else search the directory for dir_name
				else
				{
					returnclus = find_dir
					(dir_name, returnclus);
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
			currentclus &= 0x0FFFFFFF;
			
			// if end of cluster chain is found, return 0
			if (currentclus >= 0x0FFFFFF8)
			{
				return 0;
			}
			
			// else follow cluster chain to next cluster
			else
			{
				FirstSectorofCluster = ((currentclus - 2)
				* BPB_SecPerClus) + FirstDataSector;
				offset = 0;
			}
		}
	}
}

//
// *** print_current_path ***
//
// Prints the path of the current directory. This function assumes that the
// cluster numbers stored in the entry structures that it searches through are
// all valid cluster numbers.
//
void print_current_path()
{
	vector<string> pathvector;
	string currentname;
	string longbuffer;
	uint32_t currentclus = curdir;
	uint32_t nextclus;
	uint32_t searchclus;
	uint32_t compareclus;
	unsigned int FirstSectorofCluster = ((currentclus - 2)
	* BPB_SecPerClus) + FirstDataSector;
	unsigned int currentstruct;
	uint8_t DIR_Attr;
	unsigned int offset;
	bool founddir;
	
	// insert each directory in the path into pathvector
	while (currentclus != BPB_RootClus)
	{
		FirstSectorofCluster = ((currentclus - 2) * BPB_SecPerClus)
		+ FirstDataSector;
		
		nextclus = parseInteger<uint16_t>(fdata + FirstSectorofCluster
		* BPB_BytsPerSec + 32 + 20);
		nextclus = nextclus << 16;
		nextclus += parseInteger<uint16_t>(fdata + FirstSectorofCluster
		* BPB_BytsPerSec + 32 + 26);
		nextclus &= 0x0FFFFFFF;
		
		// dotdot entry gives cluster number 0 when pointing to root
		// directory, so adjust nextclus when this is encountered
		if (nextclus == 0)
		{
			nextclus = BPB_RootClus;
			offset = 0;
		}
		
		else
		{
			offset = 2;
		}
		
		searchclus = nextclus;
		FirstSectorofCluster = ((nextclus - 2) * BPB_SecPerClus)
		+ FirstDataSector;
		founddir = false;
		
		// find the entry structure for the current directory in its
		// parent directory, then use that structure to get the current
		// directory's name
		while (!founddir)
		{
			currentstruct = FirstSectorofCluster * BPB_BytsPerSec
			+ offset * 32;
			
			// if end of cluster found, print error message
			if (parseInteger<uint8_t>(fdata + currentstruct) == 0)
			{
				cout << "(error printing path)(0)";
				return;
			}
			
			DIR_Attr = parseInteger<uint8_t>(fdata + currentstruct
			+ 11);
			
			// if directory entry is free, ignore it
			if (parseInteger<uint8_t>(fdata + currentstruct)
			== 0xE5)
			{
				
			}
			
			// else if directory/file has long name, act accordingly
			else if ((DIR_Attr & 0xF) == 0xF)
			{
				currentname.clear();
				longbuffer.clear();
				
				// read long name into longbuffer in reverse
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
						searchclus = parseInteger
						<uint32_t>(fdata
						+ BPB_RsvdSecCnt
						* BPB_BytsPerSec
						+ searchclus * 4);
						searchclus &= 0x0FFFFFFF;
						FirstSectorofCluster =
						((searchclus - 2)
						* BPB_SecPerClus)
						+ FirstDataSector;
						offset = 0;
					}
					
					currentstruct = FirstSectorofCluster
					* BPB_BytsPerSec + offset * 32;
					DIR_Attr = *(fdata + currentstruct
					+ 11);
				} while ((DIR_Attr & 0xF) == 0xF);
				
				// if long name belongs to directory, check
				// first cluster
				if ((DIR_Attr & 0x10) == 0x10)
				{
					// get cluster number stored in entry
					// structure
					compareclus = parseInteger<uint16_t>
					(fdata + currentstruct + 20);
					compareclus = compareclus << 16;
					compareclus += parseInteger<uint16_t>
					(fdata + currentstruct + 26);
					compareclus &= 0x0FFFFFFF;
					
					// if cluster numbers match, store name
					// and move to parent directory
					if (compareclus == currentclus)
					{
						founddir = true;
						currentclus = nextclus;
						
						// reverse string to get
						// correct order
						for (unsigned int i
						= longbuffer.size(); i > 0
						&& longbuffer[i - 1] + 1 > 1;
						--i)
						{
							currentname.push_back
							(longbuffer[i - 1]);
						}
						
						pathvector.push_back
						(get_condensed_long_name
						(currentname));
					}
				}
				
				// else structure references a file, so ignore
			}
			
			// else directory/file has short name
			else
			{
				// if short name belongs to directory, check
				// first cluster
				if ((DIR_Attr & 0x10) == 0x10)
				{
					// get cluster number stored in entry
					// structure
					compareclus = parseInteger<uint16_t>
					(fdata + currentstruct + 20);
					compareclus = compareclus << 16;
					compareclus += parseInteger<uint16_t>
					(fdata + currentstruct + 26);
					compareclus &= 0x0FFFFFFF;
					
					// if cluster numbers match, store name
					// and move to parent directory
					if (compareclus == currentclus)
					{
						founddir = true;
						currentclus = nextclus;
						
						currentname.clear();
						
						for (unsigned int i = 0;
						i < 8; ++i)
						{
							currentname.push_back
							(*(fdata
							+ currentstruct + i));
						}
						
						while (currentname.back()
						== 0x20)
						{
							currentname.pop_back();
						}
						
						currentname.push_back(0x2E);
						
						for (unsigned int i = 8;
						i < 11; ++i)
						{
							currentname.push_back
							(*(fdata
							+ currentstruct + i));
						}
						
						while (currentname.back()
						== 0x20)
						{
							currentname.pop_back();
						}
						
						pathvector.push_back
						(get_condensed_short_name
						(currentname));
					}
				}
				
				// else structure references a file, so ignore
			}
			
			// if matching cluster number has been found, don't
			// advance to next entry structure
			if (founddir)
			{
				
			}
			
			// else if not at end of cluster
			else if ((offset + 1) * 32 < BPB_BytsPerSec
			* BPB_SecPerClus)
			{
				++offset;
			}
			
			// else end of cluster has been reached
			else
			{
				searchclus = parseInteger<uint32_t>(fdata
				+ BPB_RsvdSecCnt * BPB_BytsPerSec
				+ searchclus * 4);
				searchclus &= 0x0FFFFFFF;
				
				// if end of cluster chain is found, print
				// error message
				if (searchclus >= 0x0FFFFFF8)
				{
					cout << "(error printing path)(eoc)";
					return;
				}
				
				// else follow cluster chain to next cluster
				else
				{
					FirstSectorofCluster
					= ((searchclus - 2) * BPB_SecPerClus)
					+ FirstDataSector;
					offset = 0;
				}
			}
		}
	}
	
	// print out the path one directory at a time
	for (unsigned int i = 0; i < pathvector.size(); ++i)
	{
		cout << "/" << pathvector[pathvector.size() - i - 1];
	}
}

//
// *** fat32_fsinfo ***
//
// Prints out a brief summary of the volume's file system values. Includes
// bytes per sector, sectors per cluster, total sectors, number of FATs,
// sectors per FAT, and number of free sectors.
//
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

//
// *** fat32_open ***
//
// Opens a file with the name "file_name" by adding it to the open file table
// with the permissions denoted by "mode." The file must be in the current
// directory. Fails if "file_name" is an invalid name or cannot be found. Also
// fails if "mode" is not a valid mode. Does not require "file_name" to have
// been validated prior to the call to this function.
//
void fat32_open(string file_name, string mode)
{
	string comparename;
	unsigned int filestruct;
	uint32_t fstclus;
	
	// check if file_name is a valid name
	if (is_valid_short_name(file_name))
	{
		comparename = get_upper(get_condensed_short_name(file_name));
	}
	
	else if (is_valid_long_name(file_name))
	{
		comparename = get_upper(get_condensed_long_name(file_name));
	}
	
	else
	{
		cout << file_name << " is not a valid name" << endl;
		return;
	}
	
	// check if mode is a valid mode
	if (mode.compare(READ) && mode.compare(WRITE)
	&& mode.compare(READWRITE))
	{
		cout << mode << " is not a valid mode" << endl;
		return;
	}
	
	// if file is already open, print error message and return
	for (unsigned int i = 0; i < filenamevector.size(); ++i)
	{
		if (comparename.compare(filenamevector[i]) == 0)
		{
			cout << file_name << " is already open" << endl;
			return;
		}
	}
	
	filestruct = find_file_in_curdir(comparename);
	
	// if file could not be found, print error message and return
	if (filestruct == 0)
	{
		cout << file_name << " could not be found" << endl;
		return;
	}
	
	fstclus = parseInteger<uint16_t>(fdata + filestruct + 20);
	fstclus = fstclus << 16;
	fstclus += parseInteger<uint16_t>(fdata + filestruct + 26);
	fstclus &= 0x0FFFFFFF;
	
	// create entry in open file table
	filenamevector.push_back(comparename);
	modevector.push_back(mode);
	dirvector.push_back(curdir);
	structvector.push_back(filestruct);
	fstclusvector.push_back(fstclus);
	
	// output appropriate message upon successful open
	cout << file_name << " has been opened with ";
	if (!mode.compare(READ))
	{
		cout << "read-only";
	}
	else if (!mode.compare(WRITE))
	{
		cout << "write-only";
	}
	else
	{
		cout << "read-write";
	}
	cout << " permission" << endl;
}

//
// *** fat32_close ***
//
// Closes a file by removing it from the open file table. Fails if "file_name"
// is an invalid name or is not in the open file table. Does not require
// "file_name" to have been validated prior to the call to this function.
//
void fat32_close(string file_name)
{
	string comparename;
	
	// check if file_name is a valid name
	if (is_valid_short_name(file_name))
	{
		comparename = get_upper(get_condensed_short_name(file_name));
	}
	
	else if (is_valid_long_name(file_name))
	{
		comparename = get_upper(get_condensed_long_name(file_name));
	}
	
	else
	{
		cout << file_name << " is not a valid name" << endl;
		return;
	}
	
	// if file is currently open, close it and return
	for (unsigned int i = 0; i < filenamevector.size(); ++i)
	{
		if (comparename.compare(filenamevector[i]) == 0)
		{
			filenamevector.erase(filenamevector.begin() + i);
			modevector.erase(modevector.begin() + i);
			dirvector.erase(dirvector.begin() + i);
			structvector.erase(structvector.begin() + i);
			fstclusvector.erase(fstclusvector.begin() + i);
			cout << file_name << " is now closed" << endl;
			return;
		}
	}
	
	// else file could not be found
	cout << file_name << " cannot be found in the open file table" << endl;
}

//
// *** fat32_cd ***
//
// Changes the present working directory to the directory named "dir_name."
// Fails if "dir_name" is an invalid name or "dir_name" cannot be found. Does
// not require "dir_name" to have been validated prior to the call to this
// function.
//
void fat32_cd(string dir_name)
{
	uint32_t newclus = 0;
	
	// handle root directory as special case
	if (!dir_name.compare("\\"))
	{
		newclus = BPB_RootClus;
	}
	
	// else if short name
	else if (is_valid_short_name(dir_name))
	{
		newclus = find_dir(get_upper(get_condensed_short_name
		(dir_name)), BPB_RootClus);
	}
	
	// else if long name
	else if (is_valid_long_name(dir_name))
	{
		newclus = find_dir(get_upper(get_condensed_long_name
		(dir_name)), BPB_RootClus);
	}
	
	if (newclus < 2 || newclus >= 0x0FFFFFF7)
	{
		cout << dir_name << " could not be found" << endl;
	}
	
	else
	{
		curdir = newclus;
	}
}

//
// *** fat32_ls ***
//
// Prints out all entries in the directory "dir_name." Does not require
// "dir_name" to have been validated prior to the call to this function.
//
void fat32_ls(string dir_name)
{
	uint32_t lsclus = 0;
	uint32_t currentclus;
	unsigned int FirstSectorofCluster;
	unsigned int currentstruct;
	string currentname;
	string longbuffer;
	uint8_t DIR_Attr;
	unsigned int offset = 2;
	bool eoc = false;
	
	// handle root directory as special case
	if (!dir_name.compare("\\"))
	{
		lsclus = BPB_RootClus;
	}
	
	// else if short name
	else if (is_valid_short_name(dir_name))
	{
		lsclus = find_dir(get_upper(get_condensed_short_name
		(dir_name)), BPB_RootClus);
		
		// if directory wasn't found, no need to output anything
		if (lsclus == 0)
		{
			return;
		}
	}
	
	// else if long name
	else if (is_valid_long_name(dir_name))
	{
		lsclus = find_dir(get_upper(get_condensed_long_name(dir_name)),
		BPB_RootClus);
		
		// if directory wasn't found, no need to output anything
		if (lsclus == 0)
		{
			return;
		}
	}
	
	// else dir_name is invalid, so no need to output anything
	else
	{
		return;
	}
	
	// output all entries in the directory
	FirstSectorofCluster = ((lsclus - 2) * BPB_SecPerClus)
	+ FirstDataSector;
	currentclus = lsclus;
	
	if (lsclus == BPB_RootClus)
	{
		offset = 0;
	}
	
	while (!eoc)
	{
		currentstruct = FirstSectorofCluster * BPB_BytsPerSec
		+ offset * 32;
		
		// if end of cluster found, return from function
		if (parseInteger<uint8_t>(fdata + currentstruct) == 0)
		{
			return;
		}
		
		DIR_Attr = parseInteger<uint8_t>(fdata + currentstruct + 11);
		
		// if directory entry is free, ignore it
		if (parseInteger<uint8_t>(fdata + currentstruct) == 0xE5)
		{
			
		}
		
		// else if directory/file has long name, act accordingly
		else if ((DIR_Attr & 0xF) == 0xF)
		{
			currentname.clear();
			longbuffer.clear();
			
			// read long name into longbuffer in reverse
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
			
			// reverse string to get correct order
			for (unsigned int i = longbuffer.size(); i > 0
			&& longbuffer[i - 1] + 1 > 1; --i)
			{
				currentname.push_back
				(longbuffer[i - 1]);
			}
		}
		
		// else directory/file has short name
		else
		{
			currentname.clear();
			
			for (unsigned int i = 0; i < 8; ++i)
			{
				currentname.push_back
				(*(fdata + currentstruct + i));
			}
			
			while (currentname[currentname.size() - 1] == 0x20)
			{
				currentname.erase(currentname.end() - 1);
			}
			
			if (*(fdata + currentstruct + 8) != 0x20)
			{
				currentname.push_back(0x2E);
				
				for (unsigned int i = 8; i < 11; ++i)
				{
					currentname.push_back
					(*(fdata + currentstruct + i));
				}
				
				while (currentname[currentname.size()
				- 1] == 0x20)
				{
					currentname.erase
					(currentname.end() - 1);
				}
			}
		}
		
		// print out entry name
		cout << currentname << endl;
		
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
			currentclus &= 0x0FFFFFFF;
			
			// if end of cluster chain is found, break from loop
			if (currentclus >= 0x0FFFFFF8)
			{
				eoc = true;
			}
			
			// else follow cluster chain to next cluster
			else
			{
				FirstSectorofCluster = ((currentclus - 2)
				* BPB_SecPerClus) + FirstDataSector;
				offset = 0;
			}
		}
	}
}
