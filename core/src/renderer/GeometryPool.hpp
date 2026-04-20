#pragma once

class GeometryPool {
public:
    SDL_GPUBuffer* megaVertexBuffer = nullptr;
    SDL_GPUBuffer* megaIndexBuffer = nullptr;
    uint32_t vertexHead = 0;
    uint32_t indexHead = 0;
    uint32_t maxVertices = 1 << 20;
    uint32_t maxIndices = 1 << 22;
    uint32_t size = 0;

    void init(flecs::world & ecs) {

        const RenderContext& renderContext = ecs.get<RenderContext>();

        SDL_GPUBufferCreateInfo vbInfo = {};
        vbInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vbInfo.size = maxVertices * sizeof(Vertex);
        megaVertexBuffer = SDL_CreateGPUBuffer(renderContext.device, &vbInfo);

        SDL_GPUBufferCreateInfo ibInfo = {};
        ibInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        ibInfo.size = maxIndices * sizeof(uint32_t);
        megaIndexBuffer = SDL_CreateGPUBuffer(renderContext.device, &ibInfo);
    }

    bool isEmpty() const { return size == 0; }

    void upload(SDL_GPUDevice* device, Mesh& mesh) {
        size_t vSize = mesh.vertices.size() * sizeof(Vertex);
        size_t iSize = mesh.indices.size() * sizeof(uint32_t);

        assert(megaVertexBuffer && megaIndexBuffer && "GeometryPool not initialized");
        assert(vertexHead + mesh.vertices.size() <= maxVertices);
        assert(indexHead + mesh.indices.size() <= maxIndices);

        mesh.baseVertex = vertexHead;
        mesh.firstIndex = indexHead;
        mesh.vertexCount = (uint32_t)mesh.vertices.size();
        mesh.indexCount = (uint32_t)mesh.indices.size();

        appendToBuffer(device, megaVertexBuffer, mesh.vertices.data(), vSize,
            vertexHead * sizeof(Vertex));
        appendToBuffer(device, megaIndexBuffer, mesh.indices.data(), iSize,
            indexHead * sizeof(uint32_t));

        vertexHead += mesh.vertexCount;
        indexHead += mesh.indexCount;
        size++;
    }

    void release(SDL_GPUDevice* device) {
        if (megaVertexBuffer) { SDL_ReleaseGPUBuffer(device, megaVertexBuffer); megaVertexBuffer = nullptr; }
        if (megaIndexBuffer) { SDL_ReleaseGPUBuffer(device, megaIndexBuffer);  megaIndexBuffer = nullptr; }
        vertexHead = indexHead = size = 0;
    }

private:
    void appendToBuffer(SDL_GPUDevice* device, SDL_GPUBuffer* dstBuffer,
        const void* data, size_t size, size_t dstOffsetBytes)
    {
        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = (Uint32)size;
        SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device, &transferInfo);

        void* mapped = SDL_MapGPUTransferBuffer(device, tb, false);
        memcpy(mapped, data, size);
        SDL_UnmapGPUTransferBuffer(device, tb);

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(cmd);

        SDL_GPUTransferBufferLocation src = { tb, 0 };
        SDL_GPUBufferRegion dst = { dstBuffer, (Uint32)dstOffsetBytes, (Uint32)size };

        SDL_UploadToGPUBuffer(pass, &src, &dst, false);
        SDL_EndGPUCopyPass(pass);
        SDL_SubmitGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(device, tb);
    }
};