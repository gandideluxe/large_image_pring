#ifndef GIS_bitmap_LOADER_RAW_HPP
#define GIS_bitmap_LOADER_RAW_HPP

#include "data_types_fwd.hpp"

#include <array>
#include <string>

#include <glm/vec2.hpp>


class LIS_bitmap_loader_raw
{
public:
	LIS_bitmap_loader_raw() {}

	image_data_type load_bitmap(std::string filepath);
	image_data_type generate_artificial_test_byte_map(unsigned nbr = 1, glm::uvec2 dimensions = glm::uvec2(200u, 200u));

	size_t     get_byte_size(glm::uvec2 dimensions = glm::uvec2(200u, 200u)) const;
	glm::ivec2 get_dimensions(const std::string file_path) const;
	unsigned   get_channel_count(const std::string file_path) const;
	unsigned   get_bit_per_channel(const std::string file_path) const;
private:
};

#endif // define GIS_bitmap_LOADER_RAW_HPP
