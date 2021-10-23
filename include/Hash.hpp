#ifndef DKUTIL_HASH
#define DKUTIL_HASH

#include <string>

#define HASH(STR) DKUtil::Hash::FNV_1A(STR)
#define DECODE(TYPE) DKUtil::Hash::DecodeTypeCode(TYPE).c_str()


namespace DKUtil::Hash
{
	// FNV1a c++11 constexpr compile time hash functions, 32 and 64 bit
	// str should be a null terminated string literal, value should be left out
	// e.g hash_32_fnv1a_const("example")
	// code license: public domain or equivalent
	// post: https://notes.underscorediscovery.com/constexpr-fnv1a/
	
    constexpr std::uint64_t fnv_prime = 1099511628211u;
    constexpr std::uint64_t fnv_offset_basis = 14695981039346656037u;
	
	
    constexpr std::uint64_t FNV_1A(const char* const a_str, const std::uint64_t a_value = fnv_offset_basis) noexcept
    {
        return (a_str[0] == '\0') ? a_value : FNV_1A(&a_str[1], (a_value ^ std::uint64_t(a_str[0])) * fnv_prime);
    }

	
	std::string DecodeTypeCode(UInt32 a_typeCode)
	{
		constexpr auto SIZE = sizeof(UInt32);

		std::string sig;
		sig.resize(SIZE);
		auto* const iter = reinterpret_cast<char*>(&a_typeCode);
		for (std::size_t i = 0, j = SIZE - 2; i < SIZE - 1; ++i, --j) {
			sig[j] = iter[i];
		}

		return sig;
	}
}


#endif
