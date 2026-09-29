#include "Capture.h"
#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <iostream>
#include <vector>
#include <WICTextureLoader.h>

using Microsoft::WRL::ComPtr;
void Check(HRESULT hr) { if (FAILED(hr)) throw std::runtime_error(std::format("HRESULT 0x{:08X}", static_cast<unsigned>(hr))); }
int main() {
    try {
        Check(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;
        Check(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context));
        // A distinctive green rectangle at precisely the expected crop position,
        // surrounded by red. Decoding every pixel detects off-by-one/crop errors.
        constexpr UINT size = 1000;
        std::vector<uint32_t> pixels(size*size, 0xFF0000FF);
        for (UINT y=80; y<920; ++y) for (UINT x=540; x<990; ++x) pixels[y*size+x] = 0xFF00FF00;
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width=size; desc.Height=size; desc.MipLevels=1; desc.ArraySize=1;
        desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count=1;
        desc.Usage=D3D11_USAGE_DEFAULT;
        D3D11_SUBRESOURCE_DATA initial{pixels.data(), size*4, 0};
        ComPtr<ID3D11Texture2D> texture;
        Check(device->CreateTexture2D(&desc, &initial, &texture));
        const auto path = Gallery::SavePortraitTexture(texture.Get(), "capture-test-output");
        ComPtr<IWICImagingFactory> factory;
        Check(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)));
        ComPtr<IWICBitmapDecoder> decoder;
        Check(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder));
        ComPtr<IWICBitmapFrameDecode> frame;
        Check(decoder->GetFrame(0,&frame));
        UINT w{},h{}; Check(frame->GetSize(&w,&h));
        if(w!=450 || h!=840) throw std::runtime_error(std::format("Wrong crop dimensions {}x{}",w,h));
        ComPtr<IWICFormatConverter> converter;
        Check(factory->CreateFormatConverter(&converter));
        Check(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom));
        std::vector<uint32_t> actual(w*h);
        Check(converter->CopyPixels(nullptr,w*4,static_cast<UINT>(actual.size()*4),reinterpret_cast<BYTE*>(actual.data())));
        for(auto pixel:actual) if(pixel!=0xFF00FF00) throw std::runtime_error("Crop contains out-of-frame pixels or incorrect color channels");
        const auto smallPath=Gallery::SavePortraitTexture(texture.Get(),"capture-test-output",225);
        ComPtr<IWICBitmapDecoder> smallDecoder;
        Check(factory->CreateDecoderFromFilename(smallPath.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&smallDecoder));
        ComPtr<IWICBitmapFrameDecode> smallFrame; Check(smallDecoder->GetFrame(0,&smallFrame));
        Check(smallFrame->GetSize(&w,&h));
        if(w!=225 || h!=420) throw std::runtime_error("Scaled portrait size/aspect ratio incorrect");
        ComPtr<IWICFormatConverter> smallConverter; Check(factory->CreateFormatConverter(&smallConverter));
        Check(smallConverter->Initialize(smallFrame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom));
        actual.resize(w*h);
        Check(smallConverter->CopyPixels(nullptr,w*4,static_cast<UINT>(actual.size()*4),reinterpret_cast<BYTE*>(actual.data())));
        for(auto pixel:actual) if(pixel!=0xFF00FF00) throw std::runtime_error("Scaled portrait color/crop incorrect");
        // A tagged sRGB PNG must not be sampled through an sRGB-decoding view
        // by the gallery's display-referred ImGui path. Midtones expose this.
        std::fill(pixels.begin(),pixels.end(),0xFF808080);
        desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        ComPtr<ID3D11Texture2D> gray;
        Check(device->CreateTexture2D(&desc,&initial,&gray));
        const auto grayPath=Gallery::SavePortraitTexture(gray.Get(),"capture-test-output");
        ComPtr<ID3D11ShaderResourceView> automatic,display;
        Check(DirectX::CreateWICTextureFromFile(device.Get(),grayPath.c_str(),nullptr,&automatic));
        Gallery::CreatePortraitView(device.Get(),grayPath,display.GetAddressOf());
        D3D11_SHADER_RESOURCE_VIEW_DESC autoDesc{},displayDesc{};
        automatic->GetDesc(&autoDesc); display->GetDesc(&displayDesc);
        if(autoDesc.Format!=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB || displayDesc.Format!=DXGI_FORMAT_R8G8B8A8_UNORM) throw std::runtime_error("PNG display gamma regression");
        ComPtr<ID3D11Resource> displayResource; display->GetResource(&displayResource);
        ComPtr<ID3D11Texture2D> displayTexture; Check(displayResource.As(&displayTexture));
        D3D11_TEXTURE2D_DESC readDesc{}; displayTexture->GetDesc(&readDesc);
        readDesc.Usage=D3D11_USAGE_STAGING; readDesc.BindFlags=0; readDesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Texture2D> staging; Check(device->CreateTexture2D(&readDesc,nullptr,&staging));
        context->CopyResource(staging.Get(),displayTexture.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{}; Check(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped));
        const auto midtone=*static_cast<const uint32_t*>(mapped.pData);
        context->Unmap(staging.Get(),0);
        if(midtone!=0xFF808080) throw std::runtime_error("Portrait midtone values changed");
        bool rejected=false;
        try { (void)Gallery::SavePortraitTexture(nullptr,"capture-test-output"); }
        catch(const std::runtime_error&) { rejected=true; }
        if(!rejected) throw std::runtime_error("Null input was not rejected");
        std::cout << "PASS: PNG 450x840, all crop pixels correct, null input rejected\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
