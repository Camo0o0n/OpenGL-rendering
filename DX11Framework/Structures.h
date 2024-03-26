#pragma once

#include <DirectXMath.h>

using namespace DirectX;

struct ConstantBuffer
{
	XMMATRIX Projection;
	XMMATRIX View;
	XMMATRIX World;
	XMFLOAT4 DiffuseLight;
	XMFLOAT4 DiffuseMaterial;
	XMFLOAT3 LightDir;
	float Count;
	XMFLOAT4 ambientMaterial;
	XMFLOAT4 ambientLighting;
	XMFLOAT4 specularLight;
	XMFLOAT4 specularMaterial;
	XMFLOAT3 cameraPosition;
	float specPower;
	int hasTexture;
};

struct MeshData
{
	ID3D11Buffer* VertexBuffer;
	ID3D11Buffer* IndexBuffer;
	UINT VBStride;
	UINT VBOffset;
	UINT IndexCount;
};

struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 TexC;

	bool operator<(const SimpleVertex other) const
	{
		return memcmp((void*)this, (void*)&other, sizeof(SimpleVertex)) > 0;
	}
};