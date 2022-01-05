#include "DKUtil/GUI.hpp"
#include "DKUtil/Hook.hpp"


// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples#Example-for-DirectX11-users
#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"


// Simple helper function to load an image into a DX11 texture with common settings
bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    // Load from disk into a raw RGBA buffer
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create texture
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    DKU_G_DEVICE->CreateTexture2D(&desc, &subResource, &pTexture);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    DKU_G_DEVICE->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(image_data);

    return true;
}


int my_image_width = 0;
int my_image_height = 0;
ID3D11ShaderResourceView* my_texture = nullptr;


namespace Test::GUI
{
	using namespace DKUtil::Alias;

	static bool Show = false;


	void SimpleWindow() noexcept
	{
        if (Show) {
            ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f));
            ImGui::Begin("Unique Window Name");
            ImGui::Image(std::bit_cast<void*>(my_texture), ImVec2(my_image_width, my_image_height));

            ImGui::End();
        }
	}


	void Start()
	{
		// load resource, after game process resumed from skse
        if (my_texture) {
            my_texture->Release();
        }

        LoadTextureFromFile("Data\\SKSE\\Plugins\\Works.png", &my_texture, &my_image_width, &my_image_height);

		Show = true;
	}


	void Stop()
	{
		// unload resource
        if (my_texture) {
            my_texture->Release();
        }

		Show = false;
	}


	void Install()
	{
		DKUtil::GUI::InitD3D(); 
        DKUtil::GUI::AddCallback(FUNC_INFO(SimpleWindow));

		INFO("GUI installed!"sv);
	}


	void Uninstall()
	{
		Stop();
		INFO("GUI uninstalled!"sv);
	}
}