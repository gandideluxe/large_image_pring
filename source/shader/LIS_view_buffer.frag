#version 430
#extension GL_ARB_shading_language_420pack : require
#extension GL_NV_gpu_shader5 : require
#extension GL_ARB_explicit_attrib_location : require

layout(std430, binding = 0) buffer bit_array_buffer
{
	uint data[];
};

layout(std430, binding = 1) buffer bit_array_out_buffer
{
	uint data_out[];
};

layout(std430, binding = 2) buffer marker_vector_buffer
{
	vec2  marker_origin_distorted_interleaved[]; //4 vec2 marker orig 4 vec2 marker distort
};

layout(std430, binding = 3) buffer marker_aaba_buffer
{
	vec4	  aaba_buffer[];
};

layout(std430, binding = 4) buffer marker_transform_buffer
{
	vec4  marker_affine_transform_coeffs[]; //1 vec4 = a[4] 1 vec4 = b[4]
};


in vec3 entry_position;
in vec2 frag_uv;

layout(location = 0) out vec4 FragColor;

uniform mat4 Modelview;

uniform uvec2   image_out_dimensions;
uniform uint    image_out_byte_length;

const int nbr_of_points_per_marker_square = 8;

vec4
get_sample_data(vec2 in_sampling_pos) {


	uvec2 pixel_pos = uvec2(in_sampling_pos * image_out_dimensions);
	uint byte_pos = pixel_pos.x + image_out_dimensions.x * pixel_pos.y;

	uint byte_offset = byte_pos / 32;
	uint bit_offset = byte_pos % 32;

	//GLSL is working with 32bit/64bit values only
	//data storage is 32bit	
	//
	//get 0/1 from bit 
	uint value = 0;
	value = uint(data_out[byte_offset] >> bit_offset) & 0x000001;
	return vec4(float(value), 0.0, 0.0, 1.0);

}


void main()
{	
	/// Init color of fragment
	vec4 dst = vec4(0.4, 0.4, 0.4, 1.0);
	//check square
	vec4 data = get_sample_data(frag_uv);

	FragColor = dst + data;
}
