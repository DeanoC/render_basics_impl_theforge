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

	return (TheForge_DescriptorType) dt;
}

AL2O3_EXTERN_C Render_TextureHandle Render_TextureSyncCreate(Render_RendererHandle renderer,
																														 Render_TextureCreateDesc const *desc) {

	Render_TextureHandle texture = (Render_TextureHandle) MEMORY_CALLOC(1, sizeof(Render_Texture));
	if (!texture) {
		return nullptr;
	}

	// the forge has seperate textures and render targets, whereas we just define it via ROP_READ/WRITE
	// split creation if ROP_WRITE is defined
	if(desc->usageflags & Render_TUF_ROP_WRITE) {
		// TODO check render target format can be written to and if require blended with

		TheForge_RenderTargetDesc rtDesc {
			TheForge_TCF_OWN_MEMORY_BIT,
			desc->width,
			desc->height,
			desc->depth,
			desc->slices,
			desc->mipLevels,
			(TheForge_SampleCount) (desc->sampleCount ? desc->sampleCount : 1),
			desc->format,
			{{
					 desc->renderTargetClearValue.r,
					 desc->renderTargetClearValue.g,
					 desc->renderTargetClearValue.b,
					 desc->renderTargetClearValue.a}},
			TheForge_RS_UNDEFINED,
			Render_TextureUsageFlagsToDescriptorType(desc->usageflags),
			desc->debugName,
		};

		TheForge_AddRenderTarget(renderer->renderer, &rtDesc, &texture->renderTarget);
		if(!texture->renderTarget) {
			MEMORY_FREE(texture);
			return nullptr;
		}
		texture->texture = TheForge_RenderTargetGetTexture(texture->renderTarget);

	} else {
		// plain texture
		ASSERT((desc->usageflags & Render_TUF_ROP_READ) == 0);

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
				{{}},
				TheForge_RS_UNDEFINED,
				Render_TextureUsageFlagsToDescriptorType(desc->usageflags),
				nullptr,
				desc->debugName,
				nullptr,
				0,
				0
		};

		TheForge_AddTexture(renderer->renderer, &texDesc, &texture->texture);
		if (!texture->texture) {
			MEMORY_FREE(texture);
			return nullptr;
		}
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
	if(texture->renderTarget) {
		TheForge_RemoveRenderTarget(renderer->renderer, texture->renderTarget);
	} else {
		TheForge_RemoveTexture(renderer->renderer, texture->texture);
	}

	MEMORY_FREE(texture);
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
			true
	};

	TheForge_TextureUpdateDesc updateDesc{
			texture->texture,
			&rid
	};

	TheForge_UpdateTexture(&updateDesc, false);
}


