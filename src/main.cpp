#include <iostream>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

    // Main 
    while (true) {
        std::cout << "$ ";

        std::string input;
        std::getline(std::cin, input);

        if (input == "exit 0") {
            return 0; // Exits the program
        }

        if (input.substr(0, 4) == "echo" && input.size() > 5) {
            std::cout << input.substr(5) << std::endl;
        } else {
            std::cout << input << ": command not found" << std::endl;
        }
    }

    return 0;
}
