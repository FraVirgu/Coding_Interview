#include <vector>
#include <string>
#include <string_view>
/*
Your task:

Find at least 5 performance problems.
For each one, say what costs and why (an allocation? a copy? a reallocation?).
Rewrite the function to remove them.
Tell me the complexity of your version, and how many heap allocations it performs in the best case.

*/
std::vector<std::string> filter(std::vector<std::string> v, std::string prefix)
{
    std::vector<std::string> out;
    for (auto s : v)
        if (s.substr(0, prefix.size()) == prefix)
            out.push_back(s);
    return out;
}

/*
out => max size v.size()  PREALLOCATE
s is a copy of each elem of v => don't create a new elem
*/

std::vector<std::string> perf_filter(const std::vector<std::string> &v, std::string_view prefix)
{
    std::vector<std::string> out;
    out.reserve(v.size());
    for (const auto &s : v)
        if (s.starts_with(prefix))
            out.push_back(s);
    return out;
}
