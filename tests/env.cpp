#include<cstdlib>
#include<iostream>

int main(){
	const char* env = std::getenv("HOME");
	if(env == NULL){
		std::cout << "cant find it" << std::endl;
		std::cout << !env << std::endl;
	}else
		std::cout << !env << std::endl;
}
