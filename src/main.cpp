#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <cstring>

std::unordered_map<std::string, std::string> ENV;

void load_env() {
    extern char **environ;
    for (char **env = environ; *env != nullptr; env++) {
        std::string entry(*env);
        auto pos = entry.find('=');
        if (pos != std::string::npos) {
            ENV[entry.substr(0, pos)] = entry.substr(pos + 1);
        }
    }
}

std::vector<std::string> parse_input_quotes(const std::string &input) {
    std::vector<std::string> tokens;
    std::string current_token;
    bool single_quoted = false;
    bool double_quoted = false;
    bool escape = false;

    for (char c : input) {
        if (escape) {
            current_token.push_back(c);
            escape = false;
        } else if (c == '\\') {
            escape = true;
        } else if (c == '\'' && !double_quoted) {
            single_quoted = !single_quoted;
        } else if (c == '\"' && !single_quoted) {
            double_quoted = !double_quoted;
        } else if (!single_quoted && !double_quoted && std::isspace(c)) {
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
        } else {
            current_token.push_back(c);
        }
    }

    if (!current_token.empty()) {
        tokens.push_back(current_token);
    }

    return tokens;
}

std::filesystem::path locate_binary(const std::string &command) {
    if (command.empty()) return "";
    const auto path_env = ENV.find("PATH");
    if (path_env == ENV.end()) return "";

    std::istringstream paths(path_env->second);
    std::string dir;
    while (std::getline(paths, dir, ':')) {
        std::filesystem::path binary_path = std::filesystem::path(dir) / command;
        if (std::filesystem::exists(binary_path) && access(binary_path.c_str(), X_OK) == 0) {
            return binary_path;
        }
    }
    return "";
}

int execute_command(const std::string &command, const std::vector<std::string> &arguments) {
    std::vector<char *> argv;
    for (const auto &arg : arguments) {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv.data());
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    } else {
        perror("fork");
        return -1;
    }
}

int main() {
    load_env();
    std::cout << std::unitbuf; // Flush stdout after every output

    const std::unordered_set<std::string> builtins = {"echo", "pwd", "cd", "type", "exit"};

    while (true) {
        std::cout << "$ ";
        std::string input;
        std::getline(std::cin, input);

        if (input.empty()) continue;

        auto tokens = parse_input_quotes(input);
        if (tokens.empty()) continue;

        const auto &command = tokens[0];

        if (command == "exit") {
            if (tokens.size() == 2) {
                return std::stoi(tokens[1]);
            }
            return 0;
        } else if (command == "echo") {
            for (size_t i = 1; i < tokens.size(); ++i) {
                std::cout << tokens[i] << (i + 1 < tokens.size() ? " " : "");
            }
            std::cout << "\n";
        } else if (command == "pwd") {
            std::cout << std::filesystem::current_path() << "\n";
        } else if (command == "cd") {
            if (tokens.size() < 2) {
                std::cerr << "cd: missing argument\n";
            } else {
                std::filesystem::path target = tokens[1] == "~" ? ENV["HOME"] : tokens[1];
                if (!std::filesystem::exists(target)) {
                    std::cerr << "cd: " << target << ": No such file or directory\n";
                } else {
                    std::filesystem::current_path(target);
                }
            }
        } else if (command == "type") {
            for (size_t i = 1; i < tokens.size(); ++i) {
                if (builtins.count(tokens[i])) {
                    std::cout << tokens[i] << " is a shell builtin\n";
                } else {
                    auto binary = locate_binary(tokens[i]);
                    if (!binary.empty()) {
                        std::cout << tokens[i] << " is " << binary << "\n";
                    } else {
                        std::cout << tokens[i] << ": not found\n";
                    }
                }
            }
        } else {
            auto binary = locate_binary(command);
            if (!binary.empty()) {
                execute_command(binary, tokens);
            } else {
                std::cerr << command << ": command not found\n";
            }
        }
    }

    return
