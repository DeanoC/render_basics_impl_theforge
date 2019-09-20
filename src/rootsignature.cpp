#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/rootsignature.h"

AL2O3_EXTERN_C Render_RootSignatureHandle Render_RootSignatureCreate(Render_RendererHandle renderer,
																																		 Render_RootSignatureDesc const *desc) {
	TheForge_RootSignatureDesc rootSignatureDesc{};
	rootSignatureDesc.shaderCount = desc->shaderCount;
	rootSignatureDesc.pShaders = desc->shaders;
	rootSignatureDesc.staticSamplerCount = desc->staticSamplerCount;
	rootSignatureDesc.pStaticSamplerNames = desc->staticSamplerNames;
	rootSignatureDesc.pStaticSamplers = desc->staticSamplers;

	Render_RootSignatureHandle rootSig = nullptr;
	TheForge_AddRootSignature(renderer->renderer, &rootSignatureDesc, &rootSig);
	if (!rootSig) {
		return nullptr;
	}

	return rootSig;
}

AL2O3_EXTERN_C void Render_RootSignatureDestroy(Render_RendererHandle renderer,
																								Render_RootSignatureHandle rootSignature) {
	if (!renderer || !rootSignature) {
		return;
	}

	TheForge_RemoveRootSignature(renderer->renderer, rootSignature);
}

