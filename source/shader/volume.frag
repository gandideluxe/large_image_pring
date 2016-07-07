#version 430
#extension GL_ARB_shading_language_420pack : require
#extension GL_NV_gpu_shader5 : require
#extension GL_ARB_explicit_attrib_location : require


struct Node
{
	int left_child;
	int right_child;
	int axis;

	int marker_index; // if != 0 not a leaf
};



layout(std430, binding = 0) buffer bit_array_buffer
{
	uint data[];
};

layout(std430, binding = 1) buffer marker_vector_buffer
{
	vec2  marker_origin_distorted_interleaved[];
};

layout(std430, binding = 2) buffer marker_aaba_buffer
{
	vec4	  aaba_buffer[];
};

layout(std430, binding = 3) buffer marker_bah_buffer
{
	Node  bah_nodes[];
};


in vec3 entry_position;
in vec2 frag_uv;

layout(location = 0) out vec4 FragColor;

uniform mat4 Modelview;

uniform vec3    camera_location;
uniform uvec2   image_dimensions;
uniform uint    image_byte_length;

uniform uvec2   nbr_of_marker;

vec4
get_sample_data(vec2 in_sampling_pos) {


	uvec2 pixel_pos = uvec2(in_sampling_pos * image_dimensions);
	uint byte_pos = pixel_pos.x + image_dimensions.x * pixel_pos.y;

	uint byte_offset = byte_pos / 32;
	uint bit_offset = byte_pos % 32;

	//GLSL is working with 32bit/64bit values only
	//data storage is 8bit	
	//
	//get 0/1 from bit 
	uint value = 0;
	value = uint(data[byte_offset] >> bit_offset) & 0x000001;
	//value = (uint(data[byte_offset]) & 0xFF);
	//value = (uint(data[byte_offset]));
	//value = data[0];
	//value = pixel_pos.x;

	return vec4(float(value), 0.0, 0.0, 1.0);
	//return vec4(6.0 * float(byte_offset) / (image_dimensions.x * image_dimensions.y), float(bit_offset) / 8, 0.0, 1.0);

	return vec4(float(pixel_pos.x) / image_dimensions.x, float(pixel_pos.y) / image_dimensions.y, 0.0, 1.0);

}

bool
intersectPointInTriangle(in vec2 p, in vec2 p0, in vec2 p1, in vec2 p2)
{
	float s = p0.y * p2.x - p0.x * p2.y + (p2.y - p0.y) * p.x + (p0.x - p2.x) * p.y;
	float t = p0.x * p1.y - p0.y * p1.x + (p0.y - p1.y) * p.x + (p1.x - p0.x) * p.y;

	if ((s < 0) != (t < 0))
		return false;

	float A = -p1.y * p2.x + p0.y * (p2.x - p1.x) + p0.x * (p1.y - p2.y) + p1.x * p2.y;
	if (A < 0.0)
	{
		s = -s;
		t = -t;
		A = -A;
	}
	return s > 0 && t > 0 && (s + t) <= A;
}

int
intersect(in vec2 point, in int marker_nbr, in int interleaved_lookup) {
	
	bool aaba_found = false;
	uint index = 0;

	//check AABA first
	for (int j = 0; j != nbr_of_marker.y; ++j) {
		if (aaba_found)
			break;
		for (int i = 0; i != nbr_of_marker.x; ++i) {
			index = i + j * nbr_of_marker.x;
			vec4 aaba = aaba_buffer[index];

			if (point.x > aaba.x 
				&& point.x < aaba.z
				&& point.y > aaba.y 
				&& point.y < aaba.w) {
				aaba_found = true;
				break;
			}
		}
	}

	if (!aaba_found)
		return -1;
	//else
	//	return int(index);
	
	//intersect leaf
	const int nbr_of_points_per_marker_square = 8;

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
		if (((vert[i].y>point.y) != (vert[j].y > point.y)) 
			&& (point.x < (vert[j].x - vert[i].x) * (point.y - vert[i].y) / (vert[j].y - vert[i].y) + vert[i].x)
			)
			c = !c;
	}

	if (!c)
		return -1;

	return int(index);

}



void main()
{

	/// Init color of fragment
	vec4 dst = get_sample_data(frag_uv);

	//check square
	int index = intersect(frag_uv, 0, 0);

	if (index != -1)
		dst += vec4(0.0, 1.0, 0.0, 0.0);

	//index = intersect(frag_uv, 0, 1);
	if (index != -1)
		dst += vec4(0.0, 0.0, 1.0, 0.0);

	FragColor = dst;
}
