#include <iostream>

#include "DX11Framework.h"
#include <string>
#include "DDSTextureLoader.h"

#include "Structures.h"
#include <array>

#include <codecvt>
#include <locale>

#include "JSON\json.hpp"
using json = nlohmann::json;
//#define RETURNFAIL(x) if(FAILED(x)) return x;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

HRESULT DX11Framework::Initialise(HINSTANCE hInstance, int nShowCmd)
{

    HRESULT hr = S_OK;

    hr = CreateWindowHandle(hInstance, nShowCmd);
    if (FAILED(hr)) return E_FAIL;

    hr = CreateD3DDevice();
    if (FAILED(hr)) return E_FAIL;

    hr = CreateSwapChainAndFrameBuffer();
    if (FAILED(hr)) return E_FAIL;

    hr = InitShadersAndInputLayout();
    if (FAILED(hr)) return E_FAIL;

    hr = InitVertexIndexBuffers();
    if (FAILED(hr)) return E_FAIL;

    hr = InitPipelineVariables();
    if (FAILED(hr)) return E_FAIL;

    hr = InitRunTimeData();
    if (FAILED(hr)) return E_FAIL;

    return hr;
}

HRESULT DX11Framework::CreateWindowHandle(HINSTANCE hInstance, int nCmdShow)
{
    const wchar_t* windowName  = L"DX11Framework";

    WNDCLASSW wndClass;
    wndClass.style = 0;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = 0;
    wndClass.hIcon = 0;
    wndClass.hCursor = 0;
    wndClass.hbrBackground = 0;
    wndClass.lpszMenuName = 0;
    wndClass.lpszClassName = windowName;

    RegisterClassW(&wndClass);

    _windowHandle = CreateWindowExW(0, windowName, windowName, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 
        _WindowWidth, _WindowHeight, nullptr, nullptr, hInstance, nullptr);

    return S_OK;
}

HRESULT DX11Framework::CreateD3DDevice()
{
    HRESULT hr = S_OK;

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
    };

    ID3D11Device* baseDevice;
    ID3D11DeviceContext* baseDeviceContext;

    DWORD createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT | createDeviceFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &baseDevice, nullptr, &baseDeviceContext);
    if (FAILED(hr)) return hr;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    hr = baseDevice->QueryInterface(__uuidof(ID3D11Device), reinterpret_cast<void**>(&_device));
    hr = baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext), reinterpret_cast<void**>(&_immediateContext));

    baseDevice->Release();
    baseDeviceContext->Release();

    ///////////////////////////////////////////////////////////////////////////////////////////////

    hr = _device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&_dxgiDevice));
    if (FAILED(hr)) return hr;

    IDXGIAdapter* dxgiAdapter;
    hr = _dxgiDevice->GetAdapter(&dxgiAdapter);
    hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&_dxgiFactory));
    dxgiAdapter->Release();

    return S_OK;
}

HRESULT DX11Framework::CreateSwapChainAndFrameBuffer()
{
    HRESULT hr = S_OK;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width = 0; // Defer to WindowWidth
    swapChainDesc.Height = 0; // Defer to WindowHeight
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //FLIP* modes don't support sRGB backbuffer
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;

    hr = _dxgiFactory->CreateSwapChainForHwnd(_device, _windowHandle, &swapChainDesc, nullptr, nullptr, &_swapChain);
    if (FAILED(hr)) return hr;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3D11Texture2D* frameBuffer = nullptr;

    hr = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&frameBuffer));
    if (FAILED(hr)) return hr;

    D3D11_RENDER_TARGET_VIEW_DESC framebufferDesc = {};
    framebufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //sRGB render target enables hardware gamma correction
    framebufferDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    hr = _device->CreateRenderTargetView(frameBuffer, &framebufferDesc, &_frameBufferView);

    D3D11_TEXTURE2D_DESC depthBufferDesc = {};
    frameBuffer->GetDesc(&depthBufferDesc);

    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    _device->CreateTexture2D(&depthBufferDesc, nullptr, &_depthStencilBuffer);
    _device->CreateDepthStencilView(_depthStencilBuffer, nullptr, &_depthStencilView);

    frameBuffer->Release(); // Release after depth buffer is created


    return hr;
}

HRESULT DX11Framework::InitShadersAndInputLayout()
{
    HRESULT hr = S_OK;
    ID3DBlob* errorBlob;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif
    
    ID3DBlob* vsBlob;

    hr =  D3DCompileFromFile(L"SimpleShaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS_main", "vs_5_0", dwShaderFlags, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        MessageBoxA(_windowHandle, (char*)errorBlob->GetBufferPointer(), nullptr, ERROR);
        errorBlob->Release();
        return hr;
    }

    hr = _device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &_vertexShader);

    if (FAILED(hr)) return hr;

    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 }
    };

    hr = _device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &_inputLayout);
    if (FAILED(hr)) return hr;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3DBlob* psBlob;

    hr = D3DCompileFromFile(L"SimpleShaders.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS_main", "ps_5_0", dwShaderFlags, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        MessageBoxA(_windowHandle, (char*)errorBlob->GetBufferPointer(), nullptr, ERROR);
        errorBlob->Release();
        return hr;
    }

    hr = _device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &_pixelShader);

    vsBlob->Release();
    psBlob->Release();

    return hr;
}

HRESULT DX11Framework::InitVertexIndexBuffers()
{
    HRESULT hr = S_OK;

    
    //_myObjMeshData = OBJLoader::Load((char*)"Test models\\Airplane\\Hercules.obj", _device);

    //Cube Stuff
    SimpleVertex VertexData[] = 
    {
        //Position                         //Normal                         //TEXCOORD
        { XMFLOAT3(-1.00f,  1.00f, -1.0f), XMFLOAT3(-1.00f,  1.00f, -1.0f), XMFLOAT2(0.0f, 0.0f)},
        { XMFLOAT3(1.00f,  1.00f, -1.0f),  XMFLOAT3(1.00f,  1.00f, -1.0f), XMFLOAT2(1.0f, 0.0f)},
        { XMFLOAT3(-1.00f, -1.00f, -1.0f), XMFLOAT3(-1.00f, -1.00f, -1.0f), XMFLOAT2(0.0f, 1.0f)},
        { XMFLOAT3(1.00f, -1.00f, -1.0f),  XMFLOAT3(1.00f, -1.00f, -1.0f), XMFLOAT2(1.0f, 1.0f)},

        { XMFLOAT3(-1.00f,  1.00f, 1.0f), XMFLOAT3(-1.00f,  1.00f, 1.0f), XMFLOAT2(1.0f, 0.0f)},
        { XMFLOAT3(1.00f,  1.00f, 1.0f),  XMFLOAT3(1.00f,  1.00f, 1.0f), XMFLOAT2(0.0f, 0.0f)},
        { XMFLOAT3(-1.00f, -1.00f, 1.0f), XMFLOAT3(-1.00f, -1.00f, 1.0f), XMFLOAT2(1.0f, 1.0f)},
        { XMFLOAT3(1.00f, -1.00f, 1.0f),  XMFLOAT3(1.00f, -1.00f, 1.0f), XMFLOAT2(0.0f, 1.0f)},
    };

    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.ByteWidth = sizeof(VertexData);
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData = { VertexData };

    hr = _device->CreateBuffer(&vertexBufferDesc, &vertexData, &_vertexBuffer);
    if (FAILED(hr)) return hr;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    WORD IndexData[] =
    {
        //Indices
        0, 1, 2,
        2, 1, 3,

        6, 5, 4,
        7, 5, 6,

        4, 1, 0,
        5, 1, 4,

        2, 7, 6,
        3, 7, 2,

        4, 2, 6,
        4, 0, 2,

        1, 7, 3,
        5, 7, 1,
    };

    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.ByteWidth = sizeof(IndexData);
    indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexData = { IndexData };

    hr = _device->CreateBuffer(&indexBufferDesc, &indexData, &_indexBuffer);
    if (FAILED(hr)) return hr;

    // Pyramid Stuff

    SimpleVertex PyramidVertexData[] =
    {
        { XMFLOAT3(0.0f,  1.0f, 0.0f), XMFLOAT3(0.0f,  1.0f, 0.0f)},

        { XMFLOAT3(1.00f,  -1.0, -1.0f),  XMFLOAT3(1.00f,  -1.0, -1.0f)},
        { XMFLOAT3(-1.00f, -1.0f, -1.0f), XMFLOAT3(-1.00f, -1.0f, -1.0f)},
        { XMFLOAT3(1.00f, -1.0f, 1.0f),  XMFLOAT3(1.00f, -1.0f, 1.0f)},
        { XMFLOAT3(-1.00f, -1.0f, 1.0f),  XMFLOAT3(-1.00f, -1.0f, 1.0f)}
    };

    D3D11_BUFFER_DESC pyramidVertexBufferDesc = {};
    pyramidVertexBufferDesc.ByteWidth = sizeof(PyramidVertexData);
    pyramidVertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    pyramidVertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA pyramidVertexData = { PyramidVertexData };

    hr = _device->CreateBuffer(&pyramidVertexBufferDesc, &pyramidVertexData, &_pyramidVertexBuffer);
    if (FAILED(hr)) return hr;

    WORD PyramidIndexData[] =
    {
        0, 1, 2,
        0, 2, 4,
        0, 4, 3,
        0, 3, 1,

        1, 3, 2,
        4, 3, 2
    };

    D3D11_BUFFER_DESC pyramidIndexBufferDesc = {};
    pyramidIndexBufferDesc.ByteWidth = sizeof(PyramidIndexData);
    pyramidIndexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    pyramidIndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA pyramidIndexData = { PyramidIndexData };

    hr = _device->CreateBuffer(&pyramidIndexBufferDesc, &pyramidIndexData, &_pyramidIndexBuffer);
    if (FAILED(hr)) return hr;

    // Line stuff

    SimpleVertex LineList[] =
    {
        {XMFLOAT3(0,2,0), XMFLOAT3(1, 1, 1)},
        {XMFLOAT3(0,6,0), XMFLOAT3(1, 1, 1)}
    };

    D3D11_BUFFER_DESC lineVertexBufferDesc = {};
    lineVertexBufferDesc.ByteWidth = sizeof(LineList);
    lineVertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    lineVertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA lineVertexData = { LineList };

    hr = _device->CreateBuffer(&lineVertexBufferDesc, &lineVertexData, &_lineVertexBuffer);
    if (FAILED(hr)) return hr;

    return S_OK;
}

HRESULT DX11Framework::InitPipelineVariables()
{
    HRESULT hr = S_OK;

    //Input Assembler


    _immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _immediateContext->IASetInputLayout(_inputLayout);

    //Texture Sampler
    D3D11_SAMPLER_DESC bilinearSamplerdesc = {};
    bilinearSamplerdesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    bilinearSamplerdesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    bilinearSamplerdesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    bilinearSamplerdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    bilinearSamplerdesc.MaxLOD = 1;
    bilinearSamplerdesc.MinLOD = 0;

    hr = _device->CreateSamplerState(&bilinearSamplerdesc, &_bilinearSamplerState);
    if (FAILED(hr)) return hr;

    _immediateContext->PSSetSamplers(0, 1, &_bilinearSamplerState);


    //Rasterizer
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;

    hr = _device->CreateRasterizerState(&rasterizerDesc, &_fillState);
    if (FAILED(hr)) return hr;

    _immediateContext->RSSetState(_fillState);

    D3D11_RASTERIZER_DESC wireframeDesc = {};
    wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
    wireframeDesc.CullMode = D3D11_CULL_BACK;

    hr = _device->CreateRasterizerState(&wireframeDesc, &_wireframeState);
    if (FAILED(hr)) return hr;

    //Viewport Values
    _viewport = { 0.0f, 0.0f, (float)_WindowWidth, (float)_WindowHeight, 0.0f, 1.0f };
    _immediateContext->RSSetViewports(1, &_viewport);

    //Constant Buffer
    D3D11_BUFFER_DESC constantBufferDesc = {};
    constantBufferDesc.ByteWidth = sizeof(ConstantBuffer);
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = _device->CreateBuffer(&constantBufferDesc, nullptr, &_constantBuffer);
    if (FAILED(hr)) { return hr; }

    _immediateContext->VSSetConstantBuffers(0, 1, &_constantBuffer);
    _immediateContext->PSSetConstantBuffers(0, 1, &_constantBuffer);

    return S_OK;
}

HRESULT DX11Framework::InitRunTimeData()
{   
    //Camera
    BaseCamera Camera1(XMFLOAT3(0, 0, -12.0f), XMFLOAT3(0, 0, 0), XMFLOAT3(0, 1, 0), _viewport.Width, _viewport.Height, 0.5f, 10000.0f);
    BaseCamera Camera2(XMFLOAT3(0, 0, 12.0f), XMFLOAT3(0, 0, 0), XMFLOAT3(0, 1, 0), _viewport.Width, _viewport.Height, 0.5f, 10000.0f);
    BaseCamera Camera3(XMFLOAT3(0, 12.0f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(1, 0, 0), _viewport.Width, _viewport.Height, 0.5f, 10000.0f);
    BaseCamera Camera4(XMFLOAT3(0, -12.0f, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(-1, 0, 0), _viewport.Width, _viewport.Height, 0.5f, 10000.0f);
    BaseCamera Camera5(XMFLOAT3(12.0f, 0, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(0, 1, 0), _viewport.Width, _viewport.Height, 0.5, 10000.0f);


    cameraList.push_back(Camera1);
    cameraList.push_back(Camera2);
    cameraList.push_back(Camera3);
    cameraList.push_back(Camera4);
    cameraList.push_back(Camera5);
    
    XMFLOAT3 tempCamera = XMFLOAT3(0, 0, -6.0f);
    XMVECTOR tempVar1 = XMLoadFloat3(&tempCamera);
    tempCamera = XMFLOAT3(0, 0, 1);
    XMVECTOR tempVar2 = XMLoadFloat3(&tempCamera);
    tempCamera = XMFLOAT3(0, 1, 0);
    XMVECTOR tempVar3 = XMLoadFloat3(&tempCamera);

    _lookCamera = LookCamera(tempVar1, tempVar2, tempVar3, _viewport.Width, _viewport.Height, 0.5f, 10000.0f);
#

    CameraUpdate(6);
    // Reading all the data from the JSON
    json jFile;
    std::string tempVar;
    std::ifstream fileOpen("JSON/fileData.json");

    jFile = json::parse(fileOpen);

    std::string v = jFile["version"].get<std::string>();

    json& objects = jFile["Gameobjects"]; //← gets an array
    int size = objects.size();
    for (unsigned int i = 0; i < size; i++)
    {
        GameObject g;
        json& objectDesc = objects.at(i);
        tempVar = objectDesc["MeshLocation"]; // ← gets a string
        g.SetRotation(objectDesc["RotationY"]);
        g.SetScale(objectDesc["Scale"]);
        g.SetMeshData(OBJLoader::Load((char*)tempVar.c_str(), _device));
        g.SetHasTexture(objectDesc["HasTexture"]);
        if (g.GetHasTexture() == 1)
        {
            ID3D11ShaderResourceView* temp;

            tempVar = objectDesc["TextureLocation"];
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::wstring wideTempVar = converter.from_bytes(tempVar);
            /*const char* charPointer = tempVar.c_str();*/
            HRESULT hr = CreateDDSTextureFromFile(_device, wideTempVar.c_str(), nullptr, &temp);
            //_immediateContext->PSSetShaderResources(0, 1, &_Texture);
            g.SetShaderResource(temp);
        }

        //make a variable of probably ID3D11ShaderResourceView and then pass the pointer to that address where I will pass that data into the game object
        g.SetWorldVector(XMFLOAT3(objectDesc["StartPosX"], objectDesc["StartPosY"], objectDesc["StartPosZ"]));

        gameobjects.push_back(g);
    }

    HRESULT hr = CreateDDSTextureFromFile(_device, L"Textures\\Crate_COLOR.dds", nullptr, &_crateTexture);

    _immediateContext->PSSetShaderResources(0, 1, &_crateTexture);

    _diffuseLight = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
    _diffuseMaterial = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    _lightDir = XMFLOAT3(0, 0.0f, -1.0f);
    
    _cbData.DiffuseLight = _diffuseLight;
    _cbData.DiffuseMaterial = _diffuseMaterial;
    _cbData.LightDir = _lightDir;
    _cbData.ambientMaterial = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    _cbData.ambientLighting = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);    
    _cbData.specularMaterial = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    _cbData.specularLight = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    _cbData.specPower = 10.0f;

    return S_OK;
}

DX11Framework::~DX11Framework()
{
    if(_immediateContext)_immediateContext->Release();
    if(_device)_device->Release();
    if(_dxgiDevice)_dxgiDevice->Release();
    if(_dxgiFactory)_dxgiFactory->Release();
    if(_frameBufferView)_frameBufferView->Release();
    if(_swapChain)_swapChain->Release();

    if(_fillState)_fillState->Release();
    if (_wireframeState)_wireframeState->Release();
    if(_vertexShader)_vertexShader->Release();
    if(_inputLayout)_inputLayout->Release();
    if(_pixelShader)_pixelShader->Release();
    if(_constantBuffer)_constantBuffer->Release();
    if(_vertexBuffer)_vertexBuffer->Release();
    if(_indexBuffer)_indexBuffer->Release();
    
    if (_pyramidVertexBuffer)_pyramidVertexBuffer->Release();
    if (_pyramidIndexBuffer)_pyramidIndexBuffer->Release();
    
    if (_lineVertexBuffer)_lineVertexBuffer->Release();

    if(_depthStencilBuffer)_depthStencilBuffer->Release();
    if(_depthStencilView)_depthStencilView->Release();
    /*if (_diffuseLight)_diffuseLight->Release();*/
}


void DX11Framework::Update()
{
    //Static initializes this value only once    
    static ULONGLONG frameStart = GetTickCount64();

    ULONGLONG frameNow = GetTickCount64();
    float deltaTime = (frameNow - frameStart) / 1000.0f;
    frameStart = frameNow;

   /* if (GetAsyncKeyState(VK_F2) & 0x0001)
    {
        _immediateContext->RSSetState(_wireframeState);
    }
    else if (GetAsyncKeyState(VK_F1) & 0x0001)
    {
        _immediateContext->RSSetState(_fillState);
    }*/

    static float simpleCount = 0.0f;
    simpleCount += deltaTime;
    _cbData.Count = simpleCount;

    if (GetAsyncKeyState(0x31) & 0x0001)
    {
        CameraUpdate(0);
    }
    else if (GetAsyncKeyState(0x32) & 0x0001)
    {
        CameraUpdate(1);
    }
    else if (GetAsyncKeyState(0x33) & 0x0001)
    {
        CameraUpdate(2);
    }
    else if (GetAsyncKeyState(0x34) & 0x0001)
    {
        CameraUpdate(3);
    }
    else if (GetAsyncKeyState(0x35) & 0x0001)
    {
        CameraUpdate(4);
    }
    else if (GetAsyncKeyState(0x36) & 0x0001)
    {
        CameraUpdate(5);
    }
    else if (GetAsyncKeyState(0x37) & 0x0001)
    {
        CameraUpdate(6);
    }

    if (GetKeyState(0x41) < 0) // A go right
    {
        _lookCamera.UpdateEye(-0.001, 0, 0);
        CameraUpdate(6);
    }
    if (GetKeyState(0x44) < 0) // D go left
    {
        _lookCamera.UpdateEye(0.001, 0, 0);
        CameraUpdate(6);
    }
    if (GetKeyState(0x57) < 0) // W go forward
    {
        _lookCamera.UpdateEye(0, 0, 0.001);
        CameraUpdate(6);
    }
    if (GetKeyState(0x53) < 0) // S go back
    {
        _lookCamera.UpdateEye(0, 0, -0.001);
        CameraUpdate(6);
    }
    if (GetKeyState(0x45) < 0) // E Go up
    {
        _lookCamera.UpdateEye(0, 0.001, 0);
        CameraUpdate(6);
    }
    if (GetKeyState(0x51) < 0) // Q Go down
    {
        _lookCamera.UpdateEye(0, -0.001, 0);
        CameraUpdate(6);
    }
    if (GetKeyState(0x4A) < 0) // J pan left
    {
        _lookCamera.UpdateDirection(-0.0002, 'x');
        CameraUpdate(6);
    }
    if (GetKeyState(0x4C) < 0) // L pan right
    {
        _lookCamera.UpdateDirection(0.0002, 'x');
        CameraUpdate(6);
    }
    if (GetKeyState(0x49) < 0) // I pan up
    {
        _lookCamera.UpdateDirection(-0.0002, 'y');
        CameraUpdate(6);
    }
    if (GetKeyState(0x4B) < 0) // K pan down
    {
        _lookCamera.UpdateDirection(0.0002, 'y');
        CameraUpdate(6);
    }
    //if (GetKeyState(0x55) < 0) // U
    //{
    //    _lookCamera.UpdateDirection(0.0002, 'z');
    //    CameraUpdate(6);
    //}
    //if (GetKeyState(0x4F) < 0) // O
    //{
    //    _lookCamera.UpdateDirection(-0.0002, 'z');
    //    CameraUpdate(6);
    //}
    XMStoreFloat4x4(&_World, XMMatrixIdentity() * XMMatrixRotationY(simpleCount * 0.037f) * XMMatrixRotationX(simpleCount));
    XMStoreFloat4x4(&_World2, XMMatrixIdentity() * XMMatrixScaling(0.3f, 0.3f, 0.3f) * XMMatrixTranslation(2, 0, 2) * XMMatrixRotationY(simpleCount));
    XMStoreFloat4x4(&_World3, XMMatrixIdentity() * XMMatrixRotationY(simpleCount*2) * XMMatrixScaling(0.2f, 0.2f, 0.2f) * XMMatrixTranslation(4, 0, 2));
    //XMStoreFloat4x4(&_GameObject, XMMatrixIdentity() * XMMatrixRotationY(simpleCount * 2) * XMMatrixScaling(0.2f, 0.2f, 0.2f) * XMMatrixTranslation(4, 0, 2));
    XMStoreFloat4x4(&_WorldLine, XMMatrixIdentity());

}

void DX11Framework::Draw()
{
    _immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //Present unbinds render target, so rebind and clear at start of each frame
    float backgroundColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
    _immediateContext->OMSetRenderTargets(1, &_frameBufferView, _depthStencilView);
    _immediateContext->ClearRenderTargetView(_frameBufferView, backgroundColor);
    _immediateContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0.0f);
   
    //Store this frames data in constant buffer struct
    _cbData.World = XMMatrixTranspose(XMLoadFloat4x4(&_World));

    //Write constant buffer data onto GPU
    D3D11_MAPPED_SUBRESOURCE mappedSubresource;
    _immediateContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
    memcpy(mappedSubresource.pData, &_cbData, sizeof(_cbData));
    _immediateContext->Unmap(_constantBuffer, 0);

    //Set object variables and draw
    UINT stride = {sizeof(SimpleVertex)};
    UINT offset =  0 ;
    _immediateContext->IASetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
    _immediateContext->IASetIndexBuffer(_indexBuffer, DXGI_FORMAT_R16_UINT, 0);

    _immediateContext->VSSetShader(_vertexShader, nullptr, 0);
    _immediateContext->PSSetShader(_pixelShader, nullptr, 0);

    _immediateContext->PSSetShaderResources(0,1, &_crateTexture);


    _immediateContext->DrawIndexed(36, 0, 0);

    //Remap to update Earth Data
    _immediateContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
    //Load new world information
    _cbData.World = XMMatrixTranspose(XMLoadFloat4x4(&_World2));

    memcpy(mappedSubresource.pData, &_cbData, sizeof(_cbData));
    _immediateContext->Unmap(_constantBuffer, 0);

    _immediateContext->DrawIndexed(36, 0, 0);

    //Remap to update Moon Data
    _immediateContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
    //Load new world information
    _cbData.World = XMMatrixTranspose(XMLoadFloat4x4(&_World3));

    memcpy(mappedSubresource.pData, &_cbData, sizeof(_cbData));
    _immediateContext->Unmap(_constantBuffer, 0);

    _immediateContext->IASetVertexBuffers(0, 1, &_pyramidVertexBuffer, &stride, &offset);
    _immediateContext->IASetIndexBuffer(_pyramidIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    _immediateContext->DrawIndexed(18, 0, 0);

    _immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    _immediateContext->IASetVertexBuffers(0, 1, &_lineVertexBuffer, &stride, &offset);
    _immediateContext->Draw(2, 0);

    
        
    _immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    for (int i= 0; i < gameobjects.size(); i++)
    {
        //Remap to update Mesh Data
        _immediateContext->Map(_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
        
        XMMATRIX goworld = XMMatrixIdentity() * XMMatrixRotationY(gameobjects[i].GetRotation()) * XMMatrixScaling(gameobjects[i].GetScale(), gameobjects[i].GetScale(), gameobjects[i].GetScale()) * XMMatrixTranslation(gameobjects[i].GetWorldVector()->x, gameobjects[i].GetWorldVector()->y, gameobjects[i].GetWorldVector()->z);

        //Load new Mesh information
        _cbData.World = XMMatrixTranspose(goworld);
        _cbData.hasTexture = gameobjects[i].GetHasTexture();

        memcpy(mappedSubresource.pData, &_cbData, sizeof(_cbData));
        _immediateContext->Unmap(_constantBuffer, 0);

        ID3D11Buffer* ppBuffer = gameobjects[i].GetMeshData().VertexBuffer;
        MeshData tempData = gameobjects[i].GetMeshData();
        _immediateContext->IASetVertexBuffers(0, 1, &ppBuffer, &stride, &offset);
        _immediateContext->IASetIndexBuffer(tempData.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);

        if (_cbData.hasTexture == 1)
        {
            _immediateContext->PSSetShaderResources(0, 1, gameobjects[i].GetShaderResource());
        }

        ID3D11ShaderResourceView* tmp = nullptr;

        _immediateContext->DrawIndexed(gameobjects[i].GetMeshData().IndexCount, 0, 0);
        _immediateContext->PSSetShaderResources(0, 1, &tmp);
    }

    //Present Backbuffer to screen
    _swapChain->Present(0, 0);
}

void DX11Framework::CameraUpdate(int listPosition)
{        

    if (listPosition != 6)
    {
        XMFLOAT4X4 tempView;
        _cbData.cameraPosition = cameraList[listPosition].GetEye();
        tempView = cameraList[listPosition].GetView();
        _cbData.View = XMMatrixTranspose(XMLoadFloat4x4(&tempView));
        tempView = cameraList[listPosition].GetProj();
        _cbData.Projection = XMMatrixTranspose(XMLoadFloat4x4(&tempView));
    }
    else
    {

        XMFLOAT4X4 tempView;
        XMFLOAT3 tempCamPosition;
        XMStoreFloat3(&tempCamPosition, _lookCamera.GetEye());
        _cbData.cameraPosition = tempCamPosition;
        tempView = _lookCamera.GetView();
        _cbData.View = XMMatrixTranspose(XMLoadFloat4x4(&tempView));
        tempView = _lookCamera.GetProj();
        _cbData.Projection = XMMatrixTranspose(XMLoadFloat4x4(&tempView));
    }

}
