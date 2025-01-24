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

// New parser for single quotes
std::vector<std::string> parse_input_single_quotes(const std::string &input) {
    std::vector<std::string> tokens;
    bool quotes = false;
    std::string current_token;

    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];
        if (c == '\'' || c == '\"') {
            // Toggle single-quote mode
            quotes = !quotes;
        }
        else if (!quotes && std::isspace(static_cast<unsigned char>(c))) {
            // Outside quotes, whitespace ends the current token
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
        }
        else {
            // Normal character, or space inside quotes
            current_token.push_back(c);
        }
    }

    // Last token
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
    // Flush on every output
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // Define a set of shell builtin commands
    // Removed "cat" from this set so that "type cat" works as the test expects
    std::unordered_set<std::string> builtins = {
        "echo", "exit", "type", "pwd", "cd" 
        // "cat" //removed
    };

    while (true) {
        std::cout << "$ ";

        std::string input;
        std::getline(std::cin, input);
        if (!std::cin.good()) {
            // End if no more input
            return 0;
        }

        if (input == "exit 0") {
            return 0;
        }

        // Use new parser
        std::vector<std::string> tokens = parse_input_single_quotes(input);
        if (tokens.empty()) {
            continue;
        }

        std::string command = tokens[0];

        std::vector<const char*> args;
        for (auto &t : tokens) {
            args.push_back(t.c_str());
        }
        args.push_back(nullptr);

        // Builtin: pwd
        if (command == "pwd") {
            std::string cwd = std::filesystem::current_path();
            std::cout << cwd << "\n";
            continue;
        }

        // Builtin: cd
        if (command == "cd") {
            if (tokens.size() < 2) {
                std::cerr << "cd: missing argument\n";
            } else {
                std::string &path = tokens[1];
                if (path == "~") {
                    const char* home = std::getenv("HOME");
                    if (home) {
                        path = home;
                    } else {
                        std::cerr << "cd: HOME environment variable not set\n";
                        continue;
                    }
                }
                if (chdir(path.c_str()) == -1) {
                    std::cerr << "cd: " << tokens[1] << ": No such file or directory\n";
                }
            }
            continue;
        }

        // Builtin: type
        if (input.rfind("type", 0) == 0 && input.size() > 5) {
            // e.g. "type cat", "type ls"
            std::string c = input.substr(5);
            if (builtins.count(c)) {
                std::cout << c << " is a shell builtin" << std::endl;
            } else {
                std::string p = search_path(c);
                if (!p.empty()) {
                    std::cout << c << " is " << p << std::endl;
                } else {
                    std::cout << c << ": not found" << std::endl;
                }
            }
            continue;
        }

        // Builtin: echo
        if (command == "echo") {
            // Just print tokens after echo, separated by spaces
            for (size_t i = 1; i < tokens.size(); i++) {
                std::cout << tokens[i];
                if (i + 1 < tokens.size()) {
                    std::cout << " ";
                }
            }
            std::cout << "\n";
            continue;
        }

        // Removed cat from builtins, so remove the custom cat block:
        /*
        if (args[0] == std::string("cat")) {
            //removed because we want 'cat' recognized as external
            system(input.c_str());
            ...
        }
        */

        // If not a builtin, search PATH and run external command
        std::string full_path = search_path(command);
        if (!full_path.empty()) {
            pid_t pid = fork();
            if (pid == 0) {
                execvp(full_path.c_str(), const_cast<char* const*>(args.data()));
                perror("execvp");
                exit(1);
            } else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
            } else {
                perror("fork");
            }
        } else {
            std::cout << input << ": command not found" << std::endl;
        }
    }
    return 0;
}
