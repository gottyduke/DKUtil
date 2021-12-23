#include "DKUtil/GUI.hpp"
#include "DKUtil/Hook.hpp"


namespace Test::GUI
{
	using namespace DKUtil::Alias;

	static bool Show = false;


	void SimpleWindow() noexcept
	{
		if (Show) {
            // render
            ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f), 0);
			ImGui::Begin("DKU_G Test Window");



			ImGui::End();
		}
	}


	void Start()
	{
        // load resource


		Show = true;
	}


	void Stop()
	{
        // unload resource


		Show = false;
	}


    void Install()
    {
        DKUtil::GUI::AddCallback(FUNC_INFO(SimpleWindow));
        DKUtil::GUI::InitGUI();

        INFO("GUI installed!"sv);
    }


	void Uninstall()
	{
        Stop();
		DKUtil::GUI::StopAll();

		INFO("GUI uninstalled!"sv);
	}
}