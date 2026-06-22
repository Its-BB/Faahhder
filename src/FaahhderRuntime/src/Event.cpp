#include "Faahhder/Event.hpp"

#include <algorithm>
#include <utility>

namespace faahhder {

std::vector<Events::Subscription>& Events::Subscriptions() {
    static std::vector<Subscription> subscriptions;
    return subscriptions;
}

int& Events::NextId() {
    static int id = 1;
    return id;
}

int Events::Subscribe(const std::string& type, Handler handler) {
    const int id = NextId()++;
    Subscriptions().push_back({id, type, std::move(handler)});
    return id;
}

void Events::Unsubscribe(int subscriptionId) {
    auto& subscriptions = Subscriptions();
    subscriptions.erase(std::remove_if(subscriptions.begin(), subscriptions.end(), [subscriptionId](const Subscription& item) {
        return item.id == subscriptionId;
    }), subscriptions.end());
}

void Events::Emit(const Event& event) {
    const auto subscriptions = Subscriptions();
    for (const auto& subscription : subscriptions) {
        if (subscription.type == event.type || subscription.type == "*") {
            subscription.handler(event);
        }
    }
}

void Events::Clear() {
    Subscriptions().clear();
    NextId() = 1;
}

}
