#include <iostream>
#include <string>
#include <unordered_set>

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
            if (builtins.count(command)) {
                std::cout << command << " is a shell builtin" << std::endl;
            } else {
                std::cout << command << ": not found" << std::endl;
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
