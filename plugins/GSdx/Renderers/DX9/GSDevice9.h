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

#include "GSTexture9.h"
#include "GSVector.h"
#include "Renderers/Common/GSDevice.h"

struct Direct3DSamplerState9
{
    D3DTEXTUREFILTERTYPE FilterMin[2];
    D3DTEXTUREFILTERTYPE FilterMag[2];
	D3DTEXTUREFILTERTYPE FilterMip[2];
	D3DTEXTUREFILTERTYPE Anisotropic[2];
    D3DTEXTUREADDRESS AddressU;
    D3DTEXTUREADDRESS AddressV;
	D3DTEXTUREADDRESS AddressW;
	DWORD MaxAnisotropy;
	DWORD MaxLOD;
};

struct Direct3DDepthStencilState9
{
    BOOL DepthEnable;
    BOOL DepthWriteMask;
    D3DCMPFUNC DepthFunc;
    BOOL StencilEnable;
    UINT8 StencilReadMask;
    UINT8 StencilWriteMask;
    D3DSTENCILOP StencilFailOp;
    D3DSTENCILOP StencilDepthFailOp;
    D3DSTENCILOP StencilPassOp;
    D3DCMPFUNC StencilFunc;
	uint32 StencilRef;
};

struct Direct3DBlendState9
{
    BOOL BlendEnable;
    D3DBLEND SrcBlend;
    D3DBLEND DestBlend;
    D3DBLENDOP BlendOp;
    D3DBLEND SrcBlendAlpha;
    D3DBLEND DestBlendAlpha;
    D3DBLENDOP BlendOpAlpha;
    UINT8 RenderTargetWriteMask;
};

struct GSVertexShader9
{
	CComPtr<IDirect3DVertexShader9> vs;
	CComPtr<IDirect3DVertexDeclaration9> il;
};

class GSDevice9 : public GSDevice
{
public:
	#pragma pack(push, 1)

	struct alignas(32) VSConstantBuffer
	{
		GSVector4 VertexScale;
		GSVector4 VertexOffset;
		GSVector4 Texture_Scale_Offset;

		struct VSConstantBuffer()
		{
			VertexScale = GSVector4::zero();
			VertexOffset = GSVector4::zero();
			Texture_Scale_Offset = GSVector4::zero();
		}

		__forceinline bool Update(const VSConstantBuffer* cb)
		{
			GSVector4i* a = (GSVector4i*)this;
			GSVector4i* b = (GSVector4i*)cb;

			if(!((a[0] == b[0]) & (a[1] == b[1]) & (a[2] == b[2]) & (a[3] == b[3])).alltrue())
			{
				a[0] = b[0];
				a[1] = b[1];
				a[2] = b[2];
				a[3] = b[3];

				return true;
			}

			return false;
		}
	};

	struct VSSelector
	{
		union
		{
			struct
			{
				uint32 bppz:2;
				uint32 tme:1;
				uint32 fst:1;
				uint32 logz:1;
				uint32 rtcopy:1;
			};

			uint32 key;
		};

		operator uint32() {return key & 0xff;}

		VSSelector() : key(0) {}
	};

	struct alignas(32) PSConstantBuffer
	{
		GSVector4 FogColor_AREF;
		GSVector4 HalfTexel;
		GSVector4 WH;
		GSVector4 MinMax;
		GSVector4 MinF_TA;
		GSVector4i MskFix;
		GSVector4i ChannelShuffle;

		GSVector4 TC_OffsetHack;

		struct PSConstantBuffer()
		{
			FogColor_AREF = GSVector4::zero();
			HalfTexel = GSVector4::zero();
			WH = GSVector4::zero();
			MinMax = GSVector4::zero();
			MinF_TA = GSVector4::zero();
			MskFix = GSVector4i::zero();
			ChannelShuffle = GSVector4i::zero();
		}

		__forceinline bool Update(const PSConstantBuffer* cb)
		{
			GSVector4i* a = (GSVector4i*)this;
			GSVector4i* b = (GSVector4i*)cb;

			if(!((a[0] == b[0]) /*& (a[1] == b1)*/ & (a[2] == b[2]) & (a[3] == b[3]) & (a[4] == b[4]) & (a[5] == b[5]) & (a[6] == b[6])).alltrue()) // if WH matches HalfTexel does too
			{
				a[0] = b[0];
				a[1] = b[1];
				a[2] = b[2];
				a[3] = b[3];
				a[4] = b[4];
				a[5] = b[5];
				a[6] = b[6];

				return true;
			}

			return false;
		}
	};

	struct alignas(32) GSConstantBuffer
	{
		GSVector2 PointSize;

		struct GSConstantBuffer()
		{
			PointSize = GSVector2(0);
		}

		__forceinline bool Update(const GSConstantBuffer* cb)
		{
			return true;
		}
	};

	struct GSSelector
	{
		union
		{
			struct
			{
				uint32 iip:1;
				uint32 prim:2;
				uint32 point:1;
				uint32 line:1;

				uint32 _free:27;
			};

			uint32 key;
		};

		operator uint32() {return key;}

		GSSelector() : key(0) {}
		GSSelector(uint32 k) : key(k) {}
	};

	struct PSSelector
	{
		union
		{
			struct
			{
				// *** Word 1
				// Format
				uint32 fmt:4;
				uint32 dfmt:2;
				uint32 depth_fmt:2;
				// Alpha extension/Correction
				uint32 aem:1;
				uint32 fba:1;
				// Fog
				uint32 fog:1;
				// Pixel test
				uint32 date:2;
				uint32 atst:3;
				// Color sampling
				uint32 fst:1;
				uint32 tfx:3;
				uint32 tcc:1;
				uint32 wms:2;
				uint32 wmt:2;
				uint32 ltf:1;
				// Shuffle and fbmask effect
				uint32 shuffle:1;
				uint32 read_ba:1;

				// *** Word 2
				// Blend and Colclip
				uint32 clr1:1;
				uint32 rt:1;
				uint32 colclip:2;

				// Others ways to fetch the texture
				uint32 channel:3;

				// Hack
				uint32 aout:1;
				uint32 spritehack:1;
				uint32 tcoffsethack:1;
				uint32 urban_chaos_hle:1;
				uint32 tales_of_abyss_hle:1;
				uint32 point_sampler:1;

				uint32 _free:23;
			};

			uint64 key;
		};

		operator uint64() {return key;}

		PSSelector() : key(0) {}
	};

	struct PSSamplerSelector
	{
		union
		{
			struct
			{
				uint32 tau:1;
				uint32 tav:1;
				uint32 ltf:1;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x7;}

		PSSamplerSelector() : key(0) {}
	};

	struct OMDepthStencilSelector
	{
		union
		{
			struct
			{
				uint32 ztst:2;
				uint32 zwe:1;
				uint32 date:1;
				uint32 fba:1;
				uint32 date_one:1;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x3f;}

		OMDepthStencilSelector() : key(0) {}
	};

	struct OMBlendSelector
	{
		union
		{
			struct
			{
				uint32 abe:1;
				uint32 a:2;
				uint32 b:2;
				uint32 c:2;
				uint32 d:2;
				uint32 wr:1;
				uint32 wg:1;
				uint32 wb:1;
				uint32 wa:1;
				uint32 negative:1;
			};

			struct
			{
				uint32 _pad:1;
				uint32 abcd:8;
				uint32 wrgba:4;
			};

			uint32 key;
		};

		operator uint32() {return key & 0x3fff;}

		OMBlendSelector() : key(0) {}

		bool IsCLR1() const
		{
			return (key & 0x19f) == 0x93; // abe == 1 && a == 1 && b == 2 && d == 1
		}
	};

	#pragma pack(pop)

	class ShaderMacro
	{
		struct mcstr
		{
			const char* name, * def;
			mcstr(const char* n, const char* d) : name(n), def(d) {}
		};

		struct mstring
		{
			std::string name, def;
			mstring(const char* n, std::string d) : name(n), def(d) {}
		};

		std::vector<mstring> mlist;
		std::vector<mcstr> mout;

	public:
		ShaderMacro(std::string& smodel);
		void AddMacro(const char* n, int d);
		D3D_SHADER_MACRO* GetPtr(void);
	};

private:
	GSTexture* CreateSurface(int type, int w, int h, int format, bool msaa = false);
	GSTexture* FetchSurface(int type, int w, int h, int format, bool msaa = false);

	void DoMerge(GSTexture* sTex[3], GSVector4* sRect, GSTexture* dTex, GSVector4* dRect, const GSRegPMODE& PMODE, const GSRegEXTBUF& EXTBUF, const GSVector4& c);
	void DoInterlace(GSTexture* sTex, GSTexture* dTex, int shader, bool linear, float yoffset = 0);
	void DoFXAA(GSTexture* sTex, GSTexture* dTex);
	void DoShadeBoost(GSTexture* sTex, GSTexture* dTex);
	void DoExternalFX(GSTexture* sTex, GSTexture* dTex);

	void InitExternalFX();
	void InitFXAA();

	uint16 ConvertBlendEnum(uint16 generic) final;

	//

	D3DCAPS9 m_d3dcaps;
	D3DPRESENT_PARAMETERS m_pp;
	CComPtr<IDirect3D9> m_d3d;
	CComPtr<IDirect3DDevice9> m_dev;
	CComPtr<IDirect3DSwapChain9> m_swapchain;
	CComPtr<IDirect3DVertexBuffer9> m_vb;
	CComPtr<IDirect3DVertexBuffer9> m_vb_old;
	CComPtr<IDirect3DIndexBuffer9> m_ib;
	CComPtr<IDirect3DIndexBuffer9> m_ib_old;
	bool m_lost;
	D3DFORMAT m_depth_format;

	struct {D3D_FEATURE_LEVEL level; std::string model, vs, gs, ps, cs;} m_shader;
	int m_upscale_multiplier;
	int m_mipmap;

	uint32 m_msaa;
	DXGI_SAMPLE_DESC m_msaa_desc;

	struct
	{
		IDirect3DVertexBuffer9* vb;
		size_t vb_stride;
		IDirect3DIndexBuffer9* ib;
		IDirect3DVertexDeclaration9* layout;
		D3DPRIMITIVETYPE topology;
		IDirect3DVertexShader9* vs;
		float* vs_cb;
		int vs_cb_len;
		IDirect3DTexture9* ps_srvs[3];
		IDirect3DPixelShader9* ps;
		float* ps_cb;
		int ps_cb_len;
		Direct3DSamplerState9* ps_ss;
		GSVector4i scissor;
		Direct3DDepthStencilState9* dss;
		Direct3DBlendState9* bs;
		uint32 bf;
		IDirect3DSurface9* rtv;
		IDirect3DSurface9* dsv;
	} m_state;

public: // TODO

	bool FXAA_Compiled;
	bool ExShader_Compiled;

	struct
	{
		CComPtr<IDirect3DVertexDeclaration9> il;
		CComPtr<IDirect3DVertexShader9> vs;
		CComPtr<IDirect3DPixelShader9> ps[10];
		Direct3DSamplerState9 ln;
		Direct3DSamplerState9 pt;
		Direct3DDepthStencilState9 dss;
		Direct3DBlendState9 bs;
	} m_convert;

	struct
	{
		CComPtr<IDirect3DPixelShader9> ps[2];
		Direct3DBlendState9 bs;
	} m_merge;

	struct
	{
		CComPtr<IDirect3DPixelShader9> ps[4];
	} m_interlace;

	struct
	{
		CComPtr<IDirect3DPixelShader9> ps;
	} m_shaderfx;

	struct
	{
		CComPtr<IDirect3DPixelShader9> ps;
	} m_fxaa;

	struct
	{
		CComPtr<IDirect3DPixelShader9> ps;
	} m_shadeboost;

	struct
	{
		Direct3DDepthStencilState9 dss;
		Direct3DBlendState9 bs;
	} m_date;

	void SetupDATE(GSTexture* rt, GSTexture* ds, const GSVertexPT1* vertices, bool datm);

	// Shaders...

	std::unordered_map<uint32, GSVertexShader9> m_vs;
	std::unordered_map<uint64, CComPtr<IDirect3DPixelShader9>> m_ps;
	std::unordered_map<uint32, Direct3DSamplerState9*> m_ps_ss;
	std::unordered_map<uint32, Direct3DDepthStencilState9*> m_om_dss;
	std::unordered_map<uint32, Direct3DBlendState9*> m_om_bs;
	std::unordered_map<uint32, GSTexture*> m_mskfix;

	GSTexture* CreateMskFix(uint32 size, uint32 msk, uint32 fix);

public:
	GSDevice9();
	virtual ~GSDevice9();

	bool SetFeatureLevel(D3D_FEATURE_LEVEL level, bool compat_mode);
	void GetFeatureLevel(D3D_FEATURE_LEVEL& level) const {level = m_shader.level;}

	bool Create(const std::shared_ptr<GSWnd> &wnd);
	bool Reset(int w, int h);
	bool IsLost(bool update);
	void Flip();

	void SetVSync(int vsync);

	void BeginScene();
	void DrawPrimitive();
	void DrawIndexedPrimitive();
	void EndScene();

	void ClearRenderTarget(GSTexture* t, const GSVector4& c);
	void ClearRenderTarget(GSTexture* t, uint32 c);
	void ClearDepth(GSTexture* t);
	void ClearStencil(GSTexture* t, uint8 c);

	GSTexture* CreateRenderTarget(int w, int h, int format = 0, bool msaa = false);
	GSTexture* CreateDepthStencil(int w, int h, int format = 0, bool msaa = false);
	GSTexture* CreateTexture(int w, int h, int format = 0);
	GSTexture* CreateOffscreen(int w, int h, int format = 0);

	GSTexture* Resolve(GSTexture* t);

	GSTexture* CopyOffscreen(GSTexture* src, const GSVector4& sRect, int w, int h, int format = 0, int ps_shader = 0);

	void CopyRect(GSTexture* sTex, GSTexture* dTex, const GSVector4i& r);

	void StretchRect(GSTexture* sTex, const GSVector4& sRect, GSTexture* dTex, const GSVector4& dRect, int shader = 0, bool linear = true);
	void StretchRect(GSTexture* sTex, const GSVector4& sRect, GSTexture* dTex, const GSVector4& dRect, IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len, bool linear = true);
	void StretchRect(GSTexture* sTex, const GSVector4& sRect, GSTexture* dTex, const GSVector4& dRect, IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len, Direct3DBlendState9* bs, bool linear = true);

	void IASetVertexBuffer(const void* vertex, size_t stride, size_t count);
	bool IAMapVertexBuffer(void** vertex, size_t stride, size_t count);
	void IAUnmapVertexBuffer();
	void IASetVertexBuffer(IDirect3DVertexBuffer9* vb, size_t stride);
	void IASetIndexBuffer(const void* index, size_t count);
	void IASetIndexBuffer(IDirect3DIndexBuffer9* ib);
	void IASetInputLayout(IDirect3DVertexDeclaration9* layout);
	void IASetPrimitiveTopology(D3DPRIMITIVETYPE topology);
	void VSSetShader(IDirect3DVertexShader9* vs, const float* vs_cb, int vs_cb_len);
	void PSSetShaderResources(GSTexture* sr0, GSTexture* sr1);
	void PSSetShaderResource(int i, GSTexture* sr);
	void PSSetShader(IDirect3DPixelShader9* ps, const float* ps_cb, int ps_cb_len);
	void PSSetSamplerState(Direct3DSamplerState9* ss);
	void OMSetDepthStencilState(Direct3DDepthStencilState9* dss);
	void OMSetBlendState(Direct3DBlendState9* bs, uint32 bf);
	void OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor = NULL);

	IDirect3DDevice9* operator->() {return m_dev;}
	operator IDirect3DDevice9*() {return m_dev;}

	void CompileShader(const char *source, size_t size, const char *filename, const std::string& entry, const D3D_SHADER_MACRO* macro, IDirect3DVertexShader9** vs, const D3DVERTEXELEMENT9* layout, int count, IDirect3DVertexDeclaration9** il);
	void CompileShader(const char *source, size_t size, const char *filename, const std::string& entry, const D3D_SHADER_MACRO* macro, IDirect3DPixelShader9** ps);

	void SetupVS(VSSelector sel, const VSConstantBuffer* cb);
	void SetupGS(GSSelector sel, const GSConstantBuffer* cb) {}
	void SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel);
	void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 afix);

	bool HasStencil() { return m_depth_format == D3DFMT_D24S8; }
	bool HasDepth32() { return m_depth_format != D3DFMT_D24S8; }

	static uint32 GetMaxDepth(uint32 msaaCount = 0, std::string adapter_id = "");
	static void ForceValidMsaaConfig();
};

