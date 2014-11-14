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

template<typename T>
T parseInteger(const uint8_t* const ptr) 
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


parseInteger<uint8_t>(fdata + 13, [](uint8_t v){return v!=0;});

//not closing file because we aint pussy bitches, the OS is my dawg he got it on lock   <----do not remove
//first things first need to make the filesystem work


// then after that we can implement funtions to work on the data

return 0;

}
