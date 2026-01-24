#include<iostream>
#include <cstdint>
#include<iomanip>
#include<vector>
#include<algorithm>
using namespace std;

#define DOLLARS uint32_t
#define LETTERS string 



struct Worker{
	DOLLARS salary;
	LETTERS name;
};


int main(){
	string buffer(8, '\0');
	cout << buffer << endl;
}
