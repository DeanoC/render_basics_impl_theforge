#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "gfx_theforge/theforge.h"

#include "render_basics/theforge/api.h"
#include "render_basics/api.h"

AL2O3_EXTERN_C Render_BlendStateHandle Render_GetStockBlendState(Render_RendererHandle renderer,
																																 Render_StockBlendStateType stock) {

	if (renderer->stockBlendState[stock] != nullptr) {
		return renderer->stockBlendState[stock];
	}

	TheForge_BlendStateDesc const *blendDesc;
	switch (stock) {

		case Render_SBS_OPAQUE: {
			TheForge_BlendStateDesc const desc{
					{TheForge_BC_ONE},
					{TheForge_BC_ZERO},
					{TheForge_BC_ONE},
					{TheForge_BC_ZERO},
					{TheForge_BM_ADD},
					{TheForge_BM_ADD},
					{0xF},
					TheForge_BST_0,
					false, false
			};
			blendDesc = &desc;
			break;
		}

		case Render_SBS_PORTER_DUFF: {
			TheForge_BlendStateDesc const desc{
					{TheForge_BC_SRC_ALPHA},
					{TheForge_BC_ONE_MINUS_SRC_ALPHA},
					{TheForge_BC_ONE},
					{TheForge_BC_ONE},
					{TheForge_BM_ADD},
					{TheForge_BM_ADD},
					{0xF},
					TheForge_BST_0,
					false, false
			};
			blendDesc = &desc;
			break;
		}
		case Render_SBS_ADDITIVE: {
			TheForge_BlendStateDesc const desc{
					{TheForge_BC_ONE},
					{TheForge_BC_ONE},
					{TheForge_BC_ONE},
					{TheForge_BC_ONE},
					{TheForge_BM_ADD},
					{TheForge_BM_ADD},
					{0xF},
					TheForge_BST_0,
					false, false
			};
			blendDesc = &desc;
			break;
		}
		case Render_SBS_PM_PORTER_DUFF: {
			TheForge_BlendStateDesc const desc{
					{TheForge_BC_ONE},
					{TheForge_BC_ONE_MINUS_SRC_ALPHA},
					{TheForge_BC_ONE},
					{TheForge_BC_ONE},
					{TheForge_BM_ADD},
					{TheForge_BM_ADD},
					{0xF},
					TheForge_BST_0,
					false, false
			};
			blendDesc = &desc;
			break;
		}

		default: ASSERT(false);
			return nullptr;
	}

	TheForge_AddBlendState(renderer->renderer, blendDesc, &renderer->stockBlendState[stock]);
	return renderer->stockBlendState[stock];

}
AL2O3_EXTERN_C Render_DepthStateHandle Render_GetStockDepthState(Render_RendererHandle renderer,
																																 Render_StockDepthStateType stock) {
	if (renderer->stockDepthState[stock] != nullptr) {
		return renderer->stockDepthState[stock];
	}

	TheForge_DepthStateDesc const *depthDesc;
	switch (stock) {
		case Render_SDS_IGNORE: {
			TheForge_DepthStateDesc const desc{
					false, false,
					TheForge_CMP_ALWAYS,
			};
			depthDesc = &desc;
			break;
		}
		case Render_SDS_READONLY_LESS: {
			TheForge_DepthStateDesc const desc{
					true, false,
					TheForge_CMP_LESS,
			};
			depthDesc = &desc;
			break;
		}
		case Render_SDS_READWRITE_LESS: {
			TheForge_DepthStateDesc const desc{
					true, true,
					TheForge_CMP_LESS,
			};
			depthDesc = &desc;
			break;
		}
		case Render_SDS_READONLY_GREATER: {
			TheForge_DepthStateDesc const desc{
					true, false,
					TheForge_CMP_GREATER,
			};
			depthDesc = &desc;
			break;
		}
		case Render_SDS_READWRITE_GREATER: {
			TheForge_DepthStateDesc const desc{
					true, true,
					TheForge_CMP_GREATER,
			};
			depthDesc = &desc;
			break;
		}
		case Render_SDS_WRITEONLY: {
			TheForge_DepthStateDesc const desc{
					false, true,
					TheForge_CMP_ALWAYS,
			};
			depthDesc = &desc;
			break;
		}

		default: ASSERT(false);
			return nullptr;
	}

	TheForge_AddDepthState(renderer->renderer, depthDesc, &renderer->stockDepthState[stock]);
	return renderer->stockDepthState[stock];

}

AL2O3_EXTERN_C Render_RasteriserStateHandle Render_GetStockRasterisationState(Render_RendererHandle renderer,
																																							Render_StockRasterState stock) {
	if (renderer->stockRasterState[stock] != nullptr) {
		return renderer->stockRasterState[stock];
	}

	TheForge_RasterizerStateDesc const *rasterizerStateDesc;
	switch (stock) {
		case Render_SRS_NOCULL: {
			TheForge_RasterizerStateDesc const desc{
					TheForge_CM_NONE,
					0,
					0.0,
					TheForge_FM_SOLID,
					false,
					true,
			};
			rasterizerStateDesc = &desc;
			break;
		}
		case Render_SRS_BACKCULL: {
			TheForge_RasterizerStateDesc const desc{
					TheForge_CM_BACK,
					0,
					0.0,
					TheForge_FM_SOLID,
					false,
					true,
			};
			rasterizerStateDesc = &desc;
			break;
		}

		case Render_SRS_FRONTCULL: {
			TheForge_RasterizerStateDesc const desc{
					TheForge_CM_FRONT,
					0,
					0.0,
					TheForge_FM_SOLID,
					false,
					true,
			};
			rasterizerStateDesc = &desc;
			break;
		}

		default: ASSERT(false);
			return nullptr;
	}

	TheForge_AddRasterizerState(renderer->renderer, rasterizerStateDesc, &renderer->stockRasterState[stock]);
	return renderer->stockRasterState[stock];

}

AL2O3_EXTERN_C Render_SamplerHandle Render_GetStockSampler(Render_RendererHandle renderer,
																													 Render_StockSamplerType stock) {
	if (renderer->stockSamplers[stock] != nullptr) {
		return renderer->stockSamplers[stock];
	}

	TheForge_SamplerDesc const *samplerDesc;
	switch (stock) {
		case Render_SST_POINT: {
			TheForge_SamplerDesc const desc{
					TheForge_FT_NEAREST,
					TheForge_FT_NEAREST,
					TheForge_MM_NEAREST,
					TheForge_AM_CLAMP_TO_EDGE,
					TheForge_AM_CLAMP_TO_EDGE,
					TheForge_AM_CLAMP_TO_EDGE,
			};
			samplerDesc = &desc;
			break;
		}
		case Render_SST_LINEAR: {
			TheForge_SamplerDesc const desc{
					TheForge_FT_LINEAR,
					TheForge_FT_LINEAR,
					TheForge_MM_LINEAR,
					TheForge_AM_CLAMP_TO_EDGE,
					TheForge_AM_CLAMP_TO_EDGE,
					TheForge_AM_CLAMP_TO_EDGE,
			};
			samplerDesc = &desc;
			break;
		}
		default: ASSERT(false);
			return nullptr;
	}

	TheForge_AddSampler(renderer->renderer, samplerDesc, &renderer->stockSamplers[stock]);
	return renderer->stockSamplers[stock];
}


AL2O3_EXTERN_C Render_VertexLayoutHandle Render_GetStockVertexLayout(Render_RendererHandle renderer,
																																		 Render_StockVertexLayouts stock) {
	if (renderer->stockVertexLayouts[stock] != nullptr) {
		return renderer->stockVertexLayouts[stock];
	}

	static TheForge_VertexLayout const vertexLayout2DPackedColour{
			2,
			{
					{TheForge_SS_POSITION, 8, "POSITION", TinyImageFormat_R32G32_SFLOAT, 0, 0, 0},
					{TheForge_SS_COLOR, 5, "COLOR", TinyImageFormat_R8G8B8A8_UNORM, 0, 1, sizeof(float) * 2}
			}
	};

	static TheForge_VertexLayout const vertexLayout2DFloatColour{
			2,
			{
					{TheForge_SS_POSITION, 8, "POSITION", TinyImageFormat_R32G32_SFLOAT, 0, 0, 0},
					{TheForge_SS_COLOR, 5, "COLOR", TinyImageFormat_R32G32B32A32_SFLOAT, 0, 1, sizeof(float) * 2}
			}
	};

	static TheForge_VertexLayout const vertexLayout2DPackedColourUV{
			3,
			{
					{TheForge_SS_POSITION, 8, "POSITION", TinyImageFormat_R32G32_SFLOAT, 0, 0, 0},
					{TheForge_SS_TEXCOORD0, 9, "TEXCOORD", TinyImageFormat_R32G32_SFLOAT, 0, 1, sizeof(float) * 2},
					{TheForge_SS_COLOR, 5, "COLOR", TinyImageFormat_R8G8B8A8_UNORM, 0, 2, sizeof(float) * 4}
			}
	};
	static TheForge_VertexLayout const vertexLayout2DFloatColourUV{
			3,
			{
					{TheForge_SS_POSITION, 8, "POSITION", TinyImageFormat_R32G32_SFLOAT, 0, 0, 0},
					{TheForge_SS_TEXCOORD0, 9, "TEXCOORD", TinyImageFormat_R32G32_SFLOAT, 0, 1, sizeof(float) * 2},
					{TheForge_SS_COLOR, 5, "COLOR", TinyImageFormat_R32G32B32A32_SFLOAT, 0, 2, sizeof(float) * 4}
			}
	};

	static TheForge_VertexLayout const vertexLayout3DPackedColour{
			2,
			{
					{TheForge_SS_POSITION, 8, "POSITION", TinyImageFormat_R32G32B32_SFLOAT, 0, 0, 0},
					{TheForge_SS_COLOR, 5, "COLOR", TinyImageFormat_R8G8B8A8_UNORM, 0, 1, sizeof(float) * 3}
			}
	};

	static TheForge_VertexLayout const vertexLayout3DFloatColour{
			2,
			{
					{TheForge_SS_POSITION, 8, "POSITION", TinyImageFormat_R32G32B32_SFLOAT, 0, 0, 0},
					{TheForge_SS_COLOR, 5, "COLOR", TinyImageFormat_R32G32B32A32_SFLOAT, 0, 1, sizeof(float) * 3}
			}
	};

	static TheForge_VertexLayout const vertexLayout3DPackedColourUV{
			3,
			{
					{TheForge_SS_POSITION, 8, "POSITION", TinyImageFormat_R32G32B32_SFLOAT, 0, 0, 0},
					{TheForge_SS_TEXCOORD0, 9, "TEXCOORD", TinyImageFormat_R32G32_SFLOAT, 0, 1, sizeof(float) * 3},
					{TheForge_SS_COLOR, 5, "COLOR", TinyImageFormat_R8G8B8A8_UNORM, 0, 2, sizeof(float) * 5}
			}
	};
	static TheForge_VertexLayout const vertexLayout3DFloatColourUV{
			3,
			{
					{TheForge_SS_POSITION, 8, "POSITION", TinyImageFormat_R32G32B32_SFLOAT, 0, 0, 0},
					{TheForge_SS_TEXCOORD0, 9, "TEXCOORD", TinyImageFormat_R32G32_SFLOAT, 0, 1, sizeof(float) * 3},
					{TheForge_SS_COLOR, 5, "COLOR", TinyImageFormat_R32G32B32A32_SFLOAT, 0, 2, sizeof(float) * 5}
			}
	};

	static TheForge_VertexLayout const vertexLayout3DNormalPackedColour{
			3,
			{
					{TheForge_SS_POSITION, 8, "POSITION", TinyImageFormat_R32G32B32_SFLOAT, 0, 0, 0},
					{TheForge_SS_NORMAL, 9, "NORMAL", TinyImageFormat_R32G32B32_SFLOAT, 0, 1, sizeof(float) * 3},
					{TheForge_SS_COLOR, 5, "COLOR", TinyImageFormat_R8G8B8A8_UNORM, 0, 2, sizeof(float) * 6}
			}
	};

	switch (stock) {
		case Render_SVL_2D_COLOUR: renderer->stockVertexLayouts[stock] = &vertexLayout2DPackedColour;
			break;
		case Render_SVL_2D_FLOATCOLOUR: renderer->stockVertexLayouts[stock] = &vertexLayout2DFloatColour;
			break;
		case Render_SVL_2D_COLOUR_UV: renderer->stockVertexLayouts[stock] = &vertexLayout2DPackedColourUV;
			break;
		case Render_SVL_2D_FLOATCOLOUR_UV: renderer->stockVertexLayouts[stock] = &vertexLayout2DFloatColourUV;
			break;
		case Render_SVL_3D_COLOUR: renderer->stockVertexLayouts[stock] = &vertexLayout3DPackedColour;
			break;
		case Render_SVL_3D_FLOATCOLOUR: renderer->stockVertexLayouts[stock] = &vertexLayout3DFloatColour;
			break;
		case Render_SVL_3D_COLOUR_UV: renderer->stockVertexLayouts[stock] = &vertexLayout3DPackedColourUV;
			break;
		case Render_SVL_3D_FLOATCOLOUR_UV: renderer->stockVertexLayouts[stock] = &vertexLayout3DFloatColourUV;
			break;
		case Render_SVL_3D_NORMAL_COLOUR: renderer->stockVertexLayouts[stock] = &vertexLayout3DNormalPackedColour;
			break;

		case Render_SVL_COUNT: ASSERT(false);
			return nullptr;
	}

	return renderer->stockVertexLayouts[stock];

}