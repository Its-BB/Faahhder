#include "Faahhder/ExampleProject.hpp"

#include <fstream>
#include <sstream>

namespace faahhder {

void SnakeExampleProject::SetRules(SnakeRuleSet rules) {
    rules_ = rules;
}

SnakeRuleSet SnakeExampleProject::Rules() const {
    return rules_;
}

std::vector<ExampleFile> SnakeExampleProject::Files() const {
    std::vector<ExampleFile> files;
    files.push_back({"project.faahhder", "name=Snake Example\nruntime=FaahhderGame\nentry=assets/snake.faahhder\n"});

    std::ostringstream config;
    config << "title=Faahhder Snake\n";
    config << "columns=" << rules_.columns << "\n";
    config << "rows=" << rules_.rows << "\n";
    config << "cell=" << rules_.cellSize << "\n";
    config << "start_length=" << rules_.startLength << "\n";
    config << "food_score=" << rules_.foodScore << "\n";
    config << "wrap=" << (rules_.wrap ? "true" : "false") << "\n";
    config << "base_speed_ms=130\nmid_speed_ms=105\nfast_speed_ms=80\n";
    config << "mid_score=60\nfast_score=120\n";
    config << "bg=12,14,18\npanel=22,26,34\ngrid=34,40,51\n";
    config << "snake_head=80,210,145\nsnake_body=42,155,105\nfood=255,90,105\n";
    files.push_back({"assets/snake.faahhder", config.str()});

    files.push_back({"assets/scripts/snake.logic",
        "name=Faahhder Snake\n"
        "controls=Arrow keys or WASD\n"
        "restart=Space\n"
        "goal=Eat food, grow longer, avoid walls and your own body.\n"
        "on_food=grow,score\n"
        "on_wall=game_over\n"
        "on_self=game_over\n"});

    files.push_back({"assets/manifest.faahhder",
        "texture|player|textures/player.ppm\n"
        "texture|tiles|textures/tiles.ppm\n"
        "data|snake|snake.faahhder\n"
        "script|snake_logic|scripts/snake.logic\n"});
    return files;
}

bool SnakeExampleProject::WriteTo(const std::filesystem::path& root) const {
    for (const auto& file : Files()) {
        const auto target = root / file.path;
        std::filesystem::create_directories(target.parent_path());
        std::ofstream out(target);
        if (!out) return false;
        out << file.contents;
    }
    return true;
}

std::string SnakeExampleProject::Summary() const {
    std::ostringstream out;
    out << rules_.columns << "x" << rules_.rows << " grid";
    out << ", cell " << rules_.cellSize;
    out << ", start length " << rules_.startLength;
    out << ", food score " << rules_.foodScore;
    out << ", wrap " << (rules_.wrap ? "on" : "off");
    return out.str();
}

}
