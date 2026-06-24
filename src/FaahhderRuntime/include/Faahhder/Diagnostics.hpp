#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace faahhder {

struct FrameSample {
    double dtMs = 0.0;
    int sprites = 0;
    int particles = 0;
    int drawCalls = 0;
};

class FrameProfiler {
public:
    void Begin();
    FrameSample End(int sprites, int particles, int drawCalls);
    void Push(FrameSample sample);
    double AverageMs() const;
    double WorstMs() const;
    std::vector<FrameSample> Samples() const;
    std::string Summary() const;
private:
    std::chrono::steady_clock::time_point begin_{};
    std::vector<FrameSample> samples_;
};

class IssueList {
public:
    void Info(std::string message);
    void Warning(std::string message);
    void Error(std::string message);
    bool HasErrors() const;
    std::string Text() const;
private:
    std::vector<std::pair<std::string, std::string>> rows_;
};

}
