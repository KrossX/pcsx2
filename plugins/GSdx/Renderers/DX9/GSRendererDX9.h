/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "Renderers/HW/GSRendererHW.h"
#include "Renderers/HW/GSVertexHW.h"
#include "GSTextureCache9.h"

class GSRendererDX9 final : public GSRendererHW
{
	bool m_cfg_logz;
	bool m_cfg_fba;

protected:
	struct
	{
		Direct3DDepthStencilState9 dss;
		Direct3DBlendState9 bs;
	} m_fba;

	inline void ResetStates();
	inline void EmulateAtst(const int pass, const GSTextureCache::Source* tex);
	inline void EmulateZbuffer();
	inline void EmulateTextureSampler(const GSTextureCache::Source* tex);

	inline void EmulateTextureShuffleAndFbmask();
	inline void EmulateChannelShuffle(GSTexture** rt, const GSTextureCache::Source* tex);
	inline void SetupIA(const float& sx, const float& sy);
	inline void UpdateFBA(GSTexture* rt);

	GSDevice9::VSSelector m_vs_sel;
	GSDevice9::GSSelector m_gs_sel;
	GSDevice9::PSSelector m_ps_sel;

	GSDevice9::PSSamplerSelector      m_ps_ssel;
	GSDevice9::OMBlendSelector        m_om_bsel;
	GSDevice9::OMDepthStencilSelector m_om_dssel;

	GSDevice9::PSConstantBuffer ps_cb;
	GSDevice9::VSConstantBuffer vs_cb;
	GSDevice9::GSConstantBuffer gs_cb;

public:
	GSRendererDX9();
	virtual ~GSRendererDX9() {}

	void DrawPrims(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex) final;

	bool CreateDevice(GSDevice* dev);
};
