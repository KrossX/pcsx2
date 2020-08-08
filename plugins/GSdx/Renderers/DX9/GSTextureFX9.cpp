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

#include "stdafx.h"
#include "GSDevice9.h"
#include "resource.h"
#include "GSTables.h"

void GSDevice9::UpdateDithering(GIFRegDIMX *DIMX)
{
	uint32_t *a = (uint32_t*)&m_dither_cache;
	uint32_t *b = (uint32_t*)DIMX;

	if (a[0] != b[0] || a[1] != b[1]) {
		a[0] = b[0];
		a[1] = b[1];

		if (!m_dither_tex) {
			m_dither_tex = CreateTexture(4, 4, D3DFMT_L8);
			m_dev->SetTexture(5, *(GSTexture9*)m_dither_tex);

			m_dev->SetSamplerState(5, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			m_dev->SetSamplerState(5, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			m_dev->SetSamplerState(5, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
			m_dev->SetSamplerState(5, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
			m_dev->SetSamplerState(5, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
			m_dev->SetSamplerState(5, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
			m_dev->SetSamplerState(5, D3DSAMP_MAXANISOTROPY, 0);
			m_dev->SetSamplerState(5, D3DSAMP_MAXMIPLEVEL, 0);
		}

		if (m_dither_tex) {
			GSTexture::GSMap m;

			if (m_dither_tex->Map(m)) {
				m.bits[0] = (uint8)(4 + DIMX->DM00);
				m.bits[1] = (uint8)(4 + DIMX->DM10);
				m.bits[2] = (uint8)(4 + DIMX->DM20);
				m.bits[3] = (uint8)(4 + DIMX->DM30);

				m.bits[4] = (uint8)(4 + DIMX->DM01);
				m.bits[5] = (uint8)(4 + DIMX->DM11);
				m.bits[6] = (uint8)(4 + DIMX->DM21);
				m.bits[7] = (uint8)(4 + DIMX->DM31);

				m.bits[ 8] = (uint8)(4 + DIMX->DM02);
				m.bits[ 9] = (uint8)(4 + DIMX->DM12);
				m.bits[10] = (uint8)(4 + DIMX->DM22);
				m.bits[11] = (uint8)(4 + DIMX->DM32);

				m.bits[12] = (uint8)(4 + DIMX->DM03);
				m.bits[13] = (uint8)(4 + DIMX->DM13);
				m.bits[14] = (uint8)(4 + DIMX->DM23);
				m.bits[15] = (uint8)(4 + DIMX->DM33);

				m_dither_tex->Unmap();
			}
		}
	}
}

GSTexture* GSDevice9::CreateMskFix(uint32 size, uint32 msk, uint32 fix)
{
	GSTexture* t = NULL;

	uint32 hash = (size << 20) | (msk << 10) | fix;

	auto i = m_mskfix.find(hash);

	if(i != m_mskfix.end())
	{
		t = i->second;
	}
	else
	{
		t = CreateTexture(size, 1, D3DFMT_R32F);

		if(t)
		{
			GSTexture::GSMap m;

			if(t->Map(m))
			{
				for(uint32 i = 0; i < size; i++)
				{
					((float*)m.bits)[i] = (float)((i & msk) | fix) / size;
				}

				t->Unmap();
			}

			m_mskfix[hash] = t;
		}
	}

	return t;
}

void GSDevice9::SetupVS(VSSelector sel, const VSConstantBuffer* cb)
{
	auto i = std::as_const(m_vs).find(sel);

	if(i == m_vs.end())
	{
		ShaderMacro sm(m_shader.model);

		sm.AddMacro("VS_BPPZ", sel.bppz);
		sm.AddMacro("VS_TME", sel.tme);
		sm.AddMacro("VS_FST", sel.fst);
		sm.AddMacro("VS_LOGZ", sel.logz);
		sm.AddMacro("VS_RTCOPY", sel.rtcopy);

		static const D3DVERTEXELEMENT9 layout[] =
		{
			{0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
			{0, 8, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
			{0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1},
			{0, 16,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
			D3DDECL_END()
		};

		GSVertexShader9 vs;

		std::vector<char> shader;
		theApp.LoadResource(IDR_TFX_FX9, shader);
		CompileShader(shader.data(), shader.size(), "tfx.fx", "vs_main", sm.GetPtr(), &vs.vs, layout, countof(layout), &vs.il);

		m_vs[sel] = vs;

		i = m_vs.find(sel);
	}

	VSSetShader(i->second.vs, (const float*)cb, sizeof(*cb) / sizeof(GSVector4));

	IASetInputLayout(i->second.il);
}

void GSDevice9::SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel)
{
	if(cb->WH.z > 0 && cb->WH.w > 0 && (sel.wms == 3 || sel.wmt == 3))
	{
		GSVector4i size(cb->WH);

		if(sel.wms == 3)
		{
			if(GSTexture* t = CreateMskFix(size.z, cb->MskFix.x, cb->MskFix.z))
			{
				m_dev->SetTexture(3, *(GSTexture9*)t);
			}
		}

		if(sel.wmt == 3)
		{
			if(GSTexture* t = CreateMskFix(size.w, cb->MskFix.y, cb->MskFix.w))
			{
				m_dev->SetTexture(4, *(GSTexture9*)t);
			}
		}
	}

	auto i = std::as_const(m_ps).find(sel);

	if(i == m_ps.end())
	{
		ShaderMacro sm(m_shader.model);

		sm.AddMacro("PS_SCALE_FACTOR", std::max(1, m_upscale_multiplier));
		sm.AddMacro("PS_FST", sel.fst);
		sm.AddMacro("PS_WMS", sel.wms);
		sm.AddMacro("PS_WMT", sel.wmt);
		sm.AddMacro("PS_FMT", sel.fmt);
		sm.AddMacro("PS_AEM", sel.aem);
		sm.AddMacro("PS_TFX", sel.tfx);
		sm.AddMacro("PS_TCC", sel.tcc);
		sm.AddMacro("PS_ATST", sel.atst);
		sm.AddMacro("PS_FOG", sel.fog);
		sm.AddMacro("PS_CLR1", sel.clr1);
		sm.AddMacro("PS_RT", sel.rt);
		sm.AddMacro("PS_LTF", sel.ltf);
		sm.AddMacro("PS_COLCLIP", sel.colclip);
		sm.AddMacro("PS_DATE", sel.date);
		sm.AddMacro("PS_TCOFFSETHACK", sel.tcoffsethack);
		sm.AddMacro("PS_PAL_FMT", sel.fmt >> 2);
		sm.AddMacro("PS_DITHER", sel.dither);
		sm.AddMacro("PS_ZCLAMP", sel.zclamp);

		CComPtr<IDirect3DPixelShader9> ps;

		std::vector<char> shader;
		theApp.LoadResource(IDR_TFX_FX9, shader);
		CompileShader(shader.data(), shader.size(), "tfx.fx", "ps_main", sm.GetPtr(), &ps);

		m_ps[sel] = ps;

		i = m_ps.find(sel);
	}

	PSSetShader(i->second, (const float*)cb, sizeof(*cb) / sizeof(GSVector4));

	Direct3DSamplerState9* ss = NULL;

	if(sel.tfx != 4)
	{
		if(!(sel.fmt < 3 && sel.wms < 3 && sel.wmt < 3))
		{
			ssel.ltf = 0;
		}

		auto i = std::as_const(m_ps_ss).find(ssel);

		if(i != m_ps_ss.end())
		{
			ss = i->second;
		}
		else
		{
			ss = new Direct3DSamplerState9();

			memset(ss, 0, sizeof(*ss));

			D3DTEXTUREFILTERTYPE aniso = m_aniso ? D3DTEXF_ANISOTROPIC : D3DTEXF_LINEAR;

			ss->FilterMin[0] = ssel.ltf ? aniso : D3DTEXF_POINT;
			ss->FilterMag[0] = ssel.ltf ? aniso : D3DTEXF_POINT;
			ss->FilterMip[0] = ssel.ltf ? aniso : D3DTEXF_POINT;
			ss->FilterMin[1] = D3DTEXF_POINT;
			ss->FilterMag[1] = D3DTEXF_POINT;
			ss->FilterMip[1] = D3DTEXF_POINT;
			ss->AddressU = ssel.tau ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
			ss->AddressV = ssel.tav ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
			ss->MaxAnisotropy = m_aniso;
			ss->MaxLOD = ULONG_MAX;

			m_ps_ss[ssel] = ss;
		}
	}

	PSSetSamplerState(ss);
}

void GSDevice9::SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 afix)
{
	Direct3DDepthStencilState9* dss = NULL;

	auto i = std::as_const(m_om_dss).find(dssel);

	if(i == m_om_dss.end())
	{
		dss = new Direct3DDepthStencilState9();

		memset(dss, 0, sizeof(*dss));

		if(dssel.date || dssel.fba)
		{
			dss->StencilEnable = true;
			dss->StencilReadMask = 1;
			dss->StencilWriteMask = dssel.date_one ? 3 : 2;
			dss->StencilFunc = dssel.date ? D3DCMP_EQUAL : D3DCMP_ALWAYS;
			dss->StencilPassOp = dssel.date_one ? D3DSTENCILOP_ZERO : dssel.fba ? D3DSTENCILOP_REPLACE : D3DSTENCILOP_KEEP;
			dss->StencilFailOp = dssel.fba && !dssel.date_one ? D3DSTENCILOP_ZERO : D3DSTENCILOP_KEEP;
			dss->StencilDepthFailOp = D3DSTENCILOP_KEEP;
			dss->StencilRef = 3;
		}

		if(dssel.ztst != ZTST_ALWAYS || dssel.zwe)
		{
			static const D3DCMPFUNC ztst[] =
			{
				D3DCMP_NEVER,
				D3DCMP_ALWAYS,
				D3DCMP_GREATEREQUAL,
				D3DCMP_GREATER
			};

			dss->DepthEnable = true;
			dss->DepthWriteMask = dssel.zwe;
			dss->DepthFunc = ztst[dssel.ztst];
		}

		m_om_dss[dssel] = dss;

		i = m_om_dss.find(dssel);
	}

	OMSetDepthStencilState(i->second);

	auto j = std::as_const(m_om_bs).find(bsel);

	if(j == m_om_bs.end())
	{
		Direct3DBlendState9* bs = new Direct3DBlendState9();

		memset(bs, 0, sizeof(*bs));

		bs->BlendEnable = bsel.abe;

		if(bsel.abe)
		{
			int i = ((bsel.a * 3 + bsel.b) * 3 + bsel.c) * 3 + bsel.d;

			HWBlend blend = GetBlend(i);
			bs->BlendOp = (D3DBLENDOP)blend.op;
			bs->SrcBlend = (D3DBLEND)blend.src;
			bs->DestBlend = (D3DBLEND)blend.dst;
			bs->BlendOpAlpha = D3DBLENDOP_ADD;
			bs->SrcBlendAlpha = D3DBLEND_ONE;
			bs->DestBlendAlpha = D3DBLEND_ZERO;

			// Not very good but I don't wanna write another 81 row table

			if(bsel.negative)
			{
				if(bs->BlendOp == D3DBLENDOP_ADD)
				{
					bs->BlendOp = D3DBLENDOP_REVSUBTRACT;
				}
				else if(bs->BlendOp == D3DBLENDOP_REVSUBTRACT)
				{
					bs->BlendOp = D3DBLENDOP_ADD;
				}
				else
					; // god knows, best just not to mess with it for now
			}
		}

		// this is not a typo; dx9 uses BGRA rather than the gs native RGBA, unlike dx10

		if(bsel.wr) bs->RenderTargetWriteMask |= D3DCOLORWRITEENABLE_BLUE;
		if(bsel.wg) bs->RenderTargetWriteMask |= D3DCOLORWRITEENABLE_GREEN;
		if(bsel.wb) bs->RenderTargetWriteMask |= D3DCOLORWRITEENABLE_RED;
		if(bsel.wa) bs->RenderTargetWriteMask |= D3DCOLORWRITEENABLE_ALPHA;

		m_om_bs[bsel] = bs;

		j = m_om_bs.find(bsel);
	}

	OMSetBlendState(j->second, afix >= 0x80 ? 0xffffff : 0x020202 * afix);
}
