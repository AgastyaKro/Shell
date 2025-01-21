#include <iostream>
#include <string>
#include <unordered_set>
#include <cstdlib> 
#include <sstream> 
#include <vector> 
#include <filesystem> 
#include <unistd.h> 
#include <sys/wait.h>
#include <cstring>



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
    std::unordered_set<std::string> builtins = {"echo", "exit", "type", "pwd", "cd", "cat"};

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

        if (command == "cd"){
            if (tokens.size() < 2)
                std::cerr << "cd: missing argument\n";
            else {
                std::string &path = tokens[1];

                if (path == "~"){
                    const char* home = std::getenv("HOME"); 

                    if (home){
                        path = home;
                    }

                    else{
                        std::cerr << "cd: HOME environment variable not set\n";
                        continue;

                    }

                }
                if (chdir(path.c_str()) == -1){
                    std::cerr << "cd: " << tokens[1] << ": No such file or directory\n"; //added
                }
            }
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
        if (args[0] == "echo") {
            if (input[5] == '\'' || input[5] == '\"') { // Handle quoted strings
                char quote_char = input[5];
                size_t start = input.find(quote_char) + 1;
                size_t end = input.rfind(quote_char);

                if (start != std::string::npos && end != std::string::npos && start < end) {
                    std::cout << input.substr(start, end - start) << '\n';
                } else {
                    std::cerr << "Invalid quoted string\n";
                }
            } else { // Handle unquoted arguments
                for (size_t i = 1; i < args.size(); i++) {
                    if (args[i] != nullptr) {
                        std::cout << args[i];
                        if (i < args.size() - 1) std::cout << " ";
                    }
                }
                std::cout << '\n';
            }
            continue;
        }
        if (args[0] == "cat") {
            pid_t pid = fork(); // Create a child process
            if (pid == 0) { // In the child process
                execvp(args[0], const_cast<char *const *>(args.data())); // Execute 'cat'
                perror("execvp"); // Print error if execvp fails
                exit(1); // Exit the child process with an error code
            } else if (pid > 0) { // In the parent process
                int status;
                waitpid(pid, &status, 0); // Wait for the child process to finish
            } else {
                perror("fork"); // Handle fork failure
            }
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
