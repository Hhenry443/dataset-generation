#include <cstdlib>
#include <iostream>
#include <ostream>
#include <thread>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string>

void runFile(std::string filename, std::string outputDir, int seed) {
// char const *args[] = {"python3", filename.c_str(), NULL};

    std::cout << "Starting: " << filename
              << " -> " << outputDir << std::endl;

    //So this will create a separate process for this BlenderProc instance.
    //So basically it can let multiple BelenderProc instances run at the same time. i think.
    pid_t pid = fork();

    if (pid == -1) {
        //It will check if the process failed.
        std::cerr << "Failed to create process." << std::endl;
        return;
    }

    if (pid == 0) {

        // child process will launch blenderproc
        // then will change the child process with the blenderproc command, going to be real only some guy on stackoverflow knows how this works
        // passing outputDir is how im making sure each instance saves its files separately, like in the python file
        // basically so the csv files and images dont keep overwriting
        // then we will go back and collect them

        execl(
            "./.venv/bin/blenderproc",
            "blenderproc",
            "run",
            filename.c_str(),
            "--output-dir",
            outputDir.c_str(),
            "--seed",
            std::to_string(seed).c_str(),
            (char *)nullptr
        );

        // if execl fails
        perror("Failed to launch BlenderProc");
        exit(EXIT_FAILURE);
    }

    // Parent process will wait for BlenderProc to finish
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        std::cout << outputDir << " completed successfully."
                  << std::endl;
    } else {
        std::cerr << outputDir << " failed."
                  << std::endl;
    }
}

int main() {

    std::string script = "./blender/test_render.py";

    // So each thread will load its own BlenderProc process.
    // Each process recieves a different output directory to stop the overwriting
    //Each thread also gets its own seed.
    std::thread t1(runFile, script, "output/thread_1", 1);
    std::thread t2(runFile, script, "output/thread_2", 2);

    t1.join();
    t2.join();

    std::cout << "Main thread finished." << std::endl;

    return 0;
}

//clang++ -std=c++17 -pthread src/main.cpp -o generator
//./generator