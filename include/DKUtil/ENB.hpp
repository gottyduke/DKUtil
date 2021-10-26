#ifndef DKUTIL_ENB
#define DKUTIL_ENB

/*!
 * Copyright 2020 DK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define DKUTIL_ENB_VERSION_MAJOR	1
#define DKUTIL_ENB_VERSION_MINOR	0
#define DKUTIL_ENB_VERSION_PATCH	0
#define DKUTIL_ENB_VERSION_BETA		0

/*!
 * 1.0.0
 * Implemented wrapper of Boris' ENB SDK;
 */


#ifndef __cplusplus
	#error Must be in CPP runtime
#endif

#include <cstdint>
#include <type_traits>


// ++++++++++
// + Struct +
// ++++++++++
namespace DKUtil::ENB
{
	constexpr auto GAME_ID = 0x10000006;

	
	struct Parameter
	{
		enum class ParameterType : std::int32_t
		{
			kNone = 0, //invalid
			kFloat = 1, //1 float
			kInt = 2, //1 int
			kHex = 3, //1 DWORD
			kBool = 4, //1 BOOL
			kColor3 = 5, //3 float
			kColor4 = 6, //4 float
			kVector3 = 7, //3 float
			kDword = 0x7fffffff  //unused
		};


		std::uint8_t	Data[16];
		std::uint32_t	Size;
		ParameterType	Type;
	};
	using ParameterType = Parameter::ParameterType;


	std::int32_t SizeOf(const ParameterType a_param)
	{
		switch (a_param) {
		case ParameterType::kFloat:
		case ParameterType::kInt:
		case ParameterType::kHex:
		case ParameterType::kBool:
			{
				return 4;
			}
		case ParameterType::kColor3:
		case ParameterType::kVector3:
			{
				return 4 * 3;
			}
		case ParameterType::kColor4:
			{
				return 4 * 4;
			}
		default: return 0;
		}
	}


	enum class CallbackType : std::int32_t
	{
		kNone = 0, // invalid
		kEndFrame = 1, //called at the end of frame, before displaying result on the screen
		kBeginFrame = 2, //called after frame was displayed, time between end and begin frame may be big enough to execute something heavy in separate thread
		kPreSave = 3, //called before user trying to save config, useful for restoring original parameters
		kPostLoad = 4, //called when parameters are created and loaded, useful for saving original parameters
		kDword = 0x7fffffff  //unused
	};
}


// ++++++++++
// + Export +
// ++++++++++
namespace DKUtil::ENB
{
	/// <summary>
	/// Returns version of SDK used by the ENBSeries, 1000 means 1.0, 1001 means 1.01, etc
	/// </summary>
	/// <remarks>Guaranteed compatibility for all Xxxx versions only,
	/// for example 1025 will work with sdk version 1000-1025,2025 will work with sdk version 2000-2025, etc.
	/// In best cases it's equal to desired SDK version</remarks>
	using GetSDKVersion = std::add_pointer_t<std::int32_t()>;
	
	/// <summary>
	/// Returns version of the ENBSeries, 279 means 0.279
	/// </summary>
	using GetVersion = std::add_pointer_t<std::int32_t()>;
	
	/// <summary>
	/// Returns unique GAME_ID
	/// </summary>
	/// <remarks>SkyrimSE is defined as GAME_ID</remarks>
	using GetIdentifier = std::add_pointer_t<std::int32_t()>;

	/// <summary>
	/// Prototype of callback function
	/// </summary>
	/// <param name="a_calltype">CallbackType must be used to select proper action</param>
	using CallbackFunction = std::add_pointer_t<void(__stdcall)(CallbackType a_calltype)>;

	/// <summary>
	/// Assign callback function which is executed by ENBSeries at certain moments.
	/// This helps to bypass potential bugs and increase performance.
	/// </summary>
	/// <param name="a_calltype">CallbackType must be used to select proper action</param>
	using SetCallbackFunction = std::add_pointer_t<void(CallbackFunction a_func)>;

	/// <summary>
	/// Get value of parameter
	/// </summary>
	/// <param name="a_filename">could be nullptr to access shader variables instead of configuration files</param>
	/// <returns>false if failed, because function arguments are invalid,
	/// parameter not exist, hidden or read only. Also parameters/may spawn or to be deleted when user modifying shaders.</returns>
	/// <remarks>for shader variables set filename = nullptr</remarks>
	using GetParameter = std::add_pointer_t<bool(char* a_filename, char* a_category, char* a_keyname, Parameter* a_outparam)>;

	/// <summary>
	/// Set value of parameter
	/// </summary>
	/// <param name="a_filename">could be nullptr to access shader variables instead of configuration files</param>
	/// <returns>false if failed, because function arguments are invalid,
	/// parameter not exist, hidden or read only. Also parameters/may spawn or to be deleted when user modifying shaders.</returns>
	/// <remarks>for shader variables set filename = nullptr</remarks>
	using SetParameter = std::add_pointer_t<bool(char* a_filename, char* a_category, char* a_keyname, Parameter* a_outparam)>;
}


// ++++++++
// + Init +
// ++++++++
namespace DKUtil::ENB
{
	
}
#endif