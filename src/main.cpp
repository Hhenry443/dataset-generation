#include <cstdlib>
#include <iostream>
#include <ostream>
#include <thread>
#include <unistd.h>
#include <string>

void runFile(std::string filename) {
    // char const *args[] = {"python3", filename.c_str(), NULL};
    
    std::cout << filename << std::endl;
    char temp[512];

    sprintf(temp, "./blender/.venv/bin/blenderproc run %s", filename.c_str());
    system((char *)temp);
}

int main() {
    std::string str = "./blender/test_render.py";
    
    std::thread t1(runFile, str);
    std::thread t2(runFile, str);

    t1.join();
    t2.join();

    std::cout << "Main thread finished." << std::endl;
    
    return 0;
}
