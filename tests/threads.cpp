#include<iostream>
#include<thread>
#include<string>
#include<vector>
#include<mutex>


std::mutex cout_mtx;

void foo(){
	std::lock_guard<std::mutex> lock(cout_mtx); 		
	std::string name = "Chad boy";
	std::cout << "THREAD 1" << std::endl;	
	for (const auto& x : name)
		std::cout << x << " ";
	std::cout << std::endl;
}

void bar(){
	std::lock_guard<std::mutex> lock(cout_mtx); 		
	std::vector<int> zahlen = {1,2,3,4,5} ;
	std::cout << "THREAD 2" << std::endl;	
	for (const auto& x : zahlen)
		std::cout << x << " ";   
	std::cout << std::endl;
}

int main()
{
	std::thread t1(foo);
	std::thread t2(bar);
	t2.join(); 
	t1.join();

}
