#pragma once

#include "al2o3_platform/platform.h"
#include "gfx_theforge/theforge.h"
#include "gfx_shadercompiler/compiler.h"

#define Render_VertexLayout TheForge_VertexLayout
#include "render_basics/api.h"
#include "render_basics/shader.h"
#include "render_basics/view.h"

typedef struct Render_FrameBuffer {
	Render_RendererHandle renderer;

	void *platformHandle;                ///< platform specific for the window/display (HWND etc.)
	TheForge_CmdPoolHandle commandPool;
	Render_QueueHandle presentQueue;
	uint32_t frameBufferCount;
	TinyImageFormat colourBufferFormat;

	TheForge_SwapChainHandle swapChain;
	TheForge_FenceHandle *renderCompleteFences;
	TheForge_SemaphoreHandle imageAcquiredSemaphore;
	TheForge_SemaphoreHandle *renderCompleteSemaphores;
	TheForge_CmdHandle *frameCmds;

	Math_Vec4F entireViewport;
	Math_Vec4U32 entireScissor;

	struct ImguiBindings_Context *imguiBindings;

	struct RenderTF_VisualDebug *visualDebug;
	struct Render_GpuView *debugGpuView;

	// current (this frame) data
	Render_TextureHandle currentColourTarget;
	Render_GraphicsEncoderHandle graphicsEncoder;

} Render_FrameBuffer;

typedef struct Render_BlendState {
	TheForge_BlendStateHandle state;
} Render_BlendState;

typedef struct Render_BlitEncoder {
	TheForge_CmdPoolHandle cmdPool;

	TheForge_CmdHandle cmd;
} Render_BlitEncoder;

typedef struct Render_Buffer {
	Render_RendererHandle renderer;
	TheForge_BufferHandle buffer;

	uint64_t size; // size of a single frame, total size = maxFrame * size
	bool frequentlyUpdated;
} Render_Buffer;

typedef struct Render_ComputeEncoder {
	TheForge_CmdPoolHandle cmdPool;

	TheForge_CmdHandle cmd;
} Render_ComputeEncoder;

typedef struct Render_DepthState {
	TheForge_DepthStateHandle state;
} Render_DepthState;

typedef struct Render_DescriptorSet {
	Render_RendererHandle renderer;
	TheForge_DescriptorSetHandle descriptorSet;
	TheForge_DescriptorUpdateFrequency frequency;
	uint32_t maxSetsPerFrame;
	uint32_t setIndexOffset;
} Render_DescriptorSet;

typedef struct Render_GraphicsEncoder {
	TheForge_CmdHandle cmd;
	Render_View view;
} Render_GraphicsEncoder;

typedef struct Render_Queue {
	TheForge_QueueHandle queue;
} Render_Queue;

typedef struct Render_Pipeline {
	TheForge_PipelineHandle pipeline;
} Render_Pipeline;

typedef struct Render_RasteriserState {
	TheForge_RasterizerStateHandle state;
} Render_RasteriserState;

typedef struct Render_RootSignature {
	TheForge_RootSignatureHandle signature;
} Render_RootSignature;

typedef struct Render_Sampler {
	TheForge_SamplerHandle sampler;
} Render_Sampler;

typedef struct Render_ShaderObject {
	TheForge_ShaderStage shaderType;
	ShaderCompiler_Output output;
	char name[64];
	char entryPoint[64];
} Render_ShaderObject;

typedef struct Render_Shader {
	TheForge_ShaderHandle shader;
} Render_Shader;

typedef struct Render_Texture {
	TheForge_TextureHandle texture;
	TheForge_RenderTargetHandle	renderTarget;
} Render_Texture;

typedef struct Render_Renderer {
	TheForge_RendererHandle renderer;

	InputBasic_ContextHandle input; ///< can be null

	Render_QueueHandle graphicsQueue;
	Render_QueueHandle computeQueue;
	Render_QueueHandle blitQueue;

	TheForge_CmdPoolHandle graphicsCmdPool;
	TheForge_CmdPoolHandle computeCmdPool;
	TheForge_CmdPoolHandle blitCmdPool;

	ShaderCompiler_ContextHandle shaderCompiler;

	Render_BlendStateHandle stockBlendState[Render_SBS_COUNT];
	Render_DepthStateHandle stockDepthState[Render_SDS_COUNT];
	Render_RasteriserStateHandle stockRasteriserState[Render_SRS_COUNT];
	Render_SamplerHandle stockSamplers[Render_SST_COUNT];
	Render_VertexLayout const *stockVertexLayouts[Render_SVL_COUNT];

	uint32_t maxFramesAhead;
	uint32_t frameIndex;

} Render_Renderer;


