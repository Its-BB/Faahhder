#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace faahhder {

struct ExampleFile {
    std::filesystem::path path;
    std::string contents;
};

struct SnakeRuleSet {
    int columns = 28;
    int rows = 20;
    int cellSize = 24;
    int startLength = 3;
    int foodScore = 10;
    bool wrap = false;
};

class SnakeExampleProject {
public:
    void SetRules(SnakeRuleSet rules);
    SnakeRuleSet Rules() const;
    std::vector<ExampleFile> Files() const;
    bool WriteTo(const std::filesystem::path& root) const;
    std::string Summary() const;
private:
    SnakeRuleSet rules_;
};

}
