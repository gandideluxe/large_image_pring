// -----------------------------------------------------------------------------
// Copyright  : (C) 2015 Sebastian Thiele
// License    : MIT (see the file LICENSE)
// Maintainer : Sebastian Thiele <sebastian.thiele@uni-weimar.de>
// Stability  : experimental
//
// Bounding_area_hierarchie
// -----------------------------------------------------------------------------

#include "bounding_area_hierarchie.hpp"
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
		glm::vec2 ba_min = marker_rectangles[r * nbr_of_points_per_marker_rectangle];
		glm::vec2 ba_max = marker_rectangles[r * nbr_of_points_per_marker_rectangle];

		for (auto v = 0u; v != nbr_of_points_per_marker_rectangle; ++v) {
			ba_min.x = glm::min(ba_min.x, marker_rectangles[r * nbr_of_points_per_marker_rectangle + v].x);
			ba_min.y = glm::min(ba_min.y, marker_rectangles[r * nbr_of_points_per_marker_rectangle + v].y);
			ba_max.x = glm::max(ba_max.x, marker_rectangles[r * nbr_of_points_per_marker_rectangle + v].x);
			ba_max.y = glm::max(ba_max.y, marker_rectangles[r * nbr_of_points_per_marker_rectangle + v].y);
		}

		aabr[r] = glm::vec4(ba_min, ba_max);
		ba_min_all = glm::min(ba_min_all, ba_min);
		ba_max_all = glm::max(ba_max_all, ba_max);
	}
	
	root = new Node(min, max);
	root->leaf = false;

	if (marker_rectangles.size() == 2) {
		root->leaf = true;
		root->index_left = 0;
		root->index_right = 1;
		root->split_axis = axis::x;
		root->ba_min = ba_min_all;
		root->ba_max = ba_max_all;
		root->markers_indices.push_back(0);
		return;
	}

	for (auto i = 0u; i != marker_rectangles.size() / nbr_of_points_per_marker_rectangle; ++i) {
		root->markers_indices.push_back(i); //root owns all rects
	}


	generate_child_nodes(root, marker_rectangles, aabr);

}

Bounding_area_hierarchie::~Bounding_area_hierarchie()
{
	delete root;
}

void Bounding_area_hierarchie::generate_child_nodes(Node * node, std::vector<glm::vec2>& marker, std::vector<glm::vec4>& aabr)
{

	std::stack<Node*> node_stack;
	node_stack.push(node);

	Node* current_node;

	while (!node_stack.empty()) {

		current_node = node_stack.top();
		node_stack.pop();

		if (current_node->markers_indices.size() == 1) {
			current_node->leaf = true;
		}
		else {
			Node* left_node = new Node();
			Node* right_node = new Node();

			float minimal_extent_x = 2.0;
			float maximal_extent_x = -1.0;
			float minimal_extent_y = 2.0;
			float maximal_extent_y = -1.0;
			
			//build aaba for all nodes of each site
			for (int ci = 0; ci != current_node->markers_indices.size(); ++ci) {
			
				minimal_extent_x = glm::min(minimal_extent_x, aabr[current_node->markers_indices[ci]].x); /// x minx
				maximal_extent_x = glm::min(maximal_extent_x, aabr[current_node->markers_indices[ci]].z); /// z max x

				minimal_extent_y = glm::min(minimal_extent_y, aabr[current_node->markers_indices[ci]].y); ///y min y
				maximal_extent_y = glm::min(maximal_extent_y, aabr[current_node->markers_indices[ci]].w); ///w max y
			}

			current_node->ba_min = glm::vec2(minimal_extent_x, minimal_extent_y);
			current_node->ba_max = glm::vec2(maximal_extent_x, maximal_extent_y);;

			float split_pos = (current_node->ba_max[current_node->split_axis] - current_node->ba_min[current_node->split_axis]) * 0.5f;

			//sort to correct childs
			for (int ci = 0; ci != current_node->markers_indices.size(); ++ci) {

				if (current_node->split_axis == axis::x) {
					if (aabr[current_node->markers_indices[ci]].z < split_pos){
						left_node->markers_indices.push_back(current_node->markers_indices[ci]);
					}
					else {
						right_node->markers_indices.push_back(current_node->markers_indices[ci]);
					}
				}
				else {
					if (aabr[current_node->markers_indices[ci]].y < split_pos) {
						left_node->markers_indices.push_back(current_node->markers_indices[ci]);
					}
					else {
						right_node->markers_indices.push_back(current_node->markers_indices[ci]);
					}
				}
			}

			node_stack.push(left_node);
			node_stack.push(right_node);
		}
	}
}

std::vector<glm::vec4> Bounding_area_hierarchie::get_aaba()
{
	return aabr;
}

