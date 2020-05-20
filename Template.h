#ifndef DKUTIL_TEMPLATE
#define DKUTIL_TEMPLATE

#include <memory>


namespace DKUtil::Template
{
	template <class DERIVED>
	class SDM
	{
	public:
		static DERIVED* GetSingleton()
		{
			static DERIVED singleton;
			return std::addressof(singleton);
		}


		SDM(const SDM&) = delete;
		SDM(SDM&&) = delete;
		SDM& operator=(const SDM&) = delete;
		SDM& operator=(SDM&&) = delete;

	protected:
		SDM() = default;
		virtual ~SDM() = default;
	};
}


#endif
