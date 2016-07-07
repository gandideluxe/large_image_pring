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

	struct Node
	{
		Node()
			: ba_min(0.0, 0.0)
			, ba_max(1.0, 1.0)
			, leaf(true)
			, left_child(nullptr)
			, right_child(nullptr)
			, index(-1)
			, parent_index(0)
			, left_child_index(0)
			, right_child_index(0)
		{}

		Node(glm::vec2 pos_max, glm::vec2 pos_min)
			: ba_min(pos_min)
			, ba_max(pos_max)
			, leaf(true)
			, left_child(nullptr)
			, right_child(nullptr)
			, index(-1)
			, parent_index(0)
			, left_child_index(0)
			, right_child_index(0)
		{}

		~Node() { delete left_child, delete right_child; }

		std::vector<int> markers_indices;

		glm::vec2 ba_min;
		glm::vec2 ba_max;
		unsigned  index_left;
		unsigned  index_right;

		axis split_axis;

		bool leaf;
		
		Node* left_child;
		Node* right_child;

		int index;
		int parent_index; 		
		int left_child_index;
		int right_child_index;


	};

	Bounding_area_hierarchie(std::vector<glm::vec2>& marker_squares, 
							 const glm::vec2 min = glm::vec2(-0.0f), 
							 const glm::vec2 max = glm::vec2(1.0f));
	~Bounding_area_hierarchie();

	void generate_child_nodes(Node* node, 
		std::vector<glm::vec2>& marker,
		std::vector<glm::vec4>& aabr);

	std::vector<glm::vec4> get_aaba();

public:
	Node* root;

private:
	std::vector<glm::vec4> aabr;

};

#endif // BOUNDING_AREA_HIERARCHIE_HPP
