#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/descriptorset.h"

AL2O3_EXTERN_C Render_DescriptorSetHandle Render_DescriptorSetCreate(Render_RendererHandle renderer,
																																		 Render_DescriptorSetDesc const *desc) {
	if (!renderer || desc == nullptr) {
		return nullptr;
	}

	auto ds = (Render_DescriptorSetHandle) MEMORY_CALLOC(1, sizeof(Render_DescriptorSet));

	TheForge_DescriptorSetDesc tfdesc{};
	tfdesc.rootSignature = desc->rootSignature;
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
	ds->frequency = tfdesc.updateFrequency;
	ds->maxSetsPerFrame = desc->maxSets;
	ds->setIndexOffset = 0;
	ds->renderer = renderer;
	TheForge_AddDescriptorSet(renderer->renderer, &tfdesc, &ds->descriptorSet);
	return ds;
}

AL2O3_EXTERN_C void Render_DescriptorSetDestroy(Render_RendererHandle renderer,
																								Render_DescriptorSetHandle descSet) {
	if (!renderer || !descSet) {
		return;
	}

	TheForge_RemoveDescriptorSet(renderer->renderer, descSet->descriptorSet);
	MEMORY_FREE(descSet);
}

AL2O3_EXTERN_C void Render_DescriptorUpdate(Render_DescriptorSetHandle set,
																						uint32_t setIndex,
																						uint32_t numDescriptors,
																						Render_DescriptorDesc *desc) {
	auto dd = (TheForge_DescriptorData *) STACK_ALLOC(sizeof(TheForge_DescriptorData) * numDescriptors);
	auto *offsets = (uint64_t *) STACK_ALLOC(sizeof(uint64_t) * numDescriptors);

	for (uint32_t i = 0; i < numDescriptors; ++i) {
		dd[i].pName = desc[i].name;
		dd[i].count = 1;
		switch (desc[i].type) {

			case Render_DT_TEXTURE: dd[i].pTextures = &desc[i].texture;
				break;
			case Render_DT_SAMPLER: dd[i].pSamplers = &desc[i].sampler;
				break;
			case Render_DT_BUFFER:offsets[i] = ((desc[i].buffer->curFrame) * desc[i].buffer->size) + desc[i].offset;
				dd[i].pOffsets = &offsets[i];
				dd[i].pSizes = &desc[i].size;
				dd[i].pBuffers = &desc[i].buffer->buffer;
				break;
			case Render_DT_ROOT_CONSTANT: dd[i].pRootConstant = &desc[i].rootConstant;
				break;
		}
	}

	// frame has changed and we have frequency >= frame rate adjust set index
	if (set->frequency != TheForge_DESCRIPTOR_UPDATE_FREQ_NONE) {
		set->setIndexOffset = set->renderer->frameIndex * set->maxSetsPerFrame;
	}

	TheForge_UpdateDescriptorSet(set->renderer->renderer,
															 set->setIndexOffset + setIndex,
															 set->descriptorSet,
															 numDescriptors,
															 dd);
}

