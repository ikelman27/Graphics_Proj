#include "Entity.h"
using namespace DirectX;

Entity::Entity()
{
	// set the initial world matrix
	XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());

	// set initial scale and position
	position = XMFLOAT3(0, 0, 0);
	scale = XMFLOAT3(1, 1, 1);

	// set the initial rotation quaternion
	XMStoreFloat4(&rotation, XMQuaternionIdentity());

	mesh = new Mesh();

	isDirty = true;
}

Entity::Entity(Mesh* m, Material* mat)
{
	// set the initial world matrix
	XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());

	// set initial scale and position
	position = XMFLOAT3(0, 0, 0);
	scale = XMFLOAT3(1, 1, 1);

	// set the initial rotation quaternion
	XMStoreFloat4(&rotation, XMQuaternionIdentity());

	// set the mesh pointer
	mesh = m;

	isDirty = true;

	material = mat;
}

Entity::~Entity()
{
	
}

DirectX::XMFLOAT4X4 Entity::GetWorldMatrix()
{
	return worldMatrix;
}

void Entity::SetTranslation(DirectX::XMFLOAT3 tran)
{
	position = tran;
	SetDirtyMatrix();
}

void Entity::SetScale(DirectX::XMFLOAT3 sc)
{
	scale = sc;
	SetDirtyMatrix();
}

void Entity::SetRotation(DirectX::XMFLOAT4 rot)
{
	rotation = rot;
	SetDirtyMatrix();
}

void Entity::RotateAroundAxis(DirectX::XMFLOAT3 axis, float angle)
{
	XMStoreFloat4(&rotation, XMQuaternionMultiply(XMLoadFloat4(&rotation), XMQuaternionRotationAxis(XMLoadFloat3(&axis), angle)));
	SetDirtyMatrix();
}

void Entity::Move(DirectX::XMFLOAT3 amount)
{
	// convert position and the amount to vectors
	// add them
	// store back into the position float
	XMStoreFloat3(&position, XMVectorAdd(XMLoadFloat3(&position), XMLoadFloat3(&amount)));

	SetDirtyMatrix();
}
void Entity::MoveForward(float amount)
{
	XMVECTOR dir = XMVector3Rotate(XMVectorSet(0, 0, amount, 0), XMLoadFloat4(&rotation));

	XMStoreFloat3(&position, XMVectorAdd(XMLoadFloat3(&position), dir));

	SetDirtyMatrix();
}

void Entity::SetDirtyMatrix()
{
	isDirty = true;
}

void Entity::ComputeWorldMatrix()
{
	if (isDirty == false) return;

	XMMATRIX translation = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX scaling = XMMatrixScaling(scale.x, scale.y, scale.z);
	XMMATRIX rot = XMMatrixRotationQuaternion(XMLoadFloat4(&rotation));
	XMMATRIX world = XMLoadFloat4x4(&worldMatrix);

	world = scaling * rot * translation;

	XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(world));

	isDirty = false;
}

ID3D11Buffer* Entity::GetVertexBuffer()
{
	return mesh->GetVertexBuffer();
}

ID3D11Buffer* Entity::GetIndexBuffer()
{
	return mesh->GetIndexBuffer();
}

int Entity::GetIndexCount()
{
	return mesh->GetIndexCount();
}

Material* Entity::GetMaterial()
{
	return material;
}

void Entity::PrepareMaterial(DirectX::XMFLOAT4X4 view, DirectX::XMFLOAT4X4 proj, SpotLight* light, PointLight* light2)
{
	// Send data to shader variables
		//  - Do this ONCE PER OBJECT you're drawing
		//  - This is actually a complex process of copying data to a local buffer
		//    and then copying that entire buffer to the GPU.  
		//  - The "SimpleShader" class handles all of that for you.
	material->GetVertexShader()->SetMatrix4x4("world", worldMatrix);
	material->GetVertexShader()->SetMatrix4x4("view", view);
	material->GetVertexShader()->SetMatrix4x4("projection", proj);
	material->GetPixelShader()->SetData("light", light, sizeof(SpotLight));
	material->GetPixelShader()->SetData("light2", light2, sizeof(PointLight));
	material->GetPixelShader()->SetFloat("shininess", material->GetShininess());
	material->GetPixelShader()->SetShaderResourceView("diffuseTexture", material->GetTexture());
	material->GetPixelShader()->SetShaderResourceView("normalMap", material->GetNormalMap());
	material->GetPixelShader()->SetSamplerState("basicSampler", material->GetSampler());

	// Once you've set all of the data you care to change for
	// the next draw call, you need to actually send it to the GPU
	//  - If you skip this, the "SetMatrix" calls above won't make it to the GPU!
	material->GetVertexShader()->CopyAllBufferData();
	material->GetPixelShader()->CopyAllBufferData();

	// Set the vertex and pixel shaders to use for the next Draw() command
	//  - These don't technically need to be set every frame...YET
	//  - Once you start applying different shaders to different objects,
	//    you'll need to swap the current shaders before each draw
	material->GetVertexShader()->SetShader();
	material->GetPixelShader()->SetShader();
}