#include "DKUtil/GUI.hpp"
#include "DKUtil/Hook.hpp"


namespace Test::GUI
{
	using namespace DKUtil::Alias;

	static bool Show = false;

	void SimpleWindow() noexcept
	{
		if (Show) {
			ImGui::Begin("DKU_G Test Window");

			ImGui::Text("This is a hello string from DKU_G");

			ImGui::End();
		}
	}


	HookHandle _Hook_P;


	void Install()
	{
		DKUtil::GUI::AddCallback(FUNC_INFO(SimpleWindow));
		DKUtil::GUI::InitD3D();

		INFO("GUI installed!"sv);
	}


	void Start()
	{
		Show = true;
	}


	void Stop()
	{
		Show = false;
	}


	void Uninstall()
	{
		DKUtil::GUI::StopAll();

		INFO("GUI uninstalled!"sv);
	}
}