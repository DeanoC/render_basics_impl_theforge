#pragma once

#include "al2o3_platform/platform.h"
#include "al2o3_handle/dynamic.h"
#include "render_basics/theforge/api.h"

typedef struct Render_HandleManagerTheForge {

	Handle_DynamicManager32* frameBuffers;
	Handle_DynamicManager32* blendStates;
	Handle_DynamicManager32* blitEncoders;
	Handle_DynamicManager32* buffers;
	Handle_DynamicManager32* computeEncoders;
	Handle_DynamicManager32* depthStates;
	Handle_DynamicManager32* descriptorSets;
	Handle_DynamicManager32* graphicsEncoders;
	Handle_DynamicManager32* queues;
	Handle_DynamicManager32* pipelines;
	Handle_DynamicManager32* rasteriserStates;
	Handle_DynamicManager32* rootSignatures;
	Handle_DynamicManager32* samplers;
	Handle_DynamicManager32* shaderObjects;
	Handle_DynamicManager32* shaders;
	Handle_DynamicManager32* textures;

} Render_HandleManagerTheForge;

AL2O3_EXTERN_C Render_HandleManagerTheForge* g_Render_HandleManagerTheForge;

#define RENDER_HANDLE_BUILD(type, manager) \
AL2O3_FORCE_INLINE Render_##type##Handle Render_##type##HandleAlloc(void) { \
	Render_##type##Handle handle; \
	handle.handle = Handle_DynamicManager32Alloc(g_Render_HandleManagerTheForge->manager); \
	return handle; \
} \
AL2O3_FORCE_INLINE void Render_##type##HandleRelease(Render_##type##Handle handle) { \
	Handle_DynamicManager32Release(g_Render_HandleManagerTheForge->manager, handle.handle); \
} \
AL2O3_FORCE_INLINE bool Render_##type##HandleIsValid(Render_##type##Handle handle) { \
	return Handle_DynamicManager32IsValid(g_Render_HandleManagerTheForge->manager, handle.handle); \
} \
AL2O3_FORCE_INLINE Render_##type* Render_##type##HandleToPtr(Render_##type##Handle handle) { \
	return (Render_##type*) Handle_DynamicManager32HandleToPtr(g_Render_HandleManagerTheForge->manager, handle.handle); \
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
