
#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/rootsignature.h"
#include "render_basics/theforge/handlemanager.h"

AL2O3_EXTERN_C Render_RootSignatureHandle Render_RootSignatureCreate(Render_RendererHandle renderer,
																																		 Render_RootSignatureDesc const *desc) {
	TheForge_ShaderHandle* shaders = (TheForge_ShaderHandle*)STACK_ALLOC(sizeof(TheForge_ShaderHandle*) * desc->shaderCount);
	for(uint32_t i = 0;i < desc->shaderCount;++i) {
		if(!Render_ShaderHandleIsValid(desc->shaders[i])) {
			return {0};
		}

		shaders[i] = Render_ShaderHandleToPtr(desc->shaders[i])->shader;
	}
	TheForge_SamplerHandle* samplers = (TheForge_SamplerHandle*) STACK_ALLOC(sizeof(TheForge_SamplerHandle) * desc->staticSamplerCount);
	for(uint32_t i = 0; i < desc->staticSamplerCount;++i) {
		if(!Render_SamplerHandleIsValid(desc->staticSamplers[i])) {
			return {0};
		}
		samplers[i] = Render_SamplerHandleToPtr(desc->staticSamplers[i])->sampler;
	}
	TheForge_RootSignatureDesc rootSignatureDesc{};
	rootSignatureDesc.shaderCount = desc->shaderCount;
	rootSignatureDesc.pShaders = shaders;
	rootSignatureDesc.staticSamplerCount = desc->staticSamplerCount;
	rootSignatureDesc.pStaticSamplerNames = desc->staticSamplerNames;
	rootSignatureDesc.pStaticSamplers = samplers;

	Render_RootSignatureHandle handle = Render_RootSignatureHandleAlloc();
	Render_RootSignature* rootSig = Render_RootSignatureHandleToPtr(handle);
	TheForge_AddRootSignature(renderer->renderer, &rootSignatureDesc, &rootSig->signature);
	if (!rootSig->signature) {
		Render_RootSignatureHandleRelease(handle);
		return {0};
	}

	return handle;
}

AL2O3_EXTERN_C void Render_RootSignatureDestroy(Render_RendererHandle renderer,
																								Render_RootSignatureHandle handle) {
	if (!renderer || !Render_RootSignatureHandleIsValid(handle)) {
		return;
	}

	Render_RootSignature* rootSig = Render_RootSignatureHandleToPtr(handle);
	TheForge_RemoveRootSignature(renderer->renderer, rootSig->signature);
	Render_RootSignatureHandleRelease(handle);

}

