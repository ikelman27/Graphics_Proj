#include "Emitter.h"
using namespace DirectX;
Emitter::Emitter(int maxParticles, float timeBetweenSpawn, float particleLifetime, DirectX::XMFLOAT4 startColor, DirectX::XMFLOAT4 endColor, DirectX::XMFLOAT3 startVelocity, DirectX::XMFLOAT3 velocityRandomRange, DirectX::XMFLOAT3 emitterPosition, DirectX::XMFLOAT3 positionRandomRange, DirectX::XMFLOAT4 rotationRandomRanges, DirectX::XMFLOAT3 emitterAcceleration, ID3D11Device* device, SimpleVertexShader* vs, SimplePixelShader* ps, ID3D11ShaderResourceView* texture)
{
    this->vs = vs;
    this->ps = ps;
    this->texture = texture;

    this->maxParticle = maxParticles;
    this->spawnTime = timeBetweenSpawn;
    this->maxParticleLifetime = particleLifetime;

    this->startColor = startColor;
    this->endColor = endColor;
    this->startVelocity = startVelocity;
    this->startSize = 1.0f;
    this->endSize = 5.1f;


    this->velocityRandomRange = velocityRandomRange;
    this->positionRandomRange = positionRandomRange;
    this->rotationRandomRanges = rotationRandomRanges;

    this->emitterPosition = emitterPosition;
    this->emitterAcceleration = emitterAcceleration;

    timeSinceSpawn = 0;
    liveParticleCount = 0;
    firstAliveIndex = 0;
    firstDeadIndex = 0;

    particles = new Particle[maxParticles];
    ZeroMemory(particles, sizeof(Particle) * maxParticles);

    DefaultUVs[0] = XMFLOAT2(0, 0);
    DefaultUVs[1] = XMFLOAT2(1, 0);
    DefaultUVs[2] = XMFLOAT2(1, 1);
    DefaultUVs[3] = XMFLOAT2(0, 1);

    partVerts = new ParticleVertex[4 * maxParticles];
    for (int i = 0; i < maxParticles * 4; i += 4)
    {
        partVerts[i + 0].UV = DefaultUVs[0];
        partVerts[i + 1].UV = DefaultUVs[1];
        partVerts[i + 2].UV = DefaultUVs[2];
        partVerts[i + 3].UV = DefaultUVs[3];
    }


    // Create buffers for drawing particles

    // DYNAMIC vertex buffer (no initial data necessary)
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.ByteWidth = sizeof(ParticleVertex) * 4 * maxParticles;
    device->CreateBuffer(&vbDesc, 0, &vertexBuffer);

    // Index buffer data
    unsigned int* indices = new unsigned int[maxParticles * 6];
    int indexCount = 0;
    for (int i = 0; i < maxParticles * 4; i += 4)
    {
        indices[indexCount++] = i;
        indices[indexCount++] = i + 1;
        indices[indexCount++] = i + 2;
        indices[indexCount++] = i;
        indices[indexCount++] = i + 2;
        indices[indexCount++] = i + 3;
    }
    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices;

    // Regular (static) index buffer
    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = 0;
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = sizeof(unsigned int) * maxParticles * 6;
    device->CreateBuffer(&ibDesc, &indexData, &indexBuffer);

    delete[] indices;

}

Emitter::~Emitter()
{
    delete[] particles;
    delete[] partVerts;
    vertexBuffer->Release();
    indexBuffer->Release();
}

void Emitter::Update(float deltaTime)
{
    if (firstAliveIndex < firstDeadIndex) {
        for (int i = firstAliveIndex; i < firstDeadIndex; i++) {
            UpdateSingleParticle(deltaTime, i);
        }
    }
    else {
        for (int i = firstAliveIndex; i < maxParticle; i++) {
            UpdateSingleParticle(deltaTime, i);
        }
        for (int i = 0; i < firstDeadIndex; i++) {
            UpdateSingleParticle(deltaTime, i);
        }
    }

    timeSinceSpawn += deltaTime;

    if (timeSinceSpawn > spawnTime) {
        SpawnParticle();
        timeSinceSpawn = 0;
    }
}

void Emitter::Draw(ID3D11DeviceContext* context, Camera* cam)
{
    CopyParticlesToGPU(context, cam);

    UINT stride = sizeof(ParticleVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    vs->SetMatrix4x4("view", cam->GetViewMatrix());
    vs->SetMatrix4x4("projection", cam->GetProjectionMatrix());
    vs->SetShader();
    vs->CopyAllBufferData();

    ps->SetShaderResourceView("particle", texture);

    ps->SetShader();
}

void Emitter::UpdateSingleParticle(float dt, int index)
{
    if (particles[index].Age > maxParticleLifetime) {
        return;
    }
    particles[index].Age += dt;
    if (particles[index].Age >= maxParticleLifetime)
    {
        // Recent death, so retire by moving alive count
        firstAliveIndex++;
        firstAliveIndex %= maxParticle;
        liveParticleCount--;
        return;
    }

    float agePercent = particles[index].Age / maxParticleLifetime;

    XMStoreFloat4(
        &particles[index].Color,
        XMVectorLerp(
            XMLoadFloat4(&startColor),
            XMLoadFloat4(&endColor),
            agePercent));

    // Interpolate the rotation
    float rotStart = particles[index].RotationStart;
    float rotEnd = particles[index].RotationEnd;
    particles[index].Rotation = rotStart + agePercent * (rotEnd - rotStart);

    // Interpolate the size
    particles[index].Size = startSize + agePercent * (endSize - startSize);

    // Adjust the position
    XMVECTOR startPos = XMLoadFloat3(&particles[index].StartPosition);
    XMVECTOR startVel = XMLoadFloat3(&particles[index].StartVelocity);
    XMVECTOR accel = XMLoadFloat3(&emitterAcceleration);
    float t = particles[index].Age;

    // Use constant acceleration function
    XMStoreFloat3(
        &particles[index].Position,
        accel * t * t / 2.0f + startVel * t + startPos);
}

void Emitter::SpawnParticle()
{
    if (liveParticleCount >= maxParticle) {
        return;
    }

    particles[firstDeadIndex].Age = 0;
    particles[firstDeadIndex].Size = startSize;
    particles[firstDeadIndex].Color = startColor;

    particles[firstDeadIndex].StartPosition = emitterPosition;
    particles[firstDeadIndex].StartPosition.x += (((float)rand() / RAND_MAX) * 2 - 1) * positionRandomRange.x;
    particles[firstDeadIndex].StartPosition.y += (((float)rand() / RAND_MAX) * 2 - 1) * positionRandomRange.y;
    particles[firstDeadIndex].StartPosition.z += (((float)rand() / RAND_MAX) * 2 - 1) * positionRandomRange.z;
    particles[firstDeadIndex].Position = particles[firstDeadIndex].StartPosition;
    
    particles[firstDeadIndex].StartVelocity = startVelocity;

    particles[firstDeadIndex].StartVelocity.x += (((float)rand() / RAND_MAX) * 2 - 1) * velocityRandomRange.x;
    particles[firstDeadIndex].StartVelocity.y += (((float)rand() / RAND_MAX) * 2 - 1) * velocityRandomRange.y;
    particles[firstDeadIndex].StartVelocity.z += (((float)rand() / RAND_MAX) * 2 - 1) * velocityRandomRange.z;

    float rotStartMin = rotationRandomRanges.x;
    float rotStartMax = rotationRandomRanges.y;

    particles[firstDeadIndex].RotationStart = ((float)rand() / RAND_MAX) * (rotStartMax - rotStartMin) + rotStartMin;
    float rotEndMin = rotationRandomRanges.z;
    float rotEndMax = rotationRandomRanges.w;
    particles[firstDeadIndex].RotationEnd = ((float)rand() / RAND_MAX) * (rotEndMax - rotEndMin) + rotEndMin;
    
    // Increment and wrap
    firstDeadIndex++;
    firstDeadIndex %= maxParticle;

    liveParticleCount++;

}

void Emitter::CopyParticlesToGPU(ID3D11DeviceContext* context, Camera* camera)
{
    if (firstAliveIndex < firstDeadIndex) {
        for (int i = firstAliveIndex; i < firstDeadIndex; i++) {
            CopyOneParticle(i, camera);
        }
    }
    else {
        for (int i = firstAliveIndex; i < maxParticle; i++) {
            CopyOneParticle(i, camera);
        }
        for (int i = 0; i < firstDeadIndex; i++) {
            CopyOneParticle(i, camera);
        }
    }

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    context->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

    memcpy(mapped.pData, partVerts, sizeof(ParticleVertex) * 4 * maxParticle);

    context->Unmap(vertexBuffer, 0);
}

void Emitter::CopyOneParticle(int index, Camera* camera)
{
    int i = index * 4;

    partVerts[i + 0].Position = CalcParticleVertexPosition(index, 0, camera);
    partVerts[i + 1].Position = CalcParticleVertexPosition(index, 1, camera);
    partVerts[i + 2].Position = CalcParticleVertexPosition(index, 2, camera);
    partVerts[i + 3].Position = CalcParticleVertexPosition(index, 3, camera);

    partVerts[i + 0].Color = particles[index].Color;
    partVerts[i + 1].Color = particles[index].Color;
    partVerts[i + 2].Color = particles[index].Color;
    partVerts[i + 3].Color = particles[index].Color;
}

DirectX::XMFLOAT3 Emitter::CalcParticleVertexPosition(int particleIndex, int quadCornerIndex, Camera* camera)
{
    // Get the right and up vectors out of the view matrix
    // (Remember that it is probably already transposed)
    XMFLOAT4X4 view = camera->GetViewMatrix();
    XMVECTOR camRight = XMVectorSet(view._11, view._12, view._13, 0);
    XMVECTOR camUp = XMVectorSet(view._21, view._22, view._23, 0);

    // Determine the offset of this corner of the quad
    // Since the UV's are already set when the emitter is created, 
    // we can alter that data to determine the general offset of this corner
    XMFLOAT2 offset = DefaultUVs[quadCornerIndex];
    offset.x = offset.x * 2 - 1;	// Convert from [0,1] to [-1,1]
    offset.y = (offset.y * -2 + 1);	// Same, but flip the Y

    // Load into a vector, which we'll assume is float3 with a Z of 0
    // Create a Z rotation matrix and apply it to the offset
    XMVECTOR offsetVec = XMLoadFloat2(&offset);
    XMMATRIX rotMatrix = XMMatrixRotationZ(particles[particleIndex].Rotation);
    offsetVec = XMVector3Transform(offsetVec, rotMatrix);

    // Add and scale the camera up/right vectors to the position as necessary
    XMVECTOR posVec = XMLoadFloat3(&particles[particleIndex].Position);
    posVec += camRight * XMVectorGetX(offsetVec) * particles[particleIndex].Size;
    posVec += camUp * XMVectorGetY(offsetVec) * particles[particleIndex].Size;

    // This position is all set
    XMFLOAT3 pos;
    XMStoreFloat3(&pos, posVec);
    return pos;
}
