#include <algorithm>
#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
using namespace std;

#include <vector>
#include <map>
#include <unordered_map>

// COMPLEXITY SOLUTION => O(n + nlog(n) + k)
std::vector<int> topKFrequent(const std::vector<int> &nums, int k)
{
    // Count occurrences => O(n)
    std::unordered_map<int, int> count;
    std::vector<int> k_best;
    for (int n : nums)
    {
        count[n]++;
    }

    // Invert: frequency -> number (multimap keeps sorted by frequency)    O(nlog(n))
    std::multimap<int, int> byFreq;
    for (const auto &[num, freq] : count)
    {
        byFreq.insert({freq, num}); // O(log(n))
    }

    // Take the k highest frequencies, walking from the end
    std::vector<int> result;
    for (auto it = byFreq.rbegin(); it != byFreq.rend() && (int)result.size() < k; ++it)
    {
        result.push_back(it->second);
    }
    return result;
}

// COMPLEXITY SOLUTION => O(n)
std::vector<int> topKFrequent(const std::vector<int> &nums, int k)
{
    std::unordered_map<int, int> count;
    std::vector<vector<int>> freq_vect(nums.size() + 1);
    std::vector<int> solution;
    for (auto n : nums)
    {
        count[n]++;
    }
    for (const auto &[num, freq] : count)
    {
        freq_vect[freq].push_back(num);
    }
    for (int f = freq_vect.size(); f >= 1 && solution.size() < k; f--)
    {
        for (int final_n : freq_vect[f])
        {
            solution.push_back(final_n);
        }
    }
}