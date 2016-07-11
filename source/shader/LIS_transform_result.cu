#version 430
#extension GL_ARB_shading_language_420pack : require
#extension GL_NV_gpu_shader5 : require
#extension GL_ARB_explicit_attrib_location : require

//layout(local_size_x = 16) in;
layout(local_size_x = 32, local_size_y = 16) in;

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

struct ssbo_uniform {
	uvec2 image_dimensions;
	uvec2 image_out_dimensions;
	uint image_byte_length;
	uint image_out_byte_length;
	uvec2 nbr_of_marker;
	float	   process_status;
};

layout(std430, binding = 5) buffer uniform_ssbo_buffer
{
	ssbo_uniform  ssbo_uniforms;
};


//uniform uvec2   image_dimensions;
//uniform uvec2   image_out_dimensions;
//uniform uint    image_byte_length;
//uniform uint    image_out_byte_length;
//
//uniform uvec2   nbr_of_marker;
//
uniform float	process_status;

const int nbr_of_points_per_marker_square = 8;

uint
get_sample_data(vec2 in_sampling_pos) {

	uvec2 image_dimensions = ssbo_uniforms.image_dimensions;
	uvec2 pixel_pos = uvec2(in_sampling_pos * image_dimensions);
	uint byte_pos = pixel_pos.x + image_dimensions.x * pixel_pos.y;

	uint byte_offset = byte_pos / 32;
	uint bit_offset = byte_pos % 32;

	//GLSL is working with 32bit/64bit values only
	//data storage is 32bit	
	//
	//get 0/1 from bit 
	uint value = 0;
	value = uint(data[byte_offset] >> bit_offset) & 0x000001;


	return value;
}

void
set_sample_data(uvec2 store_pos, uint value) {

	uvec2 image_out_dimensions = ssbo_uniforms.image_out_dimensions;

	uint byte_offset = (store_pos.x + store_pos.y *image_out_dimensions.x) / 32;
	uint bit_offset = (store_pos.x + store_pos.y * image_out_dimensions.x) % 32;


	//GLSL is working with 32bit/64bit values only
	//data storage is 32bit	
	//
	//set 0/1 from bit 
	atomicOr(data_out[byte_offset], (value << bit_offset));// uint(data_out[byte_offset] | (1 << bit_offset));
	
}


int
intersect(in vec2 point, in int marker_nbr, in int interleaved_lookup) {

	uint index = 0;

	bool quad_hit = false;

	uvec2 nbr_of_marker = ssbo_uniforms.nbr_of_marker;
	//uvec2 image_out_dimensions = ssbo_uniforms.image_out_dimensions;


	for (index = 0; index != (nbr_of_marker.x * nbr_of_marker.y); ++index) {

		int i, j = 0;
		bool c = false;
		const int nvert = 4;

		vec2 vert[nvert];

		vert[0] = marker_origin_distorted_interleaved[(index * nbr_of_points_per_marker_square) + 0 + (interleaved_lookup * nbr_of_points_per_marker_square / 2)];
		vert[1] = marker_origin_distorted_interleaved[(index * nbr_of_points_per_marker_square) + 1 + (interleaved_lookup * nbr_of_points_per_marker_square / 2)];
		vert[2] = marker_origin_distorted_interleaved[(index * nbr_of_points_per_marker_square) + 2 + (interleaved_lookup * nbr_of_points_per_marker_square / 2)];
		vert[3] = marker_origin_distorted_interleaved[(index * nbr_of_points_per_marker_square) + 3 + (interleaved_lookup * nbr_of_points_per_marker_square / 2)];

		//
		for (i = 0, j = nvert - 1; i < nvert; j = i++) {
			if (((vert[i].y > point.y) != (vert[j].y > point.y))
				&& (point.x < (vert[j].x - vert[i].x) * (point.y - vert[i].y) / (vert[j].y - vert[i].y) + vert[i].x)
				)
				c = !c;
		}

		if (c) {
			quad_hit = true;
			break;
		}
	}

	if (quad_hit)
		return int(index);
	else
		return -1;


}

uint
affine_transform_color(vec2 frag_uv, int index) {

	vec4 a = marker_affine_transform_coeffs[2u * index]; //+ 0
	vec4 b = marker_affine_transform_coeffs[2u * index + 1];

	vec2 src_cord = vec2(a[0] * frag_uv.x + a[1] * frag_uv.y + a[2] * frag_uv.x * frag_uv.y + a[3],
		b[0] * frag_uv.x + b[1] * frag_uv.y + b[2] * frag_uv.x * frag_uv.y + b[3]);

	return get_sample_data(src_cord);
}

shared uint result_shared_data[16];//32*16 / 32 = 16

void main()
{
	uint byte_shared_pos = gl_LocalInvocationID.y;// (gl_LocalInvocationID.x + gl_LocalInvocationID.y * 16) / 32;
	uint bit_shared_pos = gl_LocalInvocationID.x % 32;

	if (0 == bit_shared_pos) {
		result_shared_data[byte_shared_pos] = 0;
	}
	//barrier();

	uvec2 image_dimensions = ssbo_uniforms.image_dimensions;
	uvec2 image_out_dimensions = ssbo_uniforms.image_out_dimensions;
	
	float process_status = ssbo_uniforms.process_status;// uintBitsToFloat(ssbo_uniforms[4].x);
	float step_size_y = float(image_out_dimensions.y) / float(image_dimensions.y);

	//check square
	uvec2 storePos = gl_GlobalInvocationID.xy;
	
	vec2 floating_pos_global = vec2(gl_GlobalInvocationID.xy) / vec2(ssbo_uniforms.image_out_dimensions);
	floating_pos_global.y = step_size_y * float(gl_GlobalInvocationID.y)/float(ssbo_uniforms.image_out_dimensions.y) + process_status;
	vec2 floating_pos_local = vec2(gl_GlobalInvocationID.xy) / vec2(ssbo_uniforms.image_out_dimensions);

	int index = intersect(floating_pos_global, 0, 1);
	uint sample_data = affine_transform_color(floating_pos_global, index);

#if 1//global memory
	set_sample_data(storePos, sample_data);
#else //shared memory
	atomicOr(result_shared_data[byte_shared_pos], (sample_data << bit_shared_pos));// uint(data_out[byte_offset] | (1 << bit_offset));
	//result_shared_data[byte_shared_pos] = result_shared_data[byte_shared_pos] | (sample_data << bit_shared_pos);
	
//	barrier();
	if (0 == bit_shared_pos) {		
		uint byte_pos = (gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * ssbo_uniforms.image_out_dimensions.x)/32;
		data_out[byte_pos] = result_shared_data[byte_shared_pos];
	}
#endif
}



