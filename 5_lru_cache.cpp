#include <cassert>
#include <iostream>
#include <list>
#include <unordered_map>
#include <utility>

/*


The hash map lets you go in one direction only: from a key to its node.
 Eviction needs the opposite direction: from a node to its key.

Look at what happens when the cache is full.
You don't start from a key, you start from the list: "the least recently used element is items_.back()".
Now you have to remove it from both structures. Removing it from the list is easy, pop_back().
But to remove it from the map you need to call pos_.erase(key), and which key is that? If the node contained only the value, say 42, you'd know the value but not the key it belonged to.
The only way to find it would be to scan the whole map looking for the entry whose iterator points to that node, which is O(n) and breaks the whole design.
*/

class LRUCache
{
public:
    explicit LRUCache(int capacity) : cap_(capacity)
    {
        pos_.reserve(capacity); // avoid rehashing as the cache fills up
    }

    int get(int key)
    {
        auto it = pos_.find(key);
        if (it == pos_.end())
            return -1;

        // A get counts as a use: move the node to the front, O(1), no allocation.
        items_.splice(items_.begin(), items_, it->second);
        return it->second->second; // iterator stays valid after splice
    }

    void put(int key, int value)
    {
        if (cap_ <= 0)
            return;

        // Case 1: key exists -> update value and mark as most recent.
        if (auto it = pos_.find(key); it != pos_.end())
        {
            it->second->second = value;
            items_.splice(items_.begin(), items_, it->second);
            return;
        }

        // Case 3: key is new and cache is full -> evict least recently used (back).
        if (static_cast<int>(items_.size()) == cap_)
        {
            pos_.erase(items_.back().first); // this is why the list stores the key
            items_.pop_back();
        }

        // Case 2 (and after eviction): insert new key at the front.
        items_.emplace_front(key, value);
        pos_[key] = items_.begin();
    }

private:
    int cap_;
    std::list<std::pair<int, int>> items_;                                  // (key, value), front = most recent
    std::unordered_map<int, std::list<std::pair<int, int>>::iterator> pos_; // key -> node
};

int main()
{
    // Trace from the exercise, capacity 2.
    LRUCache c(2);
    c.put(1, 1);           // [1]
    c.put(2, 2);           // [2, 1]
    assert(c.get(1) == 1); // [1, 2]
    c.put(3, 3);           // evicts 2 -> [3, 1]
    assert(c.get(2) == -1);
    c.put(4, 4); // evicts 1 -> [4, 3]
    assert(c.get(1) == -1);
    assert(c.get(3) == 3); // [3, 4]
    assert(c.get(4) == 4); // [4, 3]

    // Extra edge cases.
    LRUCache u(2);
    u.put(1, 1);
    u.put(2, 2);
    u.put(1, 10); // update existing key, becomes most recent -> [1, 2]
    u.put(3, 3);  // must evict 2, not 1
    assert(u.get(1) == 10);
    assert(u.get(2) == -1);

    LRUCache one(1);
    one.put(1, 1);
    one.put(2, 2); // evicts 1
    assert(one.get(1) == -1);
    assert(one.get(2) == 2);

    std::cout << "All tests passed\n";
}