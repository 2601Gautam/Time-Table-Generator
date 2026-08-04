#pragma once
#include <map>
#include <functional>

// Orders slot IDs by how many more times they still need to be placed this
// week (their "priority" = remaining weekly lecture count). Originally this
// was a hand-rolled singly linked list (`class node` + `class PriorityQ`)
// kept sorted by manually walking pointers on every insert, with a second
// O(n) linear-scan method (`dequeue_in_between`) to remove an arbitrary
// entry. A std::multimap<priority, id> keeps itself sorted (O(log n) insert)
// and iterating it front-to-back already visits entries highest-priority
// first, which is all the placement loop actually needs.
class PriorityScheduler {
public:
    void push(int id, int priority) {
        if (priority > 0) queue_.emplace(priority, id);
    }

    bool empty() const noexcept { return queue_.empty(); }
    std::size_t size() const noexcept { return queue_.size(); }

    // Scans entries from highest to lowest priority and returns the id of
    // the first one for which accept(id) is true. That entry's remaining
    // priority is decremented by one (or removed entirely if it reaches
    // zero); every other entry is left untouched. Returns -1 if nothing
    // currently in the queue satisfies accept().
    int takeBestMatching(const std::function<bool(int)>& accept) {
        for (auto it = queue_.begin(); it != queue_.end(); ++it) {
            if (!accept(it->second)) continue;
            int id = it->second;
            int remaining = it->first - 1;
            queue_.erase(it);
            if (remaining > 0) queue_.emplace(remaining, id);
            return id;
        }
        return -1;
    }

private:
    // priority -> id, kept sorted highest-priority-first.
    std::multimap<int, int, std::greater<int>> queue_;
};
