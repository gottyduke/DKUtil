#pragma once


#include "shared.hpp"
#include "exception.hpp"
#include "mock.hpp"


namespace DKUtil::serialization
{
	namespace resolver
	{
		namespace detail
		{
#ifdef DKU_X_MOCK
#	define DKU_X_ENTER_LAYOUT(t) INFO("\t{:->{}}"##t, "", ++detail::ResolvedLayoutMap[a_res.header.name])
#	define DKU_X_LEAVE_LAYOUT() --detail::ResolvedLayoutMap[a_res.header.name]
#else
#	define DKU_X_ENTER_LAYOUT(t)
#	define DKU_X_LEAVE_LAYOUT()
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


			inline static std::unordered_map<key_type, std::uint32_t> ResolvedLayoutMap = {};
		} // namespace detail

		
		struct ResolveInfo
		{
			ResolveOrder order;
			ISerializable::Header header;
			SKSE::SerializationInterface* intfc;
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

		template <typename T, std::size_t N>
			requires(dku_bindable<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept;

		template <typename T>
			requires(dku_string<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept;

		template <typename T>
			requires(std::is_void_v<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept;

		template <typename T>
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept;


		// trivial types, int, float, bool
		template <typename T>
			requires(dku_trivial<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept
		{
			DKU_X_ENTER_LAYOUT("trivial");

			T data = a_data;

			if (a_res.order == ResolveOrder::kSave) {
				DKU_X_MOCK_WRITE(&data, sizeof(data));
			} else {
				DKU_X_MOCK_READ(&data, sizeof(data));
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

			if constexpr (dku_trivial_ranges<T> && size == std::extent_v<T>) {
				DKU_X_ENTER_LAYOUT("trivial range");

				if (a_res.order == ResolveOrder::kSave) {
					DKU_X_MOCK_WRITE(&data, sizeof(data));
				} else {
					DKU_X_MOCK_READ(&data, sizeof(data));
				}

				DKU_X_LEAVE_LAYOUT();
			} else {
				if (a_res.order == ResolveOrder::kSave) {
					DKU_X_MOCK_WRITE(&size, sizeof(size));

					for (auto elem : data) {
						resolve(a_res, elem);
					}
				} else {
					DKU_X_MOCK_READ(&size, sizeof(size));
					data.clear();
					data.resize(size);

					for (index_type i = 0; i < size; ++i) {
						data.emplace_back(resolve(a_res, data[i]));
					};
				}
			}

			DKU_X_LEAVE_LAYOUT();

			return model::range_cast<T>(data);
		}

		// aggregate bindables
		template <typename T>
			requires(dku_aggregate<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept
		{
			DKU_X_ENTER_LAYOUT("aggregate");

			T data = a_data;
			auto size = static_cast<size_type>(sizeof(data));

			if (a_res.order == ResolveOrder::kSave) {
				DKU_X_MOCK_WRITE(&size, sizeof(size));
				resolve(a_res, model::tuple_cast(data));
			} else {
				DKU_X_MOCK_READ(&size, sizeof(size));

				if (size != sizeof(data)) {
					exception::report<T>(exception::code::unexpected_type_mismatch, a_res.header, 
						fmt::format("expected size: {}B\nread size: {}B", sizeof(data), size));
				};

				resolve(a_res, model::tuple_cast(data));
				DKU_X_MOCK_READ(&data, size);
			}

			DKU_X_LEAVE_LAYOUT();

			return data;
		}

		// bindables base
		template <typename T>
			requires(dku_bindable<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept
		{
			DKU_X_ENTER_LAYOUT("bindable base");

			T data = a_data;
			auto size = static_cast<size_type>(sizeof(data));

			if (a_res.order == ResolveOrder::kSave) {
				DKU_X_MOCK_WRITE(&size, sizeof(size));

				resolve<T, std::tuple_size_v<T>>(a_res, data);
			} else {
				DKU_X_MOCK_READ(&size, sizeof(size));

			}

			DKU_X_LEAVE_LAYOUT();

			return data;
		}

		// bindables iterator
		template <typename T, std::size_t N>
			requires(dku_bindable<T>)
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept
		{
			DKU_X_ENTER_LAYOUT("bindable iterator");

			T data = a_data;

			resolve(a_res, std::get<N - 1>(a_data));

			DKU_X_LEAVE_LAYOUT();

			if constexpr (N > 1) {
				resolve<T, N - 1>(a_res, a_data);
			}

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
				DKU_X_MOCK_WRITE(&size, sizeof(size));
				DKU_X_MOCK_WRITE(data.data(), size);
			} else {
				DKU_X_MOCK_READ(&size, sizeof(size));
				data.resize(size);
				DKU_X_MOCK_READ(data.data(), size);
			}

			DKU_X_LEAVE_LAYOUT();

			return data;
		}

		// default
		template <typename T>
		auto resolve(const ResolveInfo& a_res, T a_data) noexcept
		{
			if (dku_queue<T>) {
				exception::report<T>(exception::code::unsupported_type_queue, a_res.header,
					"std::queue<T> is not supported by DKU_X serializer.\n"
					"consider using a different type.");
			} else {
				exception::report<T>(exception::code::bindable_type_unpackable, a_res.header,
					"type unpacking is using fallback resolver, which is disabled.\n"
					"consider using a different type that has flatter data representation.");
			}
		}

		template <typename T>
		auto resolve_save(SKSE::SerializationInterface* a_intfc, ISerializable::Header& a_header, T a_data) noexcept
		{
			using type = std::remove_cvref_t<T>;

			DEBUG("DKU_X: resolving for saving -> {}\n{}", a_header.name, typeid(type).name());

			detail::ResolvedLayoutMap[a_header.name] = 0;

			resolve<type>({ ResolveOrder::kSave, a_header, a_intfc }, a_data);
		}
	}  // namespace resolver
} // namespace DKUtil::serialization
