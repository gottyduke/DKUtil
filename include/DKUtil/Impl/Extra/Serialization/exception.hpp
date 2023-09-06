#pragma once


#include "shared.hpp"


namespace DKUtil::serialization
{
	namespace exception
	{
		enum class code : index_type
		{
			failed_to_open_record,
			failed_to_write_data,
			failed_to_read_data,
			failed_to_resolve_formId,
			failed_to_revert,

			unexpected_type_mismatch,
			bindable_type_unpackable,
			internal_buffer_overflow,
			invalid_skse_interface,
		};

#define DKU_XS_EXCEPTION_FMT                                                    \
	"Serialization error\n[{}]\n\nrecord: {}\nversion: {}\ntype: {}\n\n{}\n\n", \
		dku::print_enum(a_code), a_header.name, a_header.version, typeid(T).name(), a_fmt

		template <typename T, bool PROMPT = DKU_X_STRICT_SERIALIZATION>
		void report(
			code a_code,
			std::string_view a_fmt = {},
			ISerializable::Header a_header = { "DKU_X: <intenral resolver>", 0x1234, DKU_XS_VERSION })
		{
			if constexpr (PROMPT) {
				ERROR(DKU_XS_EXCEPTION_FMT);
			} else {
				WARN(DKU_XS_EXCEPTION_FMT);
			}
		}
	}  // namespace exception
}  // namespace DKUtil::serialization