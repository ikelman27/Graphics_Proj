#pragma once

#include "DXCore.h"
#include "SimpleShader.h"
#include <DirectXMath.h>
#include "Mesh.h"
#include "Material.h"
#include "Lights.h"
#include "Collider.h"

class Entity
{
public:
	Entity();
	Entity(Mesh* m, Material* mat, float rad);
	Entity(Mesh* m, Material* mat, DirectX::XMFLOAT2 s);
	~Entity();
	
	DirectX::XMFLOAT4X4 GetWorldMatrix();
	void SetTranslation(DirectX::XMFLOAT3 tran);
	void SetScale(DirectX::XMFLOAT3 sc);
	void SetRotation(DirectX::XMFLOAT4 rot);
	void RotateAroundAxis(DirectX::XMFLOAT3 axis, float angle);
	void Move(DirectX::XMFLOAT3 amount);
	void MoveForward(float amount);
	void SetDirtyMatrix();
	void ComputeWorldMatrix();
	
	ID3D11Buffer* GetVertexBuffer();
	ID3D11Buffer* GetIndexBuffer();
	int GetIndexCount();
	Material* GetMaterial();
	Collider* GetCollider();
	void PrepareMaterial(DirectX::XMFLOAT4X4 view, DirectX::XMFLOAT4X4 proj, SpotLight* light, DirectionalLight* light2);
private:
	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 scale;
	DirectX::XMFLOAT4 rotation;
	Mesh* mesh;
	bool isDirty;
	Material* material;
	Collider* collider;
};

