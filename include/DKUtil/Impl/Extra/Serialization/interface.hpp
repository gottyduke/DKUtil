#pragma once


#include "exception.hpp"
#include "shared.hpp"


namespace DKUtil::serialization
{
	namespace api
	{
		namespace detail
		{
			inline auto* check_skse_intfc(SKSE::SerializationInterface* a_intfc = nullptr)
			{
				const auto* intfc = a_intfc ? a_intfc : SKSE::GetSerializationInterface();

				if (!a_intfc && !intfc) {
					exception::report<decltype(intfc)>(exception::code::invalid_skse_interface);
				}

				return intfc;
			}


			void save_all(SKSE::SerializationInterface* a_intfc)
			{
#ifndef DKU_X_MOCK
				check_skse_intfc(a_intfc);

				for (auto* serializable : ISerializable::ManagedSerializables) {
					if (!a_intfc->OpenRecord(serializable->header.hash, serializable->header.version)) {
						exception::report<decltype(serializable)>(exception::code::failed_to_open_record,
							fmt::format("type: {}", serializable->header.typeInfo));
						continue;
					} else {
						serializable->try_save();
					}
				}
#endif
			}


			void load_all(SKSE::SerializationInterface* a_intfc)
			{
#ifndef DKU_X_MOCK
				check_skse_intfc(a_intfc);

				std::uint32_t hash, version, length;
				while (a_intfc->GetNextRecordInfo(hash, version, length)) {
					for (auto* serializable : ISerializable::ManagedSerializables) {
						serializable->try_load(hash, version);
					}
				}
#endif
			}


			void revert_all(SKSE::SerializationInterface* a_intfc)
			{
				for (auto* serializable : ISerializable::ManagedSerializables) {
					serializable->try_revert();
				}
			}
		}  // namespace detail


#ifndef DKU_X_MOCK


#	define DKU_X_WRITE(D, L, T) api::write<T>(D, L, a_res.header)
#	define DKU_X_WRITE_SIZE(D) DKU_X_WRITE(std::addressof(D), sizeof(D), decltype(data))
#	define DKU_X_READ(D, L, T) api::read(D, L, a_res.header)
#	define DKU_X_READ_SIZE(D) DKU_X_READ(std::addressof(D), sizeof(D), decltype(data))
#	define DKU_X_REPORT() api::report()
#	define DKU_X_FORMID(F) api::resolveFormId(F)


		template <typename T = void>
		inline void write(const void* a_buf, size_type a_length, const ISerializable::Header& a_header) noexcept
		{
			const auto* intfc = detail::check_skse_intfc();
			if (!intfc->WriteRecordData(a_buf, a_length)) {
				exception::report<T>(exception::code::failed_to_write_data,
					fmt::format("size: {}B", a_length), a_header);
			}
		}

		template <typename T = void>
		inline void read(void* a_buf, size_type a_length, const ISerializable::Header& a_header) noexcept
		{
			const auto* intfc = detail::check_skse_intfc();
			if (!intfc->ReadRecordData(a_buf, a_length)) {
				exception::report<T>(exception::code::failed_to_read_data,
					fmt::format("size: {}B", a_length), a_header);
			}
		}

		inline RE::FormID resolveFormId(RE::FormID a_form) noexcept
		{
			RE::FormID newForm;
			const auto* intfc = detail::check_skse_intfc();

			if (!intfc->ResolveFormID(a_form, newForm) && !newForm) {
				exception::report<RE::FormID>(exception::code::failed_to_resolve_formId,
					fmt::format("old formId: {:X} | resolved: {:X}", a_form, newForm));
			}

			return newForm;
		}

		inline void report() noexcept
		{
			const auto* intfc = detail::check_skse_intfc();
			DEBUG("\nSKSE::SerializationInterface version: {}\nDKU_X_SERIALIZE version: {}", intfc->Version(), DKU_XS_VERSION);
		}
#endif


		inline void RegisterSerializable(std::string a_ref = {}) noexcept
		{
			const auto* intfc = detail::check_skse_intfc();

			a_ref += "_DKU_XS";
			auto&& [key, hash] = colliding::make_hash_key(a_ref.data());

			intfc->SetUniqueID(hash);
			intfc->SetSaveCallback(detail::save_all);
			intfc->SetLoadCallback(detail::load_all);
			intfc->SetRevertCallback(detail::revert_all);

			DEBUG("DKU_XS: Registered serializables\nplugin handle {}\nplugin hash {}", key, hash);
			DKU_X_REPORT();
		}
	}  // DKUtil::serialization::api
}  // DKUtil::serialization