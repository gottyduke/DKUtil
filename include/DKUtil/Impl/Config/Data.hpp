#pragma once


#include "Shared.hpp"


namespace DKUtil::Config::detail
{
	template <typename data_t>
	concept dku_c_numeric =
		(std::convertible_to<data_t, std::int64_t> || std::convertible_to<data_t, double>)&&!std::convertible_to<data_t, std::basic_string<char>>;


	template <typename data_t>
	concept dku_c_trivial_t =
		(dku_c_numeric<data_t> || std::convertible_to<data_t, bool>)&&!std::convertible_to<data_t, std::basic_string<char>>;


	enum class DataType
	{
		kInteger,
		kDouble,
		kBoolean,
		kString,

		kError
	};


	class IData
	{
	public:
		virtual ~IData() {}
	};


	// automatic data with collection enabled
	template <typename underlying_data_t>
	class AData : public IData
	{
		using collection = std::vector<underlying_data_t>;
		using IData::IData;

	public:
		constexpr AData() noexcept = delete;
		constexpr AData(const std::string& a_key, const std::string& a_section = "") :
			_key(std::move(a_key)), _section(std::move(a_section))
		{
		}


		constexpr AData(const AData&) noexcept = delete;
		constexpr AData(AData&&) noexcept = delete;
		constexpr ~AData() = default;

		// return the front if out of bounds
		[[nodiscard]] auto& operator[](const std::size_t a_index) noexcept
			requires(!std::is_same_v<underlying_data_t, bool>)
		{
			if (_isCollection) {
				return a_index < _collection->size() ? _collection->at(a_index) : *_collection->end();
			} else {
				return _data;
			}
		}
		[[nodiscard]] constexpr operator bool() noexcept requires(std::is_same_v<bool, underlying_data_t>) { return _data; }
		[[nodiscard]] constexpr auto& operator*() noexcept { return _data; }
		[[nodiscard]] constexpr auto& operator=(const std::convertible_to<underlying_data_t> auto& a_rhs) noexcept { return _data = static_cast<underlying_data_t>(a_rhs); }

		[[nodiscard]] constexpr auto get_key() const noexcept { return _key.c_str(); }
		[[nodiscard]] constexpr auto get_section() const noexcept { return _section.c_str(); }
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
		[[nodiscard]] constexpr auto get_type() const noexcept { return typeid(underlying_data_t).name(); }

		constexpr void set_data(const std::initializer_list<underlying_data_t>& a_list)
		{
			_collection.reset();

			_isCollection = (a_list.size() > 1);
			[[unlikely]] if (_isCollection)
			{
				_collection = std::make_unique<collection>(a_list);
			}

			_data = *a_list.begin();

			clamp();

			[[unlikely]] if (_isCollection)
			{
				std::for_each(_collection->begin(), _collection->end(), [&](underlying_data_t val) {
					DEBUG("Setting collection value [{}] to [{}]", val, _key);
				});
			}
			else
			{
				DEBUG("Setting value [{}] to [{}]", _data, _key);
			}
		}

		constexpr void set_data(const collection& a_collection)
		{
			_collection.reset();

			_isCollection = (a_collection.size() > 1);
			[[likely]] if (_isCollection)
			{
				_collection = std::make_unique<collection>(std::move(a_collection));
				_data = a_collection.front();
				clamp();
			}

			[[likely]] if (_isCollection)
			{
				std::for_each(_collection->begin(), _collection->end(), [&](underlying_data_t val) {
					DEBUG("Setting collection value [{}] to [{}]", val, _key);
				});
			}
			else
			{
				DEBUG("Setting value [{}] to [{}]", _data, _key);
			}
		}

		constexpr void set_range(std::pair<double, double> a_range)
		{
			if (dku_c_numeric<underlying_data_t>) {
				_range = a_range;
			}
		}

		constexpr void clamp()
		{
			if (!dku_c_numeric<underlying_data_t> || _range.first > _range.second) {
				return;
			}

			if (_isCollection) {
				std::transform(_collection->begin(), _collection->end(), _collection->begin(), [&](underlying_data_t data) -> underlying_data_t {
					return data < _range.first ?
					           _range.first :
					           (data > _range.second ? _range.second : data);
				});
				_data = _collection->front();
			} else {
				_data = _data < _range.first ?
				            _range.first :
				            (_data > _range.second ? _range.second : _data);
			}
		}


	private:
		const std::string _key;
		const std::string _section;
		bool _isCollection = false;
		underlying_data_t _data;
		std::pair<underlying_data_t, underlying_data_t> _range;
		std::unique_ptr<collection> _collection{ nullptr };
	};


	extern template class AData<bool>;
	extern template class AData<std::int64_t>;
	extern template class AData<double>;
	extern template class AData<std::basic_string<char>>;


	class DataManager
	{
	public:
		[[nodiscard]] auto Visit(const std::string_view a_key) noexcept
		{
			if (!_managed.contains(a_key)) {
				return detail::DataType::kError;
			}

			if (auto intData = dynamic_cast<AData<std::int64_t>*>(_managed[a_key]); intData) {
				return DataType::kInteger;
			} else if (auto doubleData = dynamic_cast<AData<double>*>(_managed[a_key]); doubleData) {
				return DataType::kDouble;
			} else if (auto boolData = dynamic_cast<AData<bool>*>(_managed[a_key]); boolData) {
				return DataType::kBoolean;
			} else if (auto strData = dynamic_cast<AData<std::basic_string<char>>*>(_managed[a_key]); strData) {
				return DataType::kString;
			} else {
				return DataType::kError;
			}
		}


		void SetByKey(const std::string_view a_key, const dku_c_trivial_t auto&... a_value) noexcept
		{
			if (!_managed.contains(a_key)) {
				return;
			}

			if (auto intData = dynamic_cast<AData<std::int64_t>*>(_managed[a_key]); intData) {
				intData->set_data({ static_cast<std::int64_t>(a_value)... });
			} else if (auto doubleData = dynamic_cast<AData<double>*>(_managed[a_key]); doubleData) {
				doubleData->set_data({ static_cast<double>(a_value)... });
			} else if (auto boolData = dynamic_cast<AData<bool>*>(_managed[a_key]); boolData) {
				boolData->set_data({ static_cast<bool>(a_value)... });
			}
		}


		// string is such a pain in the
		void SetByKey(const std::string_view a_key, const std::convertible_to<std::basic_string<char>> auto&... a_value) noexcept
		{
			if (!_managed.contains(a_key)) {
				return;
			}

			if (auto data = dynamic_cast<AData<std::basic_string<char>>*>(_managed[a_key]); data) {
				data->set_data({ static_cast<std::basic_string<char>>(a_value)... });
			}
		}


		void Add(const std::string_view a_key, IData* a_data) noexcept { _managed.try_emplace(a_key, a_data); }
		[[nodiscard]] auto size() const noexcept { return _managed.size(); }
		[[nodiscard]] auto& get_managed() noexcept { return _managed; }

	private:
		std::unordered_map<std::string_view, IData*> _managed;  // key, data*
	};
}  // namespace DKUtil::Config::detail
