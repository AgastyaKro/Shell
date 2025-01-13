#include <iostream>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // Uncomment this block to pass the first stage
  while(true){
  std::cout << "$ ";

  std::string input;
  std::getline(std::cin, input);
  if (input == "exit 0"){
    return false;
  }
  if (input.substr(0,5) == "echo"){
    std::cout<< input.substr(5);
    return;
  }
  std::cout<< input << ": command not found" << std::endl;
  }
}
