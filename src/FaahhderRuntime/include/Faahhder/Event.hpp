#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace faahhder {

struct Event {
    std::string type;
    int entity = -1;
    std::string message;
};

class Events {
public:
    using Handler = std::function<void(const Event&)>;

    static int Subscribe(const std::string& type, Handler handler);
    static void Unsubscribe(int subscriptionId);
    static void Emit(const Event& event);
    static void Clear();

private:
    struct Subscription {
        int id = 0;
        std::string type;
        Handler handler;
    };

    static std::vector<Subscription>& Subscriptions();
    static int& NextId();
};

}

