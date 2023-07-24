#pragma once


#include "exception.hpp"
#include "mock.hpp"
#include "shared.hpp"


namespace DKUtil::serialization
{
	namespace resolver
	{
		namespace detail
		{
#ifdef DKU_X_MOCK
			inline static std::unordered_map<key_type, std::uint32_t> ResolvedLayoutMap = {};
#	define DKU_X_ENTER_LAYOUT(t) INFO("\t{:->{}}"##t, "", ++detail::ResolvedLayoutMap[a_res.header.name])
#	define DKU_X_LEAVE_LAYOUT() --detail::ResolvedLayoutMap[a_res.header.name]
#	define DKU_X_CLEAR_LAYOUT() detail::ResolvedLayoutMap[a_header.name] = 0;
#else
#	define DKU_X_ENTER_LAYOUT(t)
#	define DKU_X_LEAVE_LAYOUT()
#	define DKU_X_CLEAR_LAYOUT()
#endif

#ifndef DKU_X_BUFFER_SIZE
#	define DKU_X_BUFFER_SIZE 0x200
#endif

			struct Buffer
			{
				template <typename T = void*>
				constexpr auto alloc(size_type a_size) noexcept
				{
					if (pos + a_size > data.size()) {
						exception::report<decltype(data)>(exception::code::internal_buffer_overflow);
					}

					auto* rv = static_cast<T>(data.data() + pos);
					pos += a_size;

					return rv;
				}

				constexpr auto clear() noexcept
				{
					pos = 0;
					data.fill(std::byte{ 0 });
				}

				size_type pos = 0;
				std::array<std::byte, DKU_X_BUFFER_SIZE> data = {};
			};
		}  // namespace detail


		struct ResolveInfo
		{
			ResolveOrder order;
			ISerializable::Header header;
		};


		using namespace model::concepts;

		template <typename T>
			requires(dku_trivial<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept;

		template <typename T>
			requires(dku_ranges<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept;

		template <typename T>
			requires(dku_aggregate<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept;

		template <typename T>
			requires(dku_bindable<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept;

		template <typename T>
			requires(dku_string<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept;

		template <typename T>
			requires(dku_queue<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept;

		template <typename T>
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept;


		// trivial types, int, float, bool
		template <typename T>
			requires(dku_trivial<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept
		{
			DKU_X_ENTER_LAYOUT("trivial");

			auto data = a_data;

			if (a_res.order == ResolveOrder::kSave) {
				DKU_X_WRITE(&data, sizeof(data), decltype(data));
			} else {
				DKU_X_READ(&data, sizeof(data), decltype(data));
			}

			DKU_X_LEAVE_LAYOUT();

			return data;
		}

		// ranges containers
		template <typename T>
			requires(dku_ranges<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept
		{
			DKU_X_ENTER_LAYOUT("container");

			auto data = model::vector_cast(a_data);
			auto size = static_cast<size_type>(data.size());

			if constexpr (dku_trivial_ranges<decltype(a_data)> && size == std::extent_v<decltype(a_data)>) {
				DKU_X_ENTER_LAYOUT("trivial range");

				if (a_res.order == ResolveOrder::kSave) {
					DKU_X_WRITE(&data, sizeof(data), decltype(data));
				} else {
					DKU_X_READ(&data, sizeof(data), decltype(data));

					if constexpr (std::is_same_v<decltype(data), RE::FormID>) {
						data = DKU_X_FORMID(data);
					}
				}

				DKU_X_LEAVE_LAYOUT();
			} else {
				if (a_res.order == ResolveOrder::kSave) {
					DKU_X_WRITE_SIZE(size);

					for (auto elem : data) {
						resolve(a_res, elem);
					}
				} else {
					DKU_X_READ_SIZE(size);

					data.clear();

					auto deduction = dku_value_type<decltype(data)>{};
					for (index_type i = 0; i < size; ++i) {
						data.emplace_back(resolve(a_res, deduction));
					};
				}
			}

			DKU_X_LEAVE_LAYOUT();

			return model::range_cast<decltype(a_data)>(data);
		}

		// aggregate bindables
		template <typename T>
			requires(dku_aggregate<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept
		{
			DKU_X_ENTER_LAYOUT("aggregate");

			auto data = a_data;
			auto size = static_cast<size_type>(sizeof(data));

			if (a_res.order == ResolveOrder::kSave) {
				DKU_X_WRITE_SIZE(size);
				resolve(a_res, model::tuple_cast(data));
			} else {
				DKU_X_READ_SIZE(size);

				if (size != sizeof(data)) {
					exception::report<decltype(data)>(exception::code::unexpected_type_mismatch,
						fmt::format("expected size: {}B\nread size: {}B", sizeof(data), size), a_res.header);
				};

				data = model::struct_cast<decltype(data)>(resolve(a_res, model::tuple_cast(data)));
			}

			DKU_X_LEAVE_LAYOUT();

			return data;
		}

		// bindables
		template <typename T>
			requires(dku_bindable<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept
		{
			DKU_X_ENTER_LAYOUT("bindable");

			auto data = std::apply([&](auto&&... args) {
				return decltype(a_data)(resolve(a_res, args)...);
			}, std::forward<decltype(a_data)>(a_data));

			DKU_X_LEAVE_LAYOUT();

			return data;
		}

		// string type
		template <typename T>
			requires(dku_string<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept
		{
			DKU_X_ENTER_LAYOUT("string");

			std::string data = a_data;
			auto size = static_cast<size_type>(data.size());

			if (a_res.order == ResolveOrder::kSave) {
				DKU_X_WRITE_SIZE(size);
				DKU_X_WRITE(data.data(), size, decltype(data));
			} else {
				DKU_X_READ_SIZE(size);
				data.resize(size);
				DKU_X_READ(data.data(), size, decltype(data));
			}

			DKU_X_LEAVE_LAYOUT();

			return data;
		}

		// queue
		template <typename T>
			requires(dku_queue<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept
		{
			DKU_X_ENTER_LAYOUT("queue");

			auto data = a_data;
			auto size = static_cast<size_type>(data.size());

			if (a_res.order == ResolveOrder::kSave) {
				DKU_X_WRITE_SIZE(size);

				while (!data.empty()) {
					resolve(a_res, data.front());
					data.pop();
				}
			} else {
				DKU_X_READ_SIZE(size);

				decltype(data) copy{};

				while (!data.empty()) {
					copy.push(resolve(a_res, data.front()));
					data.pop();
				}

				data = copy;
			}

			DKU_X_LEAVE_LAYOUT();

			return data;
		}

		// default
		template <typename T>
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept
		{
			exception::report<T>(exception::code::bindable_type_unpackable,
				"type unpacking is using fallback resolver, which is disabled.\n"
				"consider using a different type that has flatter data representation.",
				a_res.header);

			return a_data;
		}

		template <typename T>
		auto resolve_save(ISerializable::Header& a_header, T a_data) noexcept
		{
			using type = std::remove_cvref_t<T>;

			DEBUG("DKU_X: saving resolver -> {}\n{}", a_header.name, typeid(type).name());
			DKU_X_CLEAR_LAYOUT();

			auto info = ResolveInfo{ ResolveOrder::kSave, a_header };

			// type enforcement
			resolve(info, a_header.typeInfo);
			resolve<type>(info, a_data);
		}

		template <typename T>
		auto resolve_load(ISerializable::Header& a_header, T a_data) noexcept
		{
			using type = std::remove_cvref_t<T>;

			DEBUG("DKU_X: loading resolver -> {}\n{}", a_header.name, typeid(type).name());
			DKU_X_CLEAR_LAYOUT();

			auto info = ResolveInfo{ ResolveOrder::kLoad, a_header };

			auto check = resolve(info, a_header.typeInfo);
			if (!dku::string::iequals(check, a_header.typeInfo)) {
				exception::report<type, true>(exception::code::unexpected_type_mismatch, 
					fmt::format("ambiguous type with same hash encountered!\n"
						"this is likely an error in plugin where updated data type is still using old id.\n"
						"this is fatal, please contact the mod author.\n"
						"expected type: {}\nencountered type: {}", a_header.typeInfo, check), a_header);
			}

			return resolve<type>(info, a_data);
		}
	}  // namespace resolver
}  // namespace DKUtil::serialization
