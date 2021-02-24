#ifdef SHADER_MODEL // make safe to include in resource file to enforce dependency

struct VS_INPUT
{
	float4 p : POSITION;
	float2 t : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 p : POSITION;
	float2 t : TEXCOORD0;
};

struct PS_INPUT
{
#if SHADER_MODEL < 0x300
	float4 p : TEXCOORD1;
#else
	float4 p : VPOS;
#endif
	float2 t : TEXCOORD0;
};

struct PS_OUTPUT
{
	float4 c : COLOR;
};

sampler Texture : register(s0);

float4 sample_c(float2 uv)
{
	return tex2D(Texture, uv);
}

VS_OUTPUT vs_main(VS_INPUT input)
{
	VS_OUTPUT output;

	output.p = input.p;
	output.t = input.t;

	return output;
}

PS_OUTPUT ps_main0(PS_INPUT input)
{
	PS_OUTPUT output;
	
	output.c = sample_c(input.t);

	return output;
}

PS_OUTPUT ps_main7(PS_INPUT input)
{
	PS_OUTPUT output;
	
	float4 c = sample_c(input.t);
	
	c.a = dot(c.rgb, float3(0.299, 0.587, 0.114));

	output.c = c;

	return output;
}

float4 ps_crt(PS_INPUT input, int i)
{
	float4 mask[4] = 
	{
		float4(1, 0, 0, 0), 
		float4(0, 1, 0, 0), 
		float4(0, 0, 1, 0), 
		float4(1, 1, 1, 0)
	};
	
	return sample_c(input.t) * saturate(mask[i] + 0.5f);
}

float4 ps_scanlines(PS_INPUT input, int i)
{
	float4 mask[2] =
	{
		float4(1, 1, 1, 0),
		float4(0, 0, 0, 0)
	};

	return sample_c(input.t) * saturate(mask[i] + 0.5f);
}

PS_OUTPUT ps_main1(PS_INPUT input)
{
	PS_OUTPUT output;
	
	float4 c = sample_c(input.t);
	
	c.a *= 128.0f / 255; // *= 0.5f is no good here, need to do this in order to get 0x80 for 1.0f (instead of 0x7f)
	
	output.c = c;

	return output;
}

PS_OUTPUT ps_main2(PS_INPUT input)
{
	PS_OUTPUT output;
	
	clip(sample_c(input.t).a - 255.0f / 255); // >= 0x80 pass
	
	output.c = 0;

	return output;
}

PS_OUTPUT ps_main3(PS_INPUT input)
{
	PS_OUTPUT output;
	
	clip(254.95f / 255 - sample_c(input.t).a); // < 0x80 pass (== 0x80 should not pass)
	
	output.c = 0;

	return output;
}

PS_OUTPUT ps_main4(PS_INPUT input)
{
	PS_OUTPUT output;
	
	output.c = 1;
	
	return output;
}

PS_OUTPUT ps_main5(PS_INPUT input) // scanlines
{
	PS_OUTPUT output;
	
	int4 p = (int4)input.p;

	output.c = ps_scanlines(input, p.y % 2);

	return output;
}

PS_OUTPUT ps_main6(PS_INPUT input) // diagonal
{
	PS_OUTPUT output;

	int4 p = (int4)input.p;

	output.c = ps_crt(input, (p.x + (p.y % 3)) % 3);

	return output;
}

PS_OUTPUT ps_main8(PS_INPUT input) // triangular
{
	PS_OUTPUT output;

	int4 p = (int4)input.p;

	// output.c = ps_crt(input, ((p.x + (p.y % 2) * 3) / 2) % 3);
	output.c = ps_crt(input, ((p.x + ((p.y / 2) % 2) * 3) / 2) % 3);

	return output;
}

static const float PI = 3.14159265359f;
PS_OUTPUT ps_main9(PS_INPUT input) // triangular
{
	PS_OUTPUT output;

	// Needs DX9 conversion
	/*float2 texdim, halfpixel; 
	Texture.GetDimensions(texdim.x, texdim.y); 
	if (ddy(input.t.y) * texdim.y > 0.5) 
		output.c = sample_c(input.t); 
	else
		output.c = (0.5 - 0.5 * cos(2 * PI * input.t.y * texdim.y)) * sample_c(float2(input.t.x, (floor(input.t.y * texdim.y) + 0.5) / texdim.y));
*/

	// replacement shader
	int4 p = (int4)input.p;
	output.c = ps_crt(input, ((p.x + ((p.y / 2) % 2) * 3) / 2) % 3);

	return output;
}

#endif
