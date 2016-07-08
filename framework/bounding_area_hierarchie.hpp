#ifndef BOUNDING_AREA_HIERARCHIE_HPP
#define BOUNDING_AREA_HIERARCHIE_HPP

// -----------------------------------------------------------------------------
// Copyright  : (C) 2014 Andreas-C. Bernstein
// License    : MIT (see the file LICENSE)
// Maintainer : Andreas-C. Bernstein <andreas.bernstein@uni-weimar.de>
// Stability  : experimental
//
// Cube
// -----------------------------------------------------------------------------

#define GLM_FORCE_RADIANS
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/norm.hpp>

#include <vector>

class Bounding_area_hierarchie
{
public:
	enum axis {
		x = 0u,
		y = 1u
	};

	struct marker {
		glm::vec2 p1, p2, p3, p4;
	};

	Bounding_area_hierarchie(std::vector<glm::vec2>& marker_squares, 
							 const glm::vec2 min = glm::vec2(-0.0f), 
							 const glm::vec2 max = glm::vec2(1.0f));
	

	std::vector<glm::vec4> get_aaba();


private:
	std::vector<glm::vec4> aabr;

};

#endif // BOUNDING_AREA_HIERARCHIE_HPP
