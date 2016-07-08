// -----------------------------------------------------------------------------
// Copyright  : (C) 2015 Sebastian Thiele
// License    : MIT (see the file LICENSE)
// Maintainer : Sebastian Thiele <sebastian.thiele@uni-weimar.de>
// Stability  : experimental
//
// Bounding_area_hierarchie
// -----------------------------------------------------------------------------

#include "bounding_area_hierarchie.hpp"
#include <iostream>
#include <GL/glew.h>
#include <GL/gl.h>
#include <stack>

 

Bounding_area_hierarchie::Bounding_area_hierarchie(std::vector<glm::vec2>& marker_rectangles, const glm::vec2 min, const glm::vec2 max)
{
	assert(marker_rectangles.size() > 1);

	const int nbr_of_points_per_marker_rectangle = 8;
	int nbr_of_marker_rects = marker_rectangles.size() / nbr_of_points_per_marker_rectangle;
	//first step generate all BoundingAreas (BA) for each combination of Marker Square and Distorted Marker Square 

	aabr.resize(nbr_of_marker_rects);

	glm::vec2 ba_min_all = marker_rectangles[0];
	glm::vec2 ba_max_all = marker_rectangles[0];

	for (auto r = 0u; r != nbr_of_marker_rects; ++r) {
		glm::vec2 ba_min = glm::vec2(2.0);
		glm::vec2 ba_max = glm::vec2(-1.0);;

		for (auto v = 3u; v != 3u + (nbr_of_points_per_marker_rectangle / 2); ++v) {
			ba_min.x = glm::min(ba_min.x, marker_rectangles[r * nbr_of_points_per_marker_rectangle + v].x);
			ba_min.y = glm::min(ba_min.y, marker_rectangles[r * nbr_of_points_per_marker_rectangle + v].y);
			ba_max.x = glm::max(ba_max.x, marker_rectangles[r * nbr_of_points_per_marker_rectangle + v].x);
			ba_max.y = glm::max(ba_max.y, marker_rectangles[r * nbr_of_points_per_marker_rectangle + v].y);
		
			//std::cout << "r: " << r
			//	<< " x: " << marker_rectangles[r * nbr_of_points_per_marker_rectangle + v].x 
			//	<< " y: " << marker_rectangles[r * nbr_of_points_per_marker_rectangle + v].y << std::endl;
		}

		aabr[r] = glm::vec4(ba_min, ba_max);
		ba_min_all = glm::min(ba_min_all, ba_min);
		ba_max_all = glm::max(ba_max_all, ba_max);
	}


}

std::vector<glm::vec4> Bounding_area_hierarchie::get_aaba()
{
	return aabr;
}

