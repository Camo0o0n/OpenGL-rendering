#pragma once

#include <windows.h>
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "OBJLoader.h"
#include "Structures.h"


using namespace DirectX;
//using Microsoft::WRL::ComPtr

class GameObject
{
private:
	ID3D11ShaderResourceView* texture = nullptr;
	MeshData meshData;
	DirectX::XMFLOAT3 world;
	int hasTexture;
	float rotation;
	float scale;

public:

	GameObject() = default;

	void SetShaderResource(ID3D11ShaderResourceView* in) { texture = in; }
	void SetMeshData(MeshData in) { meshData = in; }
	void SetWorldVector(XMFLOAT3 in) { world = in; }
	void SetHasTexture(int in) { hasTexture = in; }
	void SetRotation(float in) { rotation = in; }
	void SetScale(float in) { scale = in; }

	ID3D11ShaderResourceView** GetShaderResource() { return &texture; }
	MeshData& GetMeshData() { return meshData; }
	XMFLOAT3* GetWorldVector() { return &world; }
	int GetHasTexture() { return hasTexture; }
	float GetRotation() { return rotation; }
	float GetScale() { return scale; }
};

class BaseCamera
{
private:

	XMFLOAT3 _eye;
	XMFLOAT3 _at;
	XMFLOAT3 _up;

	float _windowWidth;
	float _windowHeight;
	float _nearDepth;
	float _farDepth;

	XMFLOAT4X4 _view;
	XMFLOAT4X4 _projection;

public:

	BaseCamera(XMFLOAT3 eye, XMFLOAT3 at, XMFLOAT3 up, float windowWidth, float windowHeight, float nearDepth, float farDepth);
	~BaseCamera();

	void Update(float deltaTime);

	XMFLOAT3 GetEye() { return _eye; }
	void SetEye(XMFLOAT3 eye) { _eye = eye; }

	XMFLOAT3 GetAt() { return _at; }
	void SetAt(XMFLOAT3 at) { _at = at; }

	XMFLOAT3 GetUp() { return _up; }
	void SetUp(XMFLOAT3 up) { _up = up; }

	XMFLOAT4X4 GetView() { return _view; }
	void SetView(XMFLOAT4X4 view) { _view = view; }
	
	XMFLOAT4X4 GetProj() { return _projection; }
	void SetProj(XMFLOAT4X4 proj) { _projection = proj; }



};
inline BaseCamera::BaseCamera(XMFLOAT3 eye, XMFLOAT3 at, XMFLOAT3 up, float windowWidth, float windowHeight, float nearDepth, float farDepth)
{
	_eye = eye;
	_at = at;
	_up = up;
	_windowWidth = windowWidth;
	_windowHeight = windowHeight;
	_nearDepth = nearDepth;
	_farDepth = farDepth;

	XMStoreFloat4x4(&_view, XMMatrixLookAtLH(XMLoadFloat3(&_eye), XMLoadFloat3(&_at), XMLoadFloat3(&_up)));
	
	XMStoreFloat4x4(&_projection, XMMatrixPerspectiveFovLH(XMConvertToRadians(90), _windowWidth/_windowHeight, nearDepth, farDepth));
}

inline BaseCamera::~BaseCamera()
{
}


class LookCamera
{
private:

	XMVECTOR _eye;
	XMVECTOR _direction;
	XMVECTOR _up;

	float _windowWidth;
	float _windowHeight;
	float _nearDepth;
	float _farDepth;

	XMFLOAT4X4 _view;
	XMFLOAT4X4 _projection;

	float xRot = 0;
	float yRot = 0;
	float zRot = 0;

	XMMATRIX rotationY = XMMatrixRotationX(yRot);
	XMMATRIX rotationX = XMMatrixRotationY(xRot);

public:

	LookCamera(XMVECTOR eye, XMVECTOR direction, XMVECTOR up, float windowWidth, float windowHeight, float nearDepth, float farDepth);
	LookCamera();

	void UpdateEye(float x, float y, float z);
	void UpdateDirection(float angle, char axis);

	XMVECTOR GetEye() { return _eye; }
	void SetEye(XMVECTOR eye) { _eye = eye; }

	XMVECTOR GetDirection() { return _direction; }
	void SetDirection(XMVECTOR direction) { _direction = direction; }

	XMVECTOR GetUp() { return _up; }
	void SetUp(XMVECTOR up) { _up = up; }

	XMFLOAT4X4 GetView() { return _view; }
	void SetView(XMFLOAT4X4 view) { _view = view; }

	XMFLOAT4X4 GetProj() { return _projection; }
	void SetProj(XMFLOAT4X4 proj) { _projection = proj; }
};

inline LookCamera::LookCamera()
{
}

inline void LookCamera::UpdateEye(float x, float y, float z)
{
	XMFLOAT3 position;
	XMFLOAT3 tempX;
	XMFLOAT3 tempY;
	XMStoreFloat3(&position, _eye);
	position.x += x;
	position.y += y;
	position.z += z;

	_eye = XMLoadFloat3(&position);

	XMStoreFloat4x4(&_view, XMMatrixLookToLH(_eye, _direction, _up));
	XMStoreFloat4x4(&_projection, XMMatrixPerspectiveFovLH(XMConvertToRadians(90), _windowWidth / _windowHeight, _nearDepth, _farDepth));
}
inline void LookCamera::UpdateDirection(float angle, char axis)
{
	if (axis == 'y')
	{
		yRot += angle;
		rotationY = XMMatrixRotationX(yRot);
		_direction = rotationY.r[2] + rotationX.r[2];
	}
	else if (axis == 'x')
	{
		xRot += angle;
		rotationX = XMMatrixRotationY(xRot);
		_direction = rotationX.r[2] + rotationY.r[2];
	}
	//else if (axis == 'z')
	//{
	//	zRot += angle;
	//	rotationZ = XMMatrixRotationZ(zRot);
	//	rotationX = XMMatrixRotationY(xRot);
	//	rotationY = XMMatrixRotationX(yRot);
	//	_direction = rotationZ.r[1] + rotationX.r[2] + rotationY.r[2];
	//}

	XMStoreFloat4x4(&_view, XMMatrixLookToLH(_eye, _direction, _up));
	XMStoreFloat4x4(&_projection, XMMatrixPerspectiveFovLH(XMConvertToRadians(90), _windowWidth / _windowHeight, _nearDepth, _farDepth));
}
;

inline LookCamera::LookCamera(XMVECTOR eye, XMVECTOR direction, XMVECTOR up, float windowWidth, float windowHeight, float nearDepth, float farDepth)
{
	_eye = eye;
	_direction = direction;
	_up = up;
	_windowWidth = windowWidth;
	_windowHeight = windowHeight;
	_nearDepth = nearDepth;
	_farDepth = farDepth;

	XMStoreFloat4x4(&_view, XMMatrixLookToLH(_eye, _direction, _up));
	XMStoreFloat4x4(&_projection, XMMatrixPerspectiveFovLH(XMConvertToRadians(90), _windowWidth / _windowHeight, nearDepth, farDepth));
}


class DX11Framework
{
	int _WindowWidth = 1280;
	int _WindowHeight = 768;

	std::vector<GameObject> gameobjects;
	std::vector<BaseCamera> cameraList;

	LookCamera _lookCamera;


	ID3D11DeviceContext* _immediateContext = nullptr;
	ID3D11Device* _device;
	IDXGIDevice* _dxgiDevice = nullptr;
	IDXGIFactory2* _dxgiFactory = nullptr;
	ID3D11RenderTargetView* _frameBufferView = nullptr;
	IDXGISwapChain1* _swapChain;
	D3D11_VIEWPORT _viewport;

	ID3D11RasterizerState* _fillState;
	ID3D11RasterizerState* _wireframeState;
	ID3D11VertexShader* _vertexShader;
	ID3D11InputLayout* _inputLayout;
	ID3D11PixelShader* _pixelShader;
	ID3D11Buffer* _constantBuffer;
	ID3D11Buffer* _vertexBuffer;
	ID3D11Buffer* _indexBuffer;
	ID3D11Buffer* _pyramidVertexBuffer;
	ID3D11Buffer* _pyramidIndexBuffer;
	ID3D11Buffer* _lineVertexBuffer;
	
	ID3D11Texture2D* _depthStencilBuffer;
	ID3D11DepthStencilView* _depthStencilView;

	ID3D11SamplerState* _bilinearSamplerState;
	ID3D11ShaderResourceView* _crateTexture;
	ID3D11ShaderResourceView* _Texture;

	HWND _windowHandle;

	XMFLOAT4X4 _World;
	XMFLOAT4X4 _World2;
	XMFLOAT4X4 _World3;
	XMFLOAT4X4 _WorldLine;
	XMFLOAT4X4 _GameObject;
	XMFLOAT4X4 _View;
	XMFLOAT4X4 _Projection;

	ConstantBuffer _cbData;

	XMFLOAT4 _diffuseLight;
	XMFLOAT4 _diffuseMaterial;
	XMFLOAT3 _lightDir;

public:
	HRESULT Initialise(HINSTANCE hInstance, int nCmdShow);
	HRESULT CreateWindowHandle(HINSTANCE hInstance, int nCmdShow);
	HRESULT CreateD3DDevice();
	HRESULT CreateSwapChainAndFrameBuffer();
	HRESULT InitShadersAndInputLayout();
	HRESULT InitVertexIndexBuffers();
	HRESULT InitPipelineVariables();
	HRESULT InitRunTimeData();
	~DX11Framework();
	void Update();
	void Draw();
	void CameraUpdate(int listPosition);
};
