#pragma once


#include "shared.hpp"


namespace DKUtil::serialization
{
	namespace exception
	{
		enum class code : std::uint32_t
		{
			failed_to_open_record,
			failed_to_write_size,
			failed_to_write_data,

			failed_to_read_size,
			failed_to_read_data,
			failed_to_resolve_formId,

			failed_to_revert,

			unsupported_type_queue,
			unexpected_type_mismatch,
			bindable_type_unpackable,
			internal_buffer_overflow,
		};

		template <typename T, typename... Fmt>
		void report(
			code a_code, 
			ISerializable::Header a_header = { "DKU_X: <intenral resolver>", 0x1234, DKU_XS_VERSION }, 
			std::string_view a_fmt = {})
		{
			ERROR("Serialization error\n[{}]\n\nrecord: {}\nversion: {}\ntype: {}\n\n{}\n\n",
				dku::print_enum(a_code), a_header.name, a_header.version, typeid(T).name(), a_fmt);
		}
	}  // namespace exception
} // namespace DKUtil::serialization