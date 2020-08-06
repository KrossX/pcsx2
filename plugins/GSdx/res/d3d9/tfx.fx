#ifdef SHADER_MODEL // make safe to include in resource file to enforce dependency
#define FMT_32 0
#define FMT_24 1
#define FMT_16 2

#ifndef VS_BPPZ
#define VS_BPPZ 0
#define VS_TME 1
#define VS_FST 1
#define VS_LOGZ 1
#endif

#ifndef PS_FST
#define PS_FST 0
#define PS_WMS 0
#define PS_WMT 0
#define PS_FMT FMT_32
#define PS_AEM 0
#define PS_TFX 0
#define PS_TCC 0
#define PS_ATST 4
#define PS_FOG 0
#define PS_CLR1 0
#define PS_RT 0
#define PS_LTF 0
#define PS_COLCLIP 0
#define PS_DATE 0
#define PS_PAL_FMT 0
#endif

struct VS_INPUT
{
	float4 p : POSITION0;
	float2 t : TEXCOORD0;
	float4 c : COLOR0;
	float4 f : COLOR1;
};

struct VS_OUTPUT
{
	float4 p : POSITION;
	float4 t : TEXCOORD0;
#if VS_RTCOPY
	float4 tp : TEXCOORD1;
#endif
	float4 c : COLOR0;
};

struct PS_INPUT
{
	float4 t : TEXCOORD0;
#if PS_DATE > 0
	float4 tp : TEXCOORD1;
#endif
	float4 c : COLOR0;
};

sampler Texture : register(s0);
sampler Palette : register(s1);
sampler RTCopy : register(s2);
sampler1D UMSKFIX : register(s3);
sampler1D VMSKFIX : register(s4);

float4 vs_params[3];

#define VertexScale vs_params[0]
#define VertexOffset vs_params[1]
#define Texture_Scale_Offset vs_params[2]

float4 ps_params[7];

#define FogColor	ps_params[0].bgr
#define AREF		ps_params[0].a
#define HalfTexel	ps_params[1]
#define WH			ps_params[2]
#define MinMax		ps_params[3]
#define MinF		ps_params[4].xy
#define TA			ps_params[4].zw

#define TC_OffsetHack ps_params[6]

float4 sample_c(float2 uv)
{
	return tex2D(Texture, uv);
}

float4 sample_p(float u)
{
	return tex2D(Palette, u);
}

float4 sample_rt(float2 uv)
{
	return tex2D(RTCopy, uv);
}

#define PS_AEM_FMT (PS_FMT & 3)

float4 clamp_wrap_uv(float4 uv)
{
	if(PS_WMS == PS_WMT)
	{
/*
		if(PS_WMS == 0)
		{
			uv = frac(uv);
		}
		else if(PS_WMS == 1)
		{
			uv = saturate(uv);
		}
		else
*/
		if(PS_WMS == 2)
		{
			uv = clamp(uv, MinMax.xyxy, MinMax.zwzw);
		}
		else if(PS_WMS == 3)
		{
			uv.x = tex1D(UMSKFIX, uv.x);
			uv.y = tex1D(VMSKFIX, uv.y);
			uv.z = tex1D(UMSKFIX, uv.z);
			uv.w = tex1D(VMSKFIX, uv.w);
		}
	}
	else
	{
/*
		if(PS_WMS == 0)
		{
			uv.xz = frac(uv.xz);
		}
		else if(PS_WMS == 1)
		{
			uv.xz = saturate(uv.xz);
		}
		else
*/
		if(PS_WMS == 2)
		{
			uv.xz = clamp(uv.xz, MinMax.xx, MinMax.zz);
		}
		else if(PS_WMS == 3)
		{
			uv.x = tex1D(UMSKFIX, uv.x);
			uv.z = tex1D(UMSKFIX, uv.z);
		}
/*
		if(PS_WMT == 0)
		{
			uv.yw = frac(uv.yw);
		}
		else if(PS_WMT == 1)
		{
			uv.yw = saturate(uv.yw);
		}
		else
*/
		if(PS_WMT == 2)
		{
			uv.yw = clamp(uv.yw, MinMax.yy, MinMax.ww);
		}
		else if(PS_WMT == 3)
		{
			uv.y = tex1D(VMSKFIX, uv.y);
			uv.w = tex1D(VMSKFIX, uv.w);
		}
	}

	return uv;
}

float4x4 sample_4c(float4 uv)
{
	float4x4 c;

	c[0] = sample_c(uv.xy);
	c[1] = sample_c(uv.zy);
	c[2] = sample_c(uv.xw);
	c[3] = sample_c(uv.zw);

	return c;
}

float4 sample_4_index(float4 uv)
{
	float4 c;

	c.x = sample_c(uv.xy).a;
	c.y = sample_c(uv.zy).a;
	c.z = sample_c(uv.xw).a;
	c.w = sample_c(uv.zw).a;

	if (PS_RT) c *= 128.0f / 255;
	// D3D9 doesn't support integer operations

	// Most of texture will hit this code so keep normalized float value
	// 8 bits
	return c * 255./256 + 0.5/256;
}

float4x4 sample_4p(float4 u)
{
	float4x4 c;

	c[0] = sample_p(u.x);
	c[1] = sample_p(u.y);
	c[2] = sample_p(u.z);
	c[3] = sample_p(u.w);

	return c;
}

float4 sample_color(float2 st, float q)
{
	if(!PS_FST) st /= q;

	#if PS_TCOFFSETHACK
	st += TC_OffsetHack.xy;
	#endif

	float4 t;
	float4x4 c;
	float2 dd;

	if (PS_LTF == 0 && PS_AEM_FMT == FMT_32 && PS_PAL_FMT == 0 && PS_WMS < 2 && PS_WMT < 2)
	{
		c[0] = sample_c(st);
	}
	else
	{
		float4 uv;

		if(PS_LTF)
		{
			uv = st.xyxy + HalfTexel;
			dd = frac(uv.xy * WH.zw);
		}
		else
		{
			uv = st.xyxy;
		}

		uv = clamp_wrap_uv(uv);

#if PS_PAL_FMT != 0
			c = sample_4p(sample_4_index(uv));
#else
			c = sample_4c(uv);
#endif
	}

	[unroll]
	for (uint i = 0; i < 4; i++)
	{
		if(PS_AEM_FMT == FMT_32)
		{
			if(PS_RT) c[i].a *= 128.0f / 255;
		}
		else if(PS_AEM_FMT == FMT_24)
		{
			c[i].a = !PS_AEM || any(c[i].rgb) ? TA.x : 0;
		}
		else if(PS_AEM_FMT == FMT_16)
		{
			c[i].a = c[i].a >= 0.5 ? TA.y : !PS_AEM || any(c[i].rgb) ? TA.x : 0;
		}
	}

	if(PS_LTF)
	{
		t = lerp(lerp(c[0], c[1], dd.x), lerp(c[2], c[3], dd.x), dd.y);
	}
	else
	{
		t = c[0];
	}

	return t;
}

float4 tfx(float4 t, float4 c)
{
	if(PS_TFX == 0)
	{
		if(PS_TCC)
		{
			c = c * t * 255.0f / 128;
		}
		else
		{
			c.rgb = c.rgb * t.rgb * 255.0f / 128;
		}
	}
	else if(PS_TFX == 1)
	{
		if(PS_TCC)
		{
			c = t;
		}
		else
		{
			c.rgb = t.rgb;
		}
	}
	else if(PS_TFX == 2)
	{
		c.rgb = c.rgb * t.rgb * 255.0f / 128 + c.a;

		if(PS_TCC)
		{
			c.a += t.a;
		}
	}
	else if(PS_TFX == 3)
	{
		c.rgb = c.rgb * t.rgb * 255.0f / 128 + c.a;

		if(PS_TCC)
		{
			c.a = t.a;
		}
	}

	return saturate(c);
}

void datst(PS_INPUT input)
{
#if PS_DATE > 0
	float alpha = sample_rt(input.tp.xy).a;
	float alpha0x80 = 128. / 255;

	if (PS_DATE == 1 && alpha >= alpha0x80)
		discard;
	else if (PS_DATE == 2 && alpha < alpha0x80)
		discard;
#endif
}

void atst(float4 c)
{
	float a = trunc(c.a * 255 + 0.01);

	if(PS_ATST == 0)
	{
		// nothing to do
	}
	else if(PS_ATST == 1)
	{
		#if PS_SPRITEHACK == 0
		if (a > AREF) discard;
		#endif
	}
	else if(PS_ATST == 2)
	{
		if (a < AREF) discard;
	}
	else if(PS_ATST == 3)
	{
		 if (abs(a - AREF) > 0.5f) discard;
	}
	else if(PS_ATST == 4)
	{
		if (abs(a - AREF) < 0.5f) discard;
	}
}

float4 fog(float4 c, float f)
{
	if(PS_FOG)
	{
		c.rgb = lerp(FogColor, c.rgb, f);
	}

	return c;
}

float4 ps_color(PS_INPUT input)
{
	datst(input);

#if PS_CHANNEL_FETCH == 1
	float4 t = fetch_red(int2(input.p.xy));
#elif PS_CHANNEL_FETCH == 2
	float4 t = fetch_green(int2(input.p.xy));
#elif PS_CHANNEL_FETCH == 3
	float4 t = fetch_blue(int2(input.p.xy));
#elif PS_CHANNEL_FETCH == 4
	float4 t = fetch_alpha(int2(input.p.xy));
#elif PS_CHANNEL_FETCH == 5
	float4 t = fetch_rgb(int2(input.p.xy));
#elif PS_CHANNEL_FETCH == 6
	float4 t = fetch_gXbY(int2(input.p.xy));
#elif PS_DEPTH_FMT > 0
	float4 t = sample_depth(input.p.xy);
#else
	float4 t = sample_color(input.t.xy, input.t.w);
#endif

	float4 c = tfx(t, input.c);

	atst(c);

	c = fog(c, input.t.z);

	if (PS_COLCLIP == 2)
	{
		c.rgb = 256./255. - c.rgb;
	}
	if (PS_COLCLIP > 0)
	{
		c.rgb *= c.rgb < 128./255;
	}

	if(PS_CLR1) // needed for Cd * (As/Ad/F + 1) blending modes
	{
		c.rgb = 1;
	}

	return c;
}

VS_OUTPUT vs_main(VS_INPUT input)
{
	if(VS_BPPZ == 1) // 24
	{
		input.p.z = fmod(input.p.z, 0x1000000);
	}
	else if(VS_BPPZ == 2) // 16
	{
		input.p.z = fmod(input.p.z, 0x10000);
	}

	VS_OUTPUT output;

	// pos -= 0.05 (1/320 pixel) helps avoiding rounding problems (integral part of pos is usually 5 digits, 0.05 is about as low as we can go)
	// example: ceil(afterseveralvertextransformations(y = 133)) => 134 => line 133 stays empty
	// input granularity is 1/16 pixel, anything smaller than that won't step drawing up/left by one pixel
	// example: 133.0625 (133 + 1/16) should start from line 134, ceil(133.0625 - 0.05) still above 133

	float4 p = input.p - float4(0.05f, 0.05f, 0, 0);

	output.p = p * VertexScale - VertexOffset;
#if VS_RTCOPY
	output.tp = (p * VertexScale - VertexOffset) * float4(0.5, -0.5, 0, 0) + 0.5;
#endif

	if(VS_LOGZ)
	{
		output.p.z = log2(1.0f + input.p.z) / 32;
	}

	if(VS_TME)
	{
		float2 t = input.t - Texture_Scale_Offset.zw;
		if(VS_FST)
		{

			output.t.xy = t * Texture_Scale_Offset.xy;
			output.t.zw = t;
		}
		else
		{
			output.t.xy = t;
			output.t.w = input.p.w;
		}
	}
	else
	{
		output.t.xy = 0;
		output.t.w = 1.0f;
	}

	output.c = input.c;
	output.t.z = input.f.b;

	return output;
}

float4 ps_main(PS_INPUT input) : COLOR
{
	float4 c = ps_color(input);

	c.a *= 2;

	return c;
}
#endif
