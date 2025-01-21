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

// A custom parser that splits the input respecting single quotes.
std::vector<std::string> parse_input(const std::string &input) {
    std::vector<std::string> tokens;
    bool in_single_quote = false;
    std::string current_token;

    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];

        if (c == '\'') {
            // Toggle single-quote mode
            in_single_quote = !in_single_quote;
        }
        else if (!in_single_quote && std::isspace(static_cast<unsigned char>(c))) {
            // If we're not inside quotes, whitespace ends the current token
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

    // If there's something still in current_token, push it
    if (!current_token.empty()) {
        tokens.push_back(current_token);
    }

    return tokens;
}

// We reuse your old 'split()' if needed (though parse_input replaces it for command-line parsing).
std::vector<std::string> split(const std::string &str, char delimeter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimeter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string search_path(const std::string &command) {
    const char *path_env = std::getenv("PATH");
    if (!path_env) {
        return "";
    }

    std::string path(path_env);
    std::vector<std::string> directories = split(path, ':');

    for (const auto &dir : directories) {
        std::string fullpath = dir + "/" + command;
        if (access(fullpath.c_str(), X_OK) == 0) {
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
        if(!std::getline(std::cin, input)) {
            // If we fail to read, exit
            return 0;
        }

        // Parse the input into tokens, respecting single quotes
        std::vector<std::string> tokens = parse_input(input);
        if (tokens.empty()) continue;

        std::string command = tokens[0];

        // Convert tokens into a form suitable for execvp
        std::vector<const char *> args;
        for (auto &t : tokens) {
            args.push_back(t.c_str());
        }
        args.push_back(nullptr);

        // Builtin: exit
        if (command == "exit") {
            // Let's allow just "exit" to exit; or "exit 0"
            if (tokens.size() == 1 || (tokens.size() >= 2 && tokens[1] == "0")) {
                return 0;
            }
            // If you want to handle 'exit <code>', parse and return that code
            continue;
        }

        // Builtin: pwd
        if (command == "pwd") {
            std::string cwd = std::filesystem::current_path().string();
            std::cout << cwd << "\n";
            continue;
        }

        // Builtin: cd
        if (command == "cd") {
            if (tokens.size() < 2) {
                std::cerr << "cd: missing argument\n";
            } else {
                std::string path = tokens[1];
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
                    std::cerr << "cd: " << path << ": No such file or directory\n";
                }
            }
            continue;
        }

        // Builtin: type
        if (command == "type") {
            // e.g. "type cat"
            if (tokens.size() < 2) {
                std::cerr << "type: missing argument\n";
                continue;
            }
            std::string target = tokens[1];
            if (builtins.count(target)) {
                std::cout << target << " is a shell builtin\n";
            } else {
                std::string path = search_path(target);
                if (!path.empty()) {
                    std::cout << target << " is " << path << "\n";
                } else {
                    std::cout << target << ": not found\n";
                }
            }
            continue;
        }

        // Builtin: echo
        if (command == "echo") {
            // Print arguments after "echo" with a space between them
            // (Your original code was trying to handle quotes in echo only,
            //  but now our parse_input() has already handled them.)
            for (size_t i = 1; i < tokens.size(); i++) {
                std::cout << tokens[i];
                if (i + 1 < tokens.size()) std::cout << " ";
            }
            std::cout << "\n";
            continue;
        }

        // Builtin: cat
        if (command == "cat") {
            // Instead of using system(), do a fork/exec
            pid_t pid = fork();
            if (pid == 0) {
                execvp("cat", const_cast<char * const *>(args.data()));
                perror("execvp");
                exit(1);
            } else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
            } else {
                perror("fork");
            }
            continue;
        }

        // Handle external commands
        std::string full_path = search_path(command);
        if (!full_path.empty()) {
            pid_t pid = fork();
            if (pid == 0) {
                execvp(full_path.c_str(), const_cast<char * const *>(args.data()));
                perror("execvp");
                exit(1);
            } else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
            } else {
                perror("fork");
            }
        } else {
            std::cout << command << ": command not found\n";
        }
    }

    return 0;
}
