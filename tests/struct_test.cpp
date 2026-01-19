#include<iostream>
#include <cstdint>
#include<iomanip>
using namespace std;

#define DOLLARS uint32_t
#define LETTERS string 



struct Worker{
	DOLLARS salary;
	LETTERS name;
};


int main(){
	string j = "Where is my";
	char* k = j.data();
	cout << std::hex << static_cast<int>(k[3]) << endl; 
	

}
