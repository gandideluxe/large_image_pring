#include "LIS_bitmap_loader_raw.hpp"

#include <iostream>
#include <fstream>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include <glm/geometric.hpp>

size_t
LIS_bitmap_loader_raw::get_byte_size(glm::uvec2 dimensions) const {

	size_t test_data_size_bit = (size_t)dimensions.x * dimensions.y;
	size_t test_data_size_byte = test_data_size_bit / 8 + (test_data_size_bit % 8);

	return test_data_size_byte;
}

image_data_type
LIS_bitmap_loader_raw::generate_artificial_test_byte_map(unsigned nbr, glm::uvec2 dimensions) {

	image_data_type test_data;

	size_t test_data_size_bit = (size_t)dimensions.x * dimensions.y;
	size_t test_data_size_byte = test_data_size_bit / 8 + (test_data_size_bit % 8);

	std::cout << "dimensions x: " << dimensions.x << " y: " << dimensions.y << std::endl;
	std::cout << "test_data_size_bit: " << test_data_size_bit / 1024 / 1024 << "MB" << std::endl;
	std::cout << "test_data_size_byte: " << test_data_size_byte / 1024 / 1024 << "MB" << std::endl;

	test_data.resize(test_data_size_byte, 0);

	//fill test data
	switch (nbr) {
	case 1: {//write bars 2bit 00 11 00 11
		unsigned bar_thickness = 1;


		unsigned bit_per_line = (size_t)dimensions.x / 8 + (dimensions.x % 8);

		for (auto y = 0; y != dimensions.y; ++y) {

			//one line
			unsigned bar_counter = 0;
			for (auto x = 0; x != dimensions.x; ++x) {
				++bar_counter;

				size_t byte_offset = (x + dimensions.x * y) / 8;
				size_t bit_offset = (x + dimensions.x * y) % 8;

				//std::cout << byte_offset << std::endl;

				if (bar_counter > bar_thickness) {
					test_data[byte_offset] = test_data[byte_offset] | (1 << bit_offset);
					//					std::cout << "1";
				}
				//else {
				//	std::cout << "0";
				//}
				if (bar_counter == bar_thickness * 2) {
					bar_counter = 0;
				}

			}
			//std::cout << std::endl;

		}


		break;
	}
	case 2: { // fast byte 1 byte 0
		for (auto y = 0; y != dimensions.y; y += 8) {
			for (auto x = 0; x != dimensions.x; x += 8) {
				size_t byte_offset = (x + dimensions.x * y) / 8;

				if (byte_offset % 2 == 0)
					test_data[byte_offset] = 0xFFFFu;
			}
		}
		break;
	}
	case 3: { // some geometry
		
		glm::vec2 box_pos(0.4, 0.2);
		glm::vec2 box_dim(0.2, 0.4);
		glm::vec2 cir_pos(0.7, 0.6);
		float circ_rad(0.2);

		for (auto y = 0; y != dimensions.y; ++y) {
			for (auto x = 0; x != dimensions.x; ++x) {
		
				size_t byte_offset = (x + dimensions.x * y) / 8;
				size_t bit_offset = (x + dimensions.x * y) % 8;


				float normalized_x = float(x) / dimensions.x;
				float normalized_y = float(y) / dimensions.y;
				//draw box
				if(normalized_x > box_pos.x - box_dim.x 
					&& normalized_x < box_pos.x + box_dim.x
					&& normalized_y > box_pos.y - box_dim.y
					&& normalized_y < box_pos.y + box_dim.y
					)
					test_data[byte_offset] = test_data[byte_offset] | (1 << bit_offset);
			
				//draw circle
				if (glm::distance(cir_pos, glm::vec2(normalized_x, normalized_y)) < circ_rad)
					test_data[byte_offset] = test_data[byte_offset] | (1 << bit_offset);
												
			}
		}
		break;
	}
	case 4: { // some regular geometry // TODO: can be optimized I know :)
		
		float max_dim = glm::max(dimensions.x, dimensions.y);
		
		float normalized_dim_x = dimensions.x / max_dim;
		float normalized_dim_y = dimensions.y / max_dim;


		float circle_distance = 0.1f;
		float circ_rad(0.05);

		glm::vec2 first_circle_offset = glm::vec2(circ_rad, circ_rad);

		int cirlces_x = glm::max(1, (int)(normalized_dim_x / (2 * circ_rad + circle_distance)));
		int cirlces_y = glm::max(1, (int)(normalized_dim_y / (2 * circ_rad + circle_distance)));
				
		for (auto y = 0; y != dimensions.y; ++y) {
			for (auto x = 0; x != dimensions.x; ++x) {

				size_t byte_offset = (x + dimensions.x * y) / 8;
				size_t bit_offset = (x + dimensions.x * y) % 8;


				float normalized_x = float(x) / max_dim;
				float normalized_y = float(y) / max_dim;
				
				bool in_circle = false;

				for (int circ_y = 0; circ_y != cirlces_y + 1; ++circ_y) {
					if (in_circle)
						break;
					for (int circ_x = 0; circ_x != cirlces_x + 1; ++circ_x) {
						glm::vec2 cir_pos = first_circle_offset + glm::vec2((2.0f * circ_rad + circle_distance)*circ_x, (2.0f * circ_rad + circle_distance)*circ_y);
						//std::cout << cir_pos.x << " " << cir_pos.y << std::endl;
						//std::cout << x << " " << y << std::endl;
						//std::cout << normalized_x << " " << normalized_y << std::endl;
						//std::cout << glm::distance(cir_pos, glm::vec2(normalized_x, normalized_y)) << std::endl << std::endl;
						//
						if (glm::distance(cir_pos, glm::vec2(normalized_x, normalized_y)) < circ_rad) {
							in_circle = true;
							break;
						}
					}
				}

				//draw cirlce
				if (in_circle) {
					test_data[byte_offset] = test_data[byte_offset] | (1 << bit_offset);
					//std::cout << normalized_x << " " << normalized_y << std::endl;

				}

			}
		}
		break;
	}
	default: break; // empty data case 0 also
	}

	return test_data;
}

image_data_type
LIS_bitmap_loader_raw::load_bitmap(std::string filepath)
{
  std::ifstream bitmap_file;
  bitmap_file.open(filepath, std::ios::in | std::ios::binary);

  image_data_type data;

  std::ifstream in(filepath, std::ifstream::ate | std::ifstream::binary);
  size_t file_size = in.tellg();

  glm::ivec2 img_dim = get_dimensions(filepath);

  if (bitmap_file.is_open()) {

	  unsigned estimatet_header_bytesize = 100;
	  std::vector<unsigned int> header_data_int;
	  std::vector<unsigned char> header_data_char;
	  header_data_int.resize(estimatet_header_bytesize);
	  header_data_char.resize(estimatet_header_bytesize * sizeof(int));
	  // read header

	  bitmap_file.seekg(0, std::ios::beg);
	  bitmap_file.read((char*)&header_data_char.front(), estimatet_header_bytesize * sizeof(int));
	  //bitmap_file.close();

	  bitmap_file.seekg(0, std::ios::beg);
	  bitmap_file.read((char*)&header_data_int.front(), estimatet_header_bytesize * sizeof(int));
	  bitmap_file.close();

	  std::string header_string(header_data_char.begin(), header_data_char.end());
	  //std::replace(header_string.begin(), header_string.end(), ';', '\n');

	  std::cout << "ResX:" << img_dim.x << std::endl;
	  std::cout << "ResY:" << img_dim.y << std::endl;
	  std::cout << "Data Size LR Bit (origin):" << file_size / 1024 / 1024 << " MB" << std::endl;
	  std::cout << "Data Size Bit:" << (size_t)img_dim.x * img_dim.y / 8 / 1024 / 1024 / 1024 << " GB" << std::endl;
	  std::cout << "Data Size Byte:" << (size_t)img_dim.x * img_dim.y / 1024 / 1024 / 1024 << " GB" << std::endl;

	  std::cout << header_string;
	  std::cout << "read dat beutiful header and Press Button" << std::endl;
	  getchar();

    unsigned channels = get_channel_count(filepath);
    unsigned byte_per_channel = get_bit_per_channel(filepath) / 8;


    size_t data_size = img_dim.x
                      * img_dim.y
                      * 1 //vol_dim.z
                      * channels
                      * byte_per_channel;

    
    data.resize(data_size);

	bitmap_file.seekg(0, std::ios::beg);
	bitmap_file.read((char*)&data.front(), data_size);
	bitmap_file.close();

    //std::cout << "File " << filepath << " successfully loaded" << std::endl;

    return data;
  } else {
    std::cerr << "File " << filepath << " doesnt exist! Check Filepath!" << std::endl;
    assert(0);
    return data;
  }

  //never reached
  assert(0);
  return data;
}

glm::ivec2 LIS_bitmap_loader_raw::get_dimensions(const std::string filepath) const
{

  std::ifstream bitmap_file;

  bitmap_file.open(filepath, std::ios::in | std::ios::binary);
  glm::ivec2 vol_dim;

  if (bitmap_file.is_open()) {

	  unsigned estimatet_header_bytesize = 100;
	  std::vector<unsigned int> header_data_int;
	  std::vector<unsigned char> header_data_char;
	  header_data_int.resize(estimatet_header_bytesize);
	  header_data_char.resize(estimatet_header_bytesize * sizeof(int));
	  // read header

	  bitmap_file.seekg(0, std::ios::beg);
	  bitmap_file.read((char*)&header_data_char.front(), estimatet_header_bytesize * sizeof(int));
	  //bitmap_file.close();

	  bitmap_file.seekg(0, std::ios::beg);
	  bitmap_file.read((char*)&header_data_int.front(), estimatet_header_bytesize * sizeof(int));
	  bitmap_file.close();

	  std::string header_string(header_data_char.begin(), header_data_char.end());

	  vol_dim = glm::ivec2(_byteswap_ulong(header_data_int[40]), _byteswap_ulong(header_data_int[41]));


	  //std::cout << "ResX:" << vol_dim.x << std::endl;
	  //std::cout << "ResY:" << vol_dim.y << std::endl;
	}
	bitmap_file.close();

	return vol_dim;
}

unsigned LIS_bitmap_loader_raw::get_channel_count(const std::string filepath) const
{
  unsigned channels = 0;

  size_t p0 = 0, p1 = std::string::npos;

  p0 = filepath.find("_c", 0) + 2;
  p1 = filepath.find("_b", p0);

  std::string token = filepath.substr(p0, p1 - p0);
  channels = std::atoi(token.c_str());

  return channels;
}

unsigned LIS_bitmap_loader_raw::get_bit_per_channel(const std::string filepath) const
{
  unsigned byte_per_channel = 0;

  size_t p0 = 0, p1 = std::string::npos;

  p0 = filepath.find("_b", 0) + 2;
  p1 = filepath.find(".", p0);

  std::string token = filepath.substr(p0, p1 - p0);
  byte_per_channel = std::atoi(token.c_str());

  return byte_per_channel;
}
