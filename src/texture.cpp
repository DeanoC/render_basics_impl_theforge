#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "gfx_theforge/theforge.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"
#include "render_basics/texture.h"

TheForge_DescriptorType Render_TextureUsageFlagsToDescriptorType(Render_TextureUsageFlags tuf) {
	uint32_t dt = 0;
	if (tuf & Render_TUF_SHADER_READ) {
		dt |= TheForge_DESCRIPTOR_TYPE_TEXTURE;
	}
	if (tuf & Render_TUF_SHADER_WRITE) {
		dt |= TheForge_DESCRIPTOR_TYPE_RW_TEXTURE;
	}

	if (tuf & Render_TUF_ROP_READ) {
	}

	if (tuf & Render_TUF_ROP_WRITE) {
	}

	return (TheForge_DescriptorType) dt;
}

AL2O3_EXTERN_C Render_TextureHandle Render_TextureSyncCreate(Render_RendererHandle renderer,
																														 Render_TextureCreateDesc const *desc) {

	TheForge_TextureDesc const texDesc{
			TheForge_TCF_NONE,
			desc->width,
			desc->height,
			desc->depth,
			desc->slices,
			desc->mipLevels,
			(TheForge_SampleCount) (desc->sampleCount ? desc->sampleCount : 1),
			desc->sampleQuality,
			desc->format,
			{{
					 desc->renderTargetClearValue.r,
					 desc->renderTargetClearValue.g,
					 desc->renderTargetClearValue.b,
					 desc->renderTargetClearValue.a}},
			TheForge_RS_UNDEFINED,
			Render_TextureUsageFlagsToDescriptorType(desc->usageflags),
			nullptr,
			desc->debugName,
			nullptr,
			0,
			0
	};
	Render_TextureHandle texture = nullptr;
	TheForge_AddTexture(renderer->renderer, &texDesc, &texture);
	if (!texture) {
		return nullptr;
	}

	if (desc->initialData != nullptr) {
		Render_TextureUpdateDesc update{
				desc->format,
				desc->width,
				desc->height,
				desc->depth,
				desc->slices,
				desc->mipLevels,
				desc->initialData,
		};

		Render_TextureSyncUpdate(texture, &update);
	}

	return texture;
}

AL2O3_EXTERN_C void Render_TextureDestroy(Render_RendererHandle renderer, Render_TextureHandle texture) {
	if (!renderer || !texture) {
		return;
	}
	TheForge_RemoveTexture(renderer->renderer, texture);
}

AL2O3_EXTERN_C void Render_TextureSyncUpdate(Render_TextureHandle texture, Render_TextureUpdateDesc const *desc) {
	TheForge_RawImageData rid{
			(unsigned char const *) desc->data,
			desc->format,
			desc->width,
			desc->height,
			desc->depth,
			desc->slices,
			desc->mipLevels,
	};

	TheForge_TextureUpdateDesc updateDesc{
			texture,
			&rid
	};

	TheForge_UpdateTexture(&updateDesc, false);
}


