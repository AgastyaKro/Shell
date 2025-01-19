#include <iostream>
#include <string>
#include <unordered_set>
#include <cstdlib> 
#include <sstream> 
#include <vector> 
#include <filesystem> 
#include <unistd.h> 
#include <sys/wait.h>

std::vector<std::string> split (const std::string &str, char delimeter){
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while(std::getline(ss,token, delimeter)){
        tokens.push_back(token);
    }
    return tokens;
}

std::string search_path(const std::string &command){
    const char *path_env = std::getenv("PATH");
    if (!path_env){
        return "";
    }

    std::string path(path_env);
    std::vector<std::string> directories = split(path, ':');

    for (const auto &dir : directories){
        std::string fullpath = dir + "/" + command;
        if (access(fullpath.c_str(), X_OK) == 0){
            return fullpath;
        }
    }
    return "";
}

int main() {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // Define a set of shell builtin commands
    std::unordered_set<std::string> builtins = {"echo", "exit", "type", "pwd"};

    while (true) {
        std::cout << "$ ";

        std::string input;
        std::getline(std::cin, input);

        if (input == "exit 0") {
            return 0; // Exits the program
        }

        std::vector<std::string> tokens = split(input, ' ');
        if (tokens.empty()) continue;

        std::string command = tokens[0];
        std::vector<const char *> args;
        for (const auto &token : tokens){
            args.push_back(token.c_str());
        }
        args.push_back(nullptr);

        if (command == "pwd"){
            std::string cwd = std::filesystem::current_path();
            std::cout << cwd << "\n";
            continue;
        }

        // Handle `type` builtin
        if (input.substr(0, 4) == "type" && input.size() > 5) {
            std::string command = input.substr(5);
            if (builtins.count(command)) { // will be 1 -> true if exists in builtins
                std::cout << command << " is a shell builtin" << std::endl;
            } else {
                std::string path = search_path(command); // renamed function call
                if (!path.empty()) { 
                    std::cout << command << " is " << path << std::endl; 
                } else { 
                    std::cout << command << ": not found" << std::endl; 
                } 
            }

            continue;
        }

        // Handle `echo` command
        if (input.substr(0, 4) == "echo" && input.size() > 5) {
            std::cout << input.substr(5) << std::endl;
            continue;
        }

        // Handle external commands and creates child processes w/out termenating the parent proccess
        std::string full_path = search_path(command); 
        if (!full_path.empty()) { 
            pid_t pid = fork(); // Creates a new child process
            if (pid == 0) { // child
                execvp(full_path.c_str(), const_cast<char *const *>(args.data()));  // Replaces the current process image 
                // (the child process) with the external program located at full_path.
                perror("execvp"); // if fails
                exit(1); 
            } else if (pid > 0) { // parent
                int status; 
                waitpid(pid, &status, 0); // Makes the parent process wait until the child process (external program) finishes.
            } else { 
                perror("fork"); // fails handler
            } 
            continue; 
        } 


        // Command not recognized
        std::cout << input << ": command not found" << std::endl;
    }

    return 0;
}
