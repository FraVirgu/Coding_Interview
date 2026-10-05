#include <vector>
#include <set>
#include <algorithm>

// O(n*k) time, O(1) extra space
std::vector<int> bruteForce(const std::vector<int> &nums, int k)
{
    std::vector<int> output;
    if (k <= 0 || nums.size() < (size_t)k)
        return output;
    for (size_t i = 0; i + k <= nums.size(); i++)
    {
        int mx = nums[i];
        for (size_t j = i + 1; j < i + k; j++)
            mx = std::max(mx, nums[j]);
        output.push_back(mx);
    }
    return output;
}

// O(n log k) time, O(k) extra space
std::vector<int> balancedTree(const std::vector<int> &nums, int k)
{
    std::vector<int> output;
    if (k <= 0 || nums.size() < (size_t)k)
        return output;
    std::multiset<int> window(nums.begin(), nums.begin() + k);
    output.push_back(*window.rbegin());
    for (size_t i = k; i < nums.size(); i++)
    {
        window.erase(window.find(nums[i - k])); // erase exactly ONE copy
        window.insert(nums[i]);
        output.push_back(*window.rbegin());
    }
    return output;
}

#include <vector>
#include <deque>

// O(n) time, O(k) extra space
std::vector<int> maxSlidingWindow(const std::vector<int> &nums, int k)
{
    std::vector<int> output;
    if (k <= 0 || nums.size() < (size_t)k)
        return output;

    std::deque<size_t> dq; // stores INDICES; their values are decreasing front to back

    for (size_t i = 0; i < nums.size(); i++)
    {
        // 1. Drop the front if it has slid out of the window [i-k+1, i]
        if (!dq.empty() && dq.front() + k <= i)
            dq.pop_front();

        // 2. Drop smaller elements from the back: they can never be a max again
        while (!dq.empty() && nums[dq.back()] <= nums[i])
            dq.pop_back();

        // 3. Add the new element
        dq.push_back(i);

        // 4. Once the first full window exists, the front is the max
        if (i + 1 >= (size_t)k)
            output.push_back(nums[dq.front()]);
    }
    return output;
}