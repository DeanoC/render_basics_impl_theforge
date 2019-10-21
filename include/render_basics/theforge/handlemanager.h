#pragma once

#include "al2o3_platform/platform.h"
#include "al2o3_handle/handle.h"
#include "render_basics/theforge/api.h"

typedef struct Render_HandleManagerTheForge {

	Handle_Manager32* frameBuffers;
	Handle_Manager32* blendStates;
	Handle_Manager32* blitEncoders;
	Handle_Manager32* buffers;
	Handle_Manager32* computeEncoders;
	Handle_Manager32* depthStates;
	Handle_Manager32* descriptorSets;
	Handle_Manager32* graphicsEncoders;
	Handle_Manager32* queues;
	Handle_Manager32* pipelines;
	Handle_Manager32* rasteriserStates;
	Handle_Manager32* rootSignatures;
	Handle_Manager32* samplers;
	Handle_Manager32* shaderObjects;
	Handle_Manager32* shaders;
	Handle_Manager32* textures;

} Render_HandleManagerTheForge;

AL2O3_EXTERN_C Render_HandleManagerTheForge* g_Render_HandleManagerTheForge;

#define RENDER_HANDLE_BUILD(type, manager) \
AL2O3_FORCE_INLINE Render_##type##Handle Render_##type##HandleAlloc(void) { \
	Render_##type##Handle handle; \
	handle.handle = Handle_Manager32Alloc(g_Render_HandleManagerTheForge->manager); \
	return handle; \
} \
AL2O3_FORCE_INLINE void Render_##type##HandleRelease(Render_##type##Handle handle) { \
	Handle_Manager32Release(g_Render_HandleManagerTheForge->manager, handle.handle); \
} \
AL2O3_FORCE_INLINE bool Render_##type##HandleIsValid(Render_##type##Handle handle) { \
	return Handle_Manager32IsValid(g_Render_HandleManagerTheForge->manager, handle.handle); \
} \
AL2O3_FORCE_INLINE Render_##type* Render_##type##HandleToPtr(Render_##type##Handle handle) { \
	return (Render_##type*) Handle_Manager32HandleToPtr(g_Render_HandleManagerTheForge->manager, handle.handle); \
}

RENDER_HANDLE_BUILD(FrameBuffer, frameBuffers);
RENDER_HANDLE_BUILD(BlendState, blendStates);
RENDER_HANDLE_BUILD(BlitEncoder, blitEncoders);
RENDER_HANDLE_BUILD(Buffer, buffers);
RENDER_HANDLE_BUILD(ComputeEncoder, computeEncoders);
RENDER_HANDLE_BUILD(DepthState, depthStates);
RENDER_HANDLE_BUILD(DescriptorSet, descriptorSets);
RENDER_HANDLE_BUILD(GraphicsEncoder, graphicsEncoders);
RENDER_HANDLE_BUILD(Queue, queues);
RENDER_HANDLE_BUILD(Pipeline, pipelines);
RENDER_HANDLE_BUILD(RasteriserState, rasteriserStates);
RENDER_HANDLE_BUILD(RootSignature, rootSignatures);
RENDER_HANDLE_BUILD(Sampler, samplers);
RENDER_HANDLE_BUILD(ShaderObject, shaderObjects);
RENDER_HANDLE_BUILD(Shader, shaders);
RENDER_HANDLE_BUILD(Texture, textures);

#undef RENDER_HANDLE_BUILD
