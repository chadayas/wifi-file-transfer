#include<iostream>
#include<string>
#include<vector>
#include<numeric>

int main(){
	int size = 10000;	
	std::vector<int> num(size);	
	std::iota(num.begin(), num.end(), 0);
	std::cout << "Wating";	
	for(auto i = 0; i < num.size(); i++){	
	while ( num[i] % 100 == 0){
		std::cout << "." << std::flush;
		i++;	
		}
	}

	std::cout << "\n";
}
