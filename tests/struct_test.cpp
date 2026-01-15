#include<iostream>
#include <cstdint>
using namespace std;

#define DOLLARS uint32_t
#define LETTERS string 

struct Worker{
	DOLLARS salary;
	LETTERS name;
};


int main(){
	Worker emp;
	auto it = (struct Worker*)&emp;
	cout << it << endl;
}
