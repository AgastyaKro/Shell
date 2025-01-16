#include <iostream>
#include <string>
#include <unordered_set>
#include <cstdlib> // added
#include <sstream> // added
#include <vector> // added
#include <filesystem> // added
#include <unistd.h> // for access(), added

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
    std::unordered_set<std::string> builtins = {"echo", "exit", "type"};

    while (true) {
        std::cout << "$ ";

        std::string input;
        std::getline(std::cin, input);

        if (input == "exit 0") {
            return 0; // Exits the program
        }

        // Handle `type` builtin
        if (input.substr(0, 4) == "type" && input.size() > 5) {
            std::string command = input.substr(5);
            if (builtins.count(command)) { // will be 1 -> true if exists in builtins
                std::cout << command << " is a shell builtin" << std::endl;
            } else {
                std::string path = search_path(command); // renamed function call
                if (!path.empty()) { // added
                    std::cout << command << " is " << path << std::endl; // added
                } else { // added
                    std::cout << command << ": not found" << std::endl; // added
                } // added
            }

            continue;
        }

        // Handle `echo` command
        if (input.substr(0, 4) == "echo" && input.size() > 5) {
            std::cout << input.substr(5) << std::endl;
            continue;
        }

        // Command not recognized
        std::cout << input << ": command not found" << std::endl;
    }

    return 0;
}
