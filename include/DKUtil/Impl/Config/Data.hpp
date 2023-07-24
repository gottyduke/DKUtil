#pragma once


#include "shared.hpp"


namespace DKUtil::Config::detail
{
	enum class DataType
	{
		kBoolean,
		kDouble,
		kInteger,
		kString,

		kError,
	};


	template <typename data_t>
	struct data_trait
	{
		using type = DataType;
		static constexpr bool is_bool = std::is_same_v<data_t, bool>;
		static constexpr bool is_double = std::is_same_v<data_t, double>;
		static constexpr bool is_integer = std::is_same_v<data_t, std::int64_t>;
		static constexpr bool is_string = std::is_same_v<data_t, std::basic_string<char>>;
		static constexpr auto value()
		{
			if constexpr (is_bool)
				return type::kBoolean;
			if constexpr (is_double)
				return type::kDouble;
			if constexpr (is_integer)
				return type::kInteger;
			if constexpr (is_string)
				return type::kString;
			return type::kError;
		};
	};
	template <typename data_t>
	static constexpr auto data_trait_v = data_trait<data_t>::value();


	class IData
	{
	public:
		constexpr IData(DataType a_type) noexcept :
			_type(a_type)
		{}
		virtual constexpr ~IData() {}

		[[nodiscard]] constexpr auto get_type() const noexcept { return _type; }

		template <typename data_t>
		[[nodiscard]] auto* As();

	protected:
		DataType _type;
	};


	// automatic data with collection enabled
	template <
		typename data_t,
		DataType TYPE = data_trait_v<data_t>>
	class AData : public IData
	{
		using collection = std::vector<data_t>;
		using IData::IData;

	public:
		constexpr AData() noexcept = delete;
		constexpr AData(const std::string& a_key, const std::string& a_section = "") :
			IData(TYPE), _key(std::move(a_key)), _section(std::move(a_section))
		{}


		constexpr AData(const AData&) noexcept = delete;
		constexpr AData(AData&&) noexcept = delete;
		constexpr ~AData() = default;

		// return the front if out of bounds
		[[nodiscard]] auto& operator[](const std::size_t a_index) noexcept
			requires(!std::is_same_v<data_t, bool>)
		{
			if (_isCollection) {
				return a_index < _collection->size() ? _collection->at(a_index) : *_collection->end();
			} else {
				return _data;
			}
		}
		[[nodiscard]] constexpr operator bool() noexcept
			requires(std::is_same_v<bool, data_t>)
		{
			return _data;
		}
		[[nodiscard]] constexpr auto& operator*() noexcept { return _data; }
		[[nodiscard]] constexpr std::string_view get_key() const noexcept { return _key; }
		[[nodiscard]] constexpr std::string_view get_section() const noexcept { return _section; }
		[[nodiscard]] constexpr auto is_collection() const noexcept { return _isCollection; }

		[[nodiscard]] constexpr const auto get_data() const noexcept { return _data; }
		[[nodiscard]] constexpr auto& get_collection() noexcept
		{
			if (_isCollection) {
				return *_collection;
			} else {
				ERROR(".get_collection is called on config value {} while it holds singular data!\n\nCheck .is_collection before accessing collcetion!", _key);
			}
		}
		[[nodiscard]] constexpr auto get_size() const noexcept { return _isCollection ? _collection->size() : 0; }
		[[nodiscard]] constexpr auto get_type() const noexcept { return typeid(data_t).name(); }
		constexpr void debug_dump() const noexcept
		{
			if (_isCollection) {
				std::ranges::for_each(*_collection, [&](data_t val) {
					DEBUG("Setting collection value [{}] to [{}]", val, _key);
				});
			} else {
				DEBUG("Setting value [{}] to [{}]", _data, _key);
			}
		}

		constexpr void set_data(data_t a_value) noexcept
		{
			_isCollection = false;
			_data = a_value;

			clamp();
			debug_dump();
		}

		constexpr void set_data(const std::initializer_list<data_t>& a_list)
		{
			_collection.reset();

			_isCollection = (a_list.size() > 1);
			[[unlikely]] if (_isCollection) {
				_collection = std::make_unique<collection>(a_list);
			}

			_data = *a_list.begin();

			clamp();
			debug_dump();
		}

		constexpr void set_data(const collection& a_collection)
		{
			_collection.reset();

			_isCollection = (a_collection.size() > 1);
			[[likely]] if (_isCollection) {
				_collection = std::make_unique<collection>(std::move(a_collection));
				_data = a_collection.front();
			}

			clamp();
			debug_dump();
		}

		constexpr void set_range(std::pair<double, double> a_range)
		{
			if constexpr (model::concepts::dku_numeric<data_t>) {
				_range = a_range;
			}
		}

		constexpr void clamp()
		{
			if (!model::concepts::dku_numeric<data_t> || _range.first > _range.second) {
				return;
			}

			auto single_clamp = [this](data_t data) {
				return data < _range.first ?
				           _range.first :
				           (data > _range.second ? _range.second : data);
			};

			if (_isCollection) {
				*_collection | std::views::transform(single_clamp);
				_data = _collection->front();
			} else {
				_data = single_clamp(_data);
			}
		}


	private:
		const std::string _key;
		const std::string _section;
		bool _isCollection = false;
		data_t _data;
		std::pair<data_t, data_t> _range;
		std::unique_ptr<collection> _collection{ nullptr };
	};


	template <typename data_t>
	auto* IData::As()
	{
		return dynamic_cast<AData<data_t>*>(this);
	}


	extern template class AData<bool>;
	extern template class AData<std::int64_t>;
	extern template class AData<double>;
	extern template class AData<std::basic_string<char>>;
}  // namespace DKUtil::Config::detail
