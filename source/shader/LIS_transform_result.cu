#version 430
#extension GL_ARB_shading_language_420pack : require
#extension GL_NV_gpu_shader5 : require
#extension GL_ARB_explicit_attrib_location : require


layout(std430, binding = 0) buffer bit_array_buffer
{
	uint data[];
};

layout(std430, binding = 1) buffer marker_vector_buffer
{
	vec2  marker_origin_distorted_interleaved[]; //4 vec2 marker orig 4 vec2 marker distort
};

layout(std430, binding = 2) buffer marker_aaba_buffer
{
	vec4	  aaba_buffer[];
};

layout(std430, binding = 3) buffer marker_transform_buffer
{
	vec4  marker_affine_transform_coeffs[]; //1 vec4 = a[4] 1 vec4 = b[4]
};


layout(location = 0) out vec4 FragColor;

uniform mat4 Modelview;

uniform uvec2   image_dimensions;
uniform uint    image_byte_length;

uniform uvec2   nbr_of_marker;

const int nbr_of_points_per_marker_square = 8;

vec4
get_sample_data(vec2 in_sampling_pos) {


	uvec2 pixel_pos = uvec2(in_sampling_pos * image_dimensions);
	uint byte_pos = pixel_pos.x + image_dimensions.x * pixel_pos.y;

	uint byte_offset = byte_pos / 32;
	uint bit_offset = byte_pos % 32;

	uint value = 0;
	value = uint(data[byte_offset] >> bit_offset) & 0x000001;


	return vec4(float(value), 0.0, 0.0, 1.0);

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
	//for (int j = 0; j != nbr_of_marker.y; ++j) {
	//	if (aaba_found)
	//		break;
	//	for (int i = 0; i != nbr_of_marker.x; ++i) {
	//		index = i + j * nbr_of_marker.x;
	//		vec4 aaba = aaba_buffer[index];

	//		if (point.x > aaba.x 
	//			&& point.x < aaba.z
	//			&& point.y > aaba.y 
	//			&& point.y < aaba.w) {
	//			aaba_found = true;
	//			break;
	//		}
	//	}
	//}

	//if (!aaba_found)
	//	return -1;
	//else
	//	return int(index);

	bool quad_hit;

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

vec4
affine_transform_color(vec2 frag_uv, int index) {

	//	vec2 src[4];
	//
	//	src[0] = marker_origin_distorted_interleaved[(index * nbr_of_points_per_marker_square) + 0];
	//	src[1] = marker_origin_distorted_interleaved[(index * nbr_of_points_per_marker_square) + 1];
	//	src[2] = marker_origin_distorted_interleaved[(index * nbr_of_points_per_marker_square) + 2];
	//	src[3] = marker_origin_distorted_interleaved[(index * nbr_of_points_per_marker_square) + 3];
	//
	//	vec2 dst[4];
	//
	//	dst[0] = marker_origin_distorted_interleaved[(index * nbr_of_points_per_marker_square) + 0 + (nbr_of_points_per_marker_square / 2)];
	//	dst[1] = marker_origin_distorted_interleaved[(index * nbr_of_points_per_marker_square) + 1 + (nbr_of_points_per_marker_square / 2)];
	//	dst[2] = marker_origin_distorted_interleaved[(index * nbr_of_points_per_marker_square) + 2 + (nbr_of_points_per_marker_square / 2)];
	//	dst[3] = marker_origin_distorted_interleaved[(index * nbr_of_points_per_marker_square) + 3 + (nbr_of_points_per_marker_square / 2)];
	//
	//	//calc affine transform
	//	float x1 = dst[0].x;
	//	float x2 = dst[1].x;
	//	float x3 = dst[2].x;
	//	float x4 = dst[3].x;
	//
	//	float y1 = dst[0].y;
	//	float y2 = dst[1].y;
	//	float y3 = dst[2].y;
	//	float y4 = dst[3].y;
	//
	//	vec4 vxn = vec4(src[0].x, src[1].x, src[2].x, src[3].x);
	//	vec4 vyn = vec4(src[0].y, src[1].y, src[2].y, src[3].y);
	//	/*mat4 M = mat4(x1, y1, x1*y1, 1,
	//				  x2, y2, x2*y2, 1,
	//				  x3, y3, x3*y3, 1,
	//				  x4, y4, x4*y4, 1	
	//	);
	//*/
	//	mat4 M = mat4(x1, x2, x3, x4,
	//				  y1, y2, y3, y4,
	//				  x1*y1, x2*y2, x3*y3, x4*y4,
	//				  1, 1, 1, 1
	//				);
	//
	//
	//
	//	mat4 inverseM = inverse(M);
	//
	//	//a = M^ * x;
	//	vec4 a = inverseM * vxn;
	//	vec4 b = inverseM * vyn;

	vec4 a = marker_affine_transform_coeffs[2u * index]; //+ 0
	vec4 b = marker_affine_transform_coeffs[2u * index + 1];

	vec2 src_cord = vec2(a[0] * frag_uv.x + a[1] * frag_uv.y + a[2] * frag_uv.x * frag_uv.y + a[3],
		b[0] * frag_uv.x + b[1] * frag_uv.y + b[2] * frag_uv.x * frag_uv.y + b[3]);

	//return vec4(src_cord, 0.0, 0.0);
	return get_sample_data(src_cord);
}

void main()
{

	/// Init color of fragment
	//vec4 dst = get_sample_data(frag_uv);
	vec4 dst = vec4(0.0);
	//check square
	int index = intersect(frag_uv, 0, 1);

	if (index != -1) {

		vec4 sample_data = affine_transform_color(frag_uv, index);

		//dst += /*vec4(0.0, 0.2, 0.0, 0.0) +*/ abs(sample_data - vec4(frag_uv, 0.0, 0.0));
		dst = /*vec4(0.0, 0.1, 0.0, 0.0) + */sample_data;
	}



	//index = intersect(frag_uv, 0, 0);
	//if (index != -1)
	//	dst += vec4(0.0, 0.0, 0.1, 0.0);

	FragColor = dst;
}
