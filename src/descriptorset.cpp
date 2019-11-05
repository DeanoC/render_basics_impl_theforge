#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/descriptorset.h"
#include "render_basics/theforge/handlemanager.h"

AL2O3_EXTERN_C Render_DescriptorSetHandle Render_DescriptorSetCreate(Render_RendererHandle renderer,
																																		 Render_DescriptorSetDesc const *desc) {

	TheForge_DescriptorSetDesc tfdesc{};
	Render_RootSignature* rrs = Render_RootSignatureHandleToPtr(desc->rootSignature);
	tfdesc.rootSignature = rrs->signature;

	tfdesc.maxSets = desc->maxSets * ((desc->updateFrequency == Render_DUF_NEVER) ? 1 : renderer->maxFramesAhead);
	switch (desc->updateFrequency) {
		case Render_DUF_NEVER: tfdesc.updateFrequency = TheForge_DESCRIPTOR_UPDATE_FREQ_NONE;
			break;
		case Render_DUF_PER_FRAME: tfdesc.updateFrequency = TheForge_DESCRIPTOR_UPDATE_FREQ_PER_FRAME;
			break;
		case Render_DUF_PER_BATCH: tfdesc.updateFrequency = TheForge_DESCRIPTOR_UPDATE_FREQ_PER_FRAME;
			break;
		case Render_DUF_PER_DRAW: tfdesc.updateFrequency = TheForge_DESCRIPTOR_UPDATE_FREQ_PER_DRAW;
			break;
	}

	Render_DescriptorSetHandle handle = Render_DescriptorSetHandleAlloc();
	Render_DescriptorSet* ds = Render_DescriptorSetHandleToPtr(handle);
	ds->frequency = tfdesc.updateFrequency;
	ds->maxSetsPerFrame = desc->maxSets;
	ds->setIndexOffset = 0;
	ds->renderer = renderer;
	TheForge_AddDescriptorSet(renderer->renderer, &tfdesc, &ds->descriptorSet);
	return handle;

}

AL2O3_EXTERN_C void Render_DescriptorSetDestroy(Render_RendererHandle renderer,
																								Render_DescriptorSetHandle handle) {
	if (!renderer || !Render_DescriptorSetHandleIsValid(handle)) {
		return;
	}

	Render_DescriptorSet* ds = Render_DescriptorSetHandleToPtr(handle);
	TheForge_RemoveDescriptorSet(renderer->renderer, ds->descriptorSet);
	Render_DescriptorSetHandleRelease(handle);

}

static void descriptorUpdate(Render_DescriptorSetHandle handle,
																						 uint32_t setIndex,
																						 uint32_t numDescriptors,
																						 Render_DescriptorDesc const *desc,
																						 uint32_t frameIndex ) {
	TheForge_DescriptorData* dd = (TheForge_DescriptorData *) STACK_ALLOC(sizeof(TheForge_DescriptorData) * numDescriptors);
	memset(dd, 0, sizeof(TheForge_DescriptorData) * numDescriptors);
	uint64_t *offsets = (uint64_t *) STACK_ALLOC(sizeof(uint64_t) * numDescriptors);
	memset(offsets, 0, sizeof(uint64_t) * numDescriptors);

	TheForge_TextureHandle* textures = (TheForge_TextureHandle*) STACK_ALLOC(sizeof(TheForge_TextureHandle) * numDescriptors);
	TheForge_BufferHandle* buffers = (TheForge_BufferHandle*) STACK_ALLOC(sizeof(TheForge_BufferHandle) * numDescriptors);
	TheForge_SamplerHandle* samplers = (TheForge_SamplerHandle*) STACK_ALLOC(sizeof(TheForge_SamplerHandle) * numDescriptors);

	for (uint32_t i = 0; i < numDescriptors; ++i) {
		dd[i].pName = desc[i].name;
		dd[i].count = 1;
		dd[i].index = ~0; // use name not index currently
		switch (desc[i].type) {
			case Render_DT_TEXTURE:
				textures[i] = Render_TextureHandleToPtr(desc[i].texture)->texture;
				dd[i].pTextures = &textures[i];
				break;
			case Render_DT_SAMPLER:
				samplers[i] = Render_SamplerHandleToPtr(desc[i].sampler)->sampler;
				dd[i].pSamplers = &samplers[i];
				break;
			case Render_DT_BUFFER: {
				Render_Buffer *buffer = Render_BufferHandleToPtr(desc[i].buffer);
				buffers[i] = buffer->buffer;
				offsets[i] = (frameIndex * buffer->size) + desc[i].offset;
				dd[i].pOffsets = &offsets[i];
				dd[i].pSizes = &desc[i].size;
				dd[i].pBuffers = &buffers[i];
				break;
			}
		}
	}

	Render_DescriptorSet* set = Render_DescriptorSetHandleToPtr(handle);

	// frame has changed and we have frequency >= frame rate adjust set index
	if (set->frequency != TheForge_DESCRIPTOR_UPDATE_FREQ_NONE) {
		set->setIndexOffset = frameIndex * set->maxSetsPerFrame;
	}

	TheForge_UpdateDescriptorSet(set->renderer->renderer,
															 set->setIndexOffset + setIndex,
															 set->descriptorSet,
															 numDescriptors,
															 dd);
}

AL2O3_EXTERN_C void 	Render_DescriptorUpdate(Render_DescriptorSetHandle handle,
																						uint32_t setIndex,
																						uint32_t numDescriptors,
																						Render_DescriptorDesc const *desc) {

	descriptorUpdate(handle, setIndex, numDescriptors, desc, Render_DescriptorSetHandleToPtr(handle)->renderer->frameIndex);
}

// optimization that prefills all the per frame buffers
AL2O3_EXTERN_C void 	Render_DescriptorPresetFrequencyUpdated(Render_DescriptorSetHandle handle,
																						 uint32_t setIndex,
																						 uint32_t numDescriptors,
																						 Render_DescriptorDesc const *desc) {
	for(uint32_t i = 0;i < Render_DescriptorSetHandleToPtr(handle)->maxSetsPerFrame;++i) {
		descriptorUpdate(handle, setIndex, numDescriptors, desc, i);
	}

}