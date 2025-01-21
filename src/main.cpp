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

// ------------------------------------------------------------------------
// Minimal new function for parsing input while respecting single quotes.
// ------------------------------------------------------------------------
std::vector<std::string> parse_input_single_quotes(const std::string &input) //added
{
    std::vector<std::string> tokens;
    bool in_single_quotes = false; //added
    std::string current_token; //added

    for (size_t i = 0; i < input.size(); i++) { //added
        char c = input[i]; //added

        if (c == '\'') {
            // Toggle single-quote mode
            in_single_quotes = !in_single_quotes; //added
        }
        else if (!in_single_quotes && std::isspace(static_cast<unsigned char>(c))) {
            // Outside quotes, whitespace ends the current token
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
        }
        else {
            // Normal character, or space inside single quotes
            current_token.push_back(c);
        }
    }

    // If there's a leftover token, push it
    if (!current_token.empty()) {
        tokens.push_back(current_token);
    }

    return tokens;
}

std::vector<std::string> split(const std::string &str, char delimeter){
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while(std::getline(ss, token, delimeter)){
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

        // --------------------------------------------------------------------
        // Use our new parse_input_single_quotes() function instead of naive split
        // --------------------------------------------------------------------
        std::vector<std::string> tokens = parse_input_single_quotes(input); //added

        if (tokens.empty()) continue;

        std::string command = tokens[0];
        std::vector<const char *> args;
        for (const auto &token : tokens){
            args.push_back(token.c_str());
        }
        args.push_back(nullptr);

        // Builtin: pwd
        if (command == "pwd"){
            std::string cwd = std::filesystem::current_path();
            std::cout << cwd << "\n";
            continue;
        }

        // Builtin: cd
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

        // Builtin: type
        if (input.substr(0, 4) == "type" && input.size() > 5) {
            std::string command = input.substr(5);
            if (builtins.count(command)) {
                std::cout << command << " is a shell builtin" << std::endl;
            } else {
                std::string path = search_path(command);
                if (!path.empty()) {
                    std::cout << command << " is " << path << std::endl;
                } else {
                    std::cout << command << ": not found" << std::endl;
                }
            }
            continue;
        }

        // --------------------------------------------------------------------
        // Simpler echo: we don't parse quotes here anymore. We rely on
        // parse_input_single_quotes() to have handled them.
        // --------------------------------------------------------------------
        if (args[0] == std::string("echo")) {
            //added (simpler echo)
            for (size_t i = 1; i < tokens.size(); i++) {
                std::cout << tokens[i];
                if (i + 1 < tokens.size()) std::cout << " ";
            }
            std::cout << std::endl;
            continue;
        }

        /* -------------------------
        //removed old echo logic that re-parsed the quotes:
        // if (args[0] == std::string("echo")) { 
        //     std::string result; 
        //     bool in_quotes = false;
        //     ...
        //     std::cout << result << std::endl;
        //     continue;
        // }
        --------------------------*/

        // --------------------------------------------------------------------
        // Builtin: cat
        // Replace system() with fork+exec so single-quoted filenames work
        // --------------------------------------------------------------------
        if(args[0] == std::string("cat")) {
            //removed system(input.c_str()); //removed
            //added new fork+exec approach:
            pid_t pid = fork(); //added
            if (pid == 0) { //child //added
                execvp("cat", const_cast<char* const*>(args.data())); //added
                perror("execvp"); //added
                exit(1); //added
            } else if (pid > 0) { //parent //added
                int status; //added
                waitpid(pid, &status, 0); //added
            } else { //added
                perror("fork"); //added
            }
            continue;
        }

        // Handle external commands
        std::string full_path = search_path(command); 
        if (!full_path.empty()) { 
            pid_t pid = fork();
            if (pid == 0) { // child
                execvp(full_path.c_str(), const_cast<char *const *>(args.data()));
                perror("execvp");
                exit(1); 
            } else if (pid > 0) { // parent
                int status; 
                waitpid(pid, &status, 0);
            } else { 
                perror("fork");
            } 
            continue; 
        } 

        // Command not recognized
        std::cout << input << ": command not found" << std::endl;
    }

    return 0;
}
