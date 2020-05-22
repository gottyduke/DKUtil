#ifndef DKUTIL_HASH
#define DKUTIL_HASH

#include <string>


namespace DKUtil::Hash
{
    constexpr std::uint64_t fnv_prime = 1099511628211u;
    constexpr std::uint64_t fnv_offset_basis = 14695981039346656037u;
	
    constexpr auto FNV_1(std::string const& text) noexcept
    -> std::uint64_t
    {
        auto hash = fnv_offset_basis;
        for (auto it : text) {
            hash *= fnv_prime;
            hash ^= it;
        }

        return hash;
    }

    constexpr auto FNV_1A(std::string const& text) noexcept
    -> std::uint64_t
    {
        auto hash = fnv_offset_basis;
        for (auto it : text) {
            hash ^= it;
            hash *= fnv_prime;
        }

        return hash;
    }
}


#endif
