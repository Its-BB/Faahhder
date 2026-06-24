#include "Faahhder/Diagnostics.hpp"

#include <algorithm>
#include <sstream>

namespace faahhder {

void FrameProfiler::Begin() {
    begin_ = std::chrono::steady_clock::now();
}

FrameSample FrameProfiler::End(int sprites, int particles, int drawCalls) {
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double, std::milli> elapsed = end - begin_;
    FrameSample sample{elapsed.count(), sprites, particles, drawCalls};
    Push(sample);
    return sample;
}

void FrameProfiler::Push(FrameSample sample) {
    samples_.push_back(sample);
    if (samples_.size() > 240) samples_.erase(samples_.begin());
}

double FrameProfiler::AverageMs() const {
    if (samples_.empty()) return 0.0;
    double total = 0.0;
    for (const auto& sample : samples_) total += sample.dtMs;
    return total / static_cast<double>(samples_.size());
}

double FrameProfiler::WorstMs() const {
    double worst = 0.0;
    for (const auto& sample : samples_) worst = std::max(worst, sample.dtMs);
    return worst;
}

std::vector<FrameSample> FrameProfiler::Samples() const {
    return samples_;
}

std::string FrameProfiler::Summary() const {
    std::ostringstream out;
    out << "avg=" << AverageMs() << "ms worst=" << WorstMs() << "ms samples=" << samples_.size();
    if (!samples_.empty()) {
        const auto& last = samples_.back();
        out << " sprites=" << last.sprites << " particles=" << last.particles << " draws=" << last.drawCalls;
    }
    return out.str();
}

void IssueList::Info(std::string message) {
    rows_.push_back({"info", std::move(message)});
}

void IssueList::Warning(std::string message) {
    rows_.push_back({"warning", std::move(message)});
}

void IssueList::Error(std::string message) {
    rows_.push_back({"error", std::move(message)});
}

bool IssueList::HasErrors() const {
    return std::any_of(rows_.begin(), rows_.end(), [](const auto& row) { return row.first == "error"; });
}

std::string IssueList::Text() const {
    std::ostringstream out;
    for (const auto& row : rows_) out << "[" << row.first << "] " << row.second << "\n";
    return out.str();
}

}
