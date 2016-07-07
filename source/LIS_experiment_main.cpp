// -----------------------------------------------------------------------------
// Copyright  : (C) 2014 Andreas-C. Bernstein
//                  2015 Sebastian Thiele
// License    : MIT (see the file LICENSE)
// Maintainer : Sebastian Thiele <sebastian.thiele@uni-weimar.de>
// Stability  : experimantal exercise
//
// scivis exercise Example
// -----------------------------------------------------------------------------
#ifdef _MSC_VER
#pragma warning (disable: 4996)         // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

#define _USE_MATH_DEFINES
#include "fensterchen.hpp"
#include <string>
#include <iostream>
#include <sstream>      // std::stringstream
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <vector>
#include <stack>

///GLM INCLUDES
#define GLM_FORCE_RADIANS
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/norm.hpp>


///PROJECT INCLUDES
#include <LIS_bitmap_loader_raw.hpp>
#include <bounding_area_hierarchie.hpp>
#include <utils.hpp>
#include <turntable.hpp>
#include <imgui.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>        // stb_image.h for PNG loading
#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))

         //-----------------------------------------------------------------------------
         // Helpers
         //-----------------------------------------------------------------------------

#define IM_ARRAYSIZE(_ARR)          ((int)(sizeof(_ARR)/sizeof(*_ARR)))

#ifdef INT_MAX
#define IM_INT_MAX INT_MAX
#else
#define IM_INT_MAX 2147483647
#endif

// Play it nice with Windows users. Notepad in 2014 still doesn't display text data with Unix-style \n.
#ifdef _MSC_VER
#define STR_NEWLINE "\r\n"
#else
#define STR_NEWLINE "\n"
#endif

const std::string g_file_vertex_shader("../../../source/shader/volume.vert");
const std::string g_file_fragment_shader("../../../source/shader/volume.frag");

const std::string g_GUI_file_vertex_shader("../../../source/shader/pass_through_GUI.vert");
const std::string g_GUI_file_fragment_shader("../../../source/shader/pass_through_GUI.frag");

GLuint loadShaders(std::string const& vs, std::string const& fs)
{
    std::string v = readFile(vs);
    std::string f = readFile(fs);
    return createProgram(v, f);
}

Turntable  g_turntable;

///SETUP VOLUME RAYCASTER HERE
// set the volume file
std::string g_file_string = "D:/LISData/ODN4088_L6SBA_0_Original.raw";

// set backgorund color here
//glm::vec3   g_background_color = glm::vec3(1.0f, 1.0f, 1.0f); //white
glm::vec3   g_background_color = glm::vec3(0.08f, 0.08f, 0.08f);   //grey

glm::ivec2  g_window_res = glm::ivec2(1600, 800);
Window g_win(g_window_res);

LIS_bitmap_loader_raw g_image_loader;
image_data_type g_image_data[2];
glm::uvec2 g_image_dimensions;
size_t g_image_byte_data_size;
unsigned g_channel_size;
unsigned g_channel_count;

// Volume Rendering GLSL Program
GLuint g_LIS_program(0);
std::string g_error_message;
bool g_reload_shader_error = false;

// imgui variables
static bool g_show_gui = true;

GLuint g_bitmap_SSBO = 0;
GLuint g_marker_vector_SSBO = 0;
GLuint g_marker_aaba_SSBO = 0;
GLuint g_marker_bah_SSBO = 0;

glm::uvec2 g_number_of_markers(2);

static GLuint fontTex;
static bool mousePressed[2] = { false, false };

static int g_gui_program, vert_handle, frag_handle;
static int texture_location, ortho_location;
static int position_location, uv_location, colour_location;
static size_t vbo_max_size = 20000;
static unsigned int vbo_handle, vao_handle;

//imgui values
bool g_over_gui = false;
bool g_reload_shader = false;
bool g_reload_shader_pressed = false;

int g_task_chosen = 21;
int g_task_chosen_old = g_task_chosen;

bool  g_pause = false;

Plane g_plane;

int g_bilinear_interpolation = true;



struct Manipulator
{
    Manipulator()
    : m_turntable()
    , m_mouse_button_pressed(0, 0, 0)
    , m_mouse(0.0f, 0.0f)
    , m_lastMouse(0.0f, 0.0f)
    {}

    glm::mat4 matrix()
    {
        m_turntable.rotate(m_slidelastMouse, m_slideMouse);
        return m_turntable.matrix();
    }

    glm::mat4 matrix(Window const& g_win)
    {
        m_mouse = g_win.mousePosition();
        if (g_win.isButtonPressed(Window::MOUSE_BUTTON_LEFT)) {
            if (!m_mouse_button_pressed[0]) {
                m_mouse_button_pressed[0] = 1;
            }
            m_turntable.rotate(m_lastMouse, m_mouse);
            m_slideMouse = m_mouse;
            m_slidelastMouse = m_lastMouse;
        }
        else {
            m_mouse_button_pressed[0] = 0;
            m_turntable.rotate(m_slidelastMouse, m_slideMouse);
            //m_slideMouse *= 0.99f;
            //m_slidelastMouse *= 0.99f;
        }

        if (g_win.isButtonPressed(Window::MOUSE_BUTTON_MIDDLE)) {
            if (!m_mouse_button_pressed[1]) {
                m_mouse_button_pressed[1] = 1;
            }
            m_turntable.pan(m_lastMouse, m_mouse);
        }
        else {
            m_mouse_button_pressed[1] = 0;
        }

        if (g_win.isButtonPressed(Window::MOUSE_BUTTON_RIGHT)) {
            if (!m_mouse_button_pressed[2]) {
                m_mouse_button_pressed[2] = 1;
            }
            m_turntable.zoom(m_lastMouse, m_mouse);
        }
        else {
            m_mouse_button_pressed[2] = 0;
        }

        m_lastMouse = m_mouse;
        return m_turntable.matrix();
    }

private:
    Turntable  m_turntable;
    glm::ivec3 m_mouse_button_pressed;
    glm::vec2  m_mouse;
    glm::vec2  m_lastMouse;
    glm::vec2  m_slideMouse;
    glm::vec2  m_slidelastMouse;
};

glm::vec2 rotate2D(glm::vec2 point, float angle)
{

	glm::vec2 rotation(glm::sin(angle), glm::cos(angle));

	return glm::vec2(point.x * rotation.y + point.y * rotation.x,
		point.y * rotation.y - point.x * rotation.x);

}

bool read_bitmap(std::string& volume_string){

    //init volume g_volume_loader
    //Volume_loader_raw g_volume_loader;
    //read image dimensions
#if 0
    g_image_dimensions = g_image_loader.get_dimensions(g_file_string);

    // loading volume file data
	g_image_data = g_image_loader.load_bitmap(g_file_string);
#else
	int mode = 3;

	g_image_dimensions = glm::uvec2(4000u, 4000u);
	g_image_data[0] = g_image_loader.generate_artificial_test_byte_map(mode, g_image_dimensions);
	g_image_data[1] = g_image_loader.generate_artificial_test_byte_map(0, g_image_dimensions);
	g_image_byte_data_size = g_image_loader.get_byte_size(g_image_dimensions);

	//write to disk
	std::string filename = std::string("D:/LISData/generated/test_file_bit_mode").append(std::to_string(mode)).append("_w").append(std::to_string(g_image_dimensions.x)).append("_h").append(std::to_string(g_image_dimensions.y));
	std::ofstream myFile(filename, std::ios::out | std::ios::binary);
	myFile.write((char*)&g_image_data[0][0], g_image_byte_data_size);
	myFile.close();

	std::cout << filename << " written" << std::endl;
#endif

    g_channel_size = g_image_loader.get_bit_per_channel(g_file_string) / 8;
    g_channel_count = g_image_loader.get_channel_count(g_file_string);

    // setting up proxy geometry
	float max_image_dim = glm::max((float)g_image_dimensions.x, (float)g_image_dimensions.y);
    g_plane = Plane(glm::vec2(-0.0f), glm::vec2((float)g_image_dimensions.x/max_image_dim, (float)g_image_dimensions.y / max_image_dim));

	glGenBuffers(1, &g_bitmap_SSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_bitmap_SSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, g_image_byte_data_size, &g_image_data[0][0], GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    return (bool)g_bitmap_SSBO;
}


bool generate_marker_ssbo(glm::vec2 nbr_marker_squares, glm::vec2 distance_in_percent, glm::vec2 distance_to_corner) {

	/*
	generate some synthetic marker
	assuming square printing job
	*/

	glm::vec2 init_left_uper_corner = distance_to_corner;
	glm::vec2 init_right_downer_corner = glm::vec2(1.0f - distance_to_corner.x,  1.0f - distance_to_corner.y);

	glm::vec2 length_available = glm::vec2(1.0f) - (distance_to_corner * 2.0f);
	glm::vec2 length_used_for_distance = length_available * (distance_in_percent / 100.0f);
	glm::vec2 length_left_for_squares = length_available - length_used_for_distance;
	glm::vec2 length_square = length_left_for_squares / nbr_marker_squares;
	glm::vec2 length_distance = length_used_for_distance / nbr_marker_squares;

	std::vector<glm::vec2> marker_data;

	glm::vec2 area_min = glm::vec2(init_left_uper_corner.x, init_left_uper_corner.y);
	glm::vec2 area_max = glm::vec2(1.0f - init_left_uper_corner.x, 1.0f - init_left_uper_corner.y);

	float angle_of_rotation = 0.05f;

	const int nbr_of_points_per_marker_square = 8;
	marker_data.resize(nbr_marker_squares.x * nbr_marker_squares.y * nbr_of_points_per_marker_square);


	for (unsigned i = 0; i != nbr_marker_squares.y; ++i) {
		for (unsigned j = 0; j != nbr_marker_squares.x; ++j) {
			float minx = init_left_uper_corner.x + j * length_distance.x + j * length_square.y;
			float maxx = init_left_uper_corner.x + j * length_distance.x + j * length_square.y + length_square.y;
			
			
			float miny = init_left_uper_corner.y + i * length_distance.y + i * length_square.y;
			float maxy = init_left_uper_corner.y + i * length_distance.y + i * length_square.y + length_square.y;
		
			marker_data[(i * nbr_marker_squares.x + j) * nbr_of_points_per_marker_square + 0] = glm::vec2(minx, miny);
			marker_data[(i * nbr_marker_squares.x + j) * nbr_of_points_per_marker_square + 1] = glm::vec2(maxx, miny);
			marker_data[(i * nbr_marker_squares.x + j) * nbr_of_points_per_marker_square + 2] = glm::vec2(maxx, maxy);
			marker_data[(i * nbr_marker_squares.x + j) * nbr_of_points_per_marker_square + 3] = glm::vec2(minx, maxy);
			
			marker_data[(i * nbr_marker_squares.x + j) * nbr_of_points_per_marker_square + 4] = rotate2D(marker_data[(i * nbr_marker_squares.x + j) * nbr_of_points_per_marker_square + 0], angle_of_rotation);
			marker_data[(i * nbr_marker_squares.x + j) * nbr_of_points_per_marker_square + 5] = rotate2D(marker_data[(i * nbr_marker_squares.x + j) * nbr_of_points_per_marker_square + 1], angle_of_rotation);
			marker_data[(i * nbr_marker_squares.x + j) * nbr_of_points_per_marker_square + 6] = rotate2D(marker_data[(i * nbr_marker_squares.x + j) * nbr_of_points_per_marker_square + 2], angle_of_rotation);
			marker_data[(i * nbr_marker_squares.x + j) * nbr_of_points_per_marker_square + 7] = rotate2D(marker_data[(i * nbr_marker_squares.x + j) * nbr_of_points_per_marker_square + 3], angle_of_rotation);				
						
		}
	}

	//generate BAH
	Bounding_area_hierarchie bah(marker_data, area_min, area_max);
	//convert BAH to binary data used on the GPU for traversal
	//struct BinaryGLSLNode
	//{
	//	glm::vec2 aaba_min;
	//	glm::vec2 aaba_max;
	//	
	//	int left_child;
	//	int right_child;
	//	
	//	int marker_index; // if != -1 not a leaf
	//};

	auto aaba = bah.get_aaba();

	//for (auto b : aaba) {

	//	std::cout << "xmin: " << b.x << " ymin: " << b.y << " xmax: " << b.z << " ymax: " << b.w << std::endl;
	//}

	//std::vector<BinaryGLSLNode> glsl_data;

	//BinaryGLSLNode root_node;
	//root_node.marker_index = 0;
	//bah.root->index = 0;
	//glsl_data.push_back(root_node);

	//std::stack<Bounding_area_hierarchie::Node*> node_stack;
	//node_stack.push(bah.root);

	//while (!node_stack.empty()) {
	//	auto current_node = node_stack.top();
	//	node_stack.pop();

	//	if (!current_node->leaf) {


	//		BinaryGLSLNode new_glsl_left_node;
	//		BinaryGLSLNode new_glsl_right_node;

	//		new_glsl_left_node.aaba_min = current_node->ba_min;
	//		new_glsl_right_node.aaba_max = current_node->ba_max;

	//		new_glsl_left_node.marker_index = 0;
	//		new_glsl_right_node.marker_index = 0;

	//		node_stack.push(current_node->left_child);
	//		node_stack.push(current_node->right_child);

	//		glsl_data.push_back(new_glsl_left_node);
	//		int left_index = glsl_data.size() - 1;
	//		
	//		glsl_data[current_node->index].left_child = left_index;
	//		current_node->left_child_index = left_index;
	//		current_node->left_child->index = left_index;

	//		glsl_data.push_back(new_glsl_right_node);
	//		int right_index = glsl_data.size() - 1;
	//		
	//		glsl_data[current_node->index].right_child = left_index;
	//		current_node->right_child_index = right_index;
	//		current_node->right_child->index = right_index;

	//		glsl_data[current_node->index].marker_index = -1;
	//		
	//	}
	//	else {
	//		assert(current_node->markers_indices.size() == 1);

	//		glsl_data[current_node->index].aaba_min = current_node->ba_min;
	//		glsl_data[current_node->index].aaba_max = current_node->ba_max;
	//		glsl_data[current_node->index].marker_index = current_node->markers_indices[0];
	//		glsl_data[current_node->index].left_child = -1;
	//		glsl_data[current_node->index].right_child = -1;
	//	}
	//}

	glGenBuffers(1, &g_marker_vector_SSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_marker_vector_SSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, (size_t)nbr_marker_squares.x * nbr_marker_squares.y * nbr_of_points_per_marker_square * sizeof(glm::vec2), &marker_data[0], GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &g_marker_aaba_SSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_marker_aaba_SSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, aaba.size() * sizeof(glm::vec4), &aaba[0], GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	//glGenBuffers(1, &g_marker_bah_SSBO);
	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_marker_bah_SSBO);
	//glBufferData(GL_SHADER_STORAGE_BUFFER, glsl_data.size() * sizeof(BinaryGLSLNode), &glsl_data[0], GL_DYNAMIC_COPY);
	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	return true;
}


// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - try adjusting ImGui::GetIO().PixelCenterOffset to 0.0f or 0.5f
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
static void ImImpl_RenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
{
    if (cmd_lists_count == 0)
        return;

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);

    // Setup texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTex);

    // Setup orthographic projection matrix
    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    const float ortho_projection[4][4] =
    {
        { 2.0f / width, 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / -height, 0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
        { -1.0f, 1.0f, 0.0f, 1.0f },
    };
    glUseProgram(g_gui_program);
    glUniform1i(texture_location, 0);
    glUniformMatrix4fv(ortho_location, 1, GL_FALSE, &ortho_projection[0][0]);

    // Grow our buffer according to what we need
    size_t total_vtx_count = 0;
    for (int n = 0; n < cmd_lists_count; n++)
        total_vtx_count += cmd_lists[n]->vtx_buffer.size();
    glBindBuffer(GL_ARRAY_BUFFER, vbo_handle);
    size_t neededBufferSize = total_vtx_count * sizeof(ImDrawVert);
    if (neededBufferSize > vbo_max_size)
    {
        vbo_max_size = neededBufferSize + 5000;  // Grow buffer
        glBufferData(GL_ARRAY_BUFFER, vbo_max_size, NULL, GL_STREAM_DRAW);
    }

    // Copy and convert all vertices into a single contiguous buffer
    unsigned char* buffer_data = (unsigned char*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    if (!buffer_data)
        return;
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        memcpy(buffer_data, &cmd_list->vtx_buffer[0], cmd_list->vtx_buffer.size() * sizeof(ImDrawVert));
        buffer_data += cmd_list->vtx_buffer.size() * sizeof(ImDrawVert);
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(vao_handle);

    int cmd_offset = 0;
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        int vtx_offset = cmd_offset;
        const ImDrawCmd* pcmd_end = cmd_list->commands.end();
        for (const ImDrawCmd* pcmd = cmd_list->commands.begin(); pcmd != pcmd_end; pcmd++)
        {
            glScissor((int)pcmd->clip_rect.x, (int)(height - pcmd->clip_rect.w), (int)(pcmd->clip_rect.z - pcmd->clip_rect.x), (int)(pcmd->clip_rect.w - pcmd->clip_rect.y));
            glDrawArrays(GL_TRIANGLES, vtx_offset, pcmd->vtx_count);
            vtx_offset += pcmd->vtx_count;
        }
        cmd_offset = vtx_offset;
    }

    // Restore modified state
    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_SCISSOR_TEST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static const char* ImImpl_GetClipboardTextFn()
{
    return glfwGetClipboardString(g_win.getGLFWwindow());
}

static void ImImpl_SetClipboardTextFn(const char* text)
{
    glfwSetClipboardString(g_win.getGLFWwindow(), text);
}


void InitImGui()
{
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f;                                  // Time elapsed since last frame, in seconds (in this sample app we'll override this every frame because our timestep is variable)
    io.PixelCenterOffset = 0.5f;                                  // Align OpenGL texels
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;                       // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    io.RenderDrawListsFn = ImImpl_RenderDrawLists;
    io.SetClipboardTextFn = ImImpl_SetClipboardTextFn;
    io.GetClipboardTextFn = ImImpl_GetClipboardTextFn;

    // Load font texture
    glGenTextures(1, &fontTex);
    glBindTexture(GL_TEXTURE_2D, fontTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    const void* png_data;
    unsigned int png_size;
    ImGui::GetDefaultFontData(NULL, NULL, &png_data, &png_size);
    int tex_x, tex_y, tex_comp;
    void* tex_data = stbi_load_from_memory((const unsigned char*)png_data, (int)png_size, &tex_x, &tex_y, &tex_comp, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_x, tex_y, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
    stbi_image_free(tex_data);


    try {
        g_gui_program = loadShaders(g_GUI_file_vertex_shader, g_GUI_file_fragment_shader);
    }
    catch (std::logic_error& e) {
        std::cerr << e.what() << std::endl;
    }

    texture_location = glGetUniformLocation(g_gui_program, "Texture");
    ortho_location = glGetUniformLocation(g_gui_program, "ortho");
    position_location = glGetAttribLocation(g_gui_program, "Position");
    uv_location = glGetAttribLocation(g_gui_program, "UV");
    colour_location = glGetAttribLocation(g_gui_program, "Colour");

    glGenBuffers(1, &vbo_handle);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_handle);
    glBufferData(GL_ARRAY_BUFFER, vbo_max_size, NULL, GL_DYNAMIC_DRAW);

    glGenVertexArrays(1, &vao_handle);
    glBindVertexArray(vao_handle);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_handle);
    glEnableVertexAttribArray(position_location);
    glEnableVertexAttribArray(uv_location);
    glEnableVertexAttribArray(colour_location);

    glVertexAttribPointer(position_location, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
    glVertexAttribPointer(uv_location, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
    glVertexAttribPointer(colour_location, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

}

void UpdateImGui()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup resolution (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(g_win.getGLFWwindow(), &w, &h);
    glfwGetFramebufferSize(g_win.getGLFWwindow(), &display_w, &display_h);
    io.DisplaySize = ImVec2((float)display_w, (float)display_h);                                   // Display size, in pixels. For clamping windows positions.

    // Setup time step
    static double time = 0.0f;
    const double current_time = glfwGetTime();
    io.DeltaTime = (float)(current_time - time);
    time = current_time;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    double mouse_x, mouse_y;
    glfwGetCursorPos(g_win.getGLFWwindow(), &mouse_x, &mouse_y);
    mouse_x *= (float)display_w / w;                                                               // Convert mouse coordinates to pixels
    mouse_y *= (float)display_h / h;
    io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);                                          // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
    io.MouseDown[0] = mousePressed[0] || glfwGetMouseButton(g_win.getGLFWwindow(), GLFW_MOUSE_BUTTON_LEFT) != 0;  // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
    io.MouseDown[1] = mousePressed[1] || glfwGetMouseButton(g_win.getGLFWwindow(), GLFW_MOUSE_BUTTON_RIGHT) != 0;

    // Start the frame
    ImGui::NewFrame();
}


void showGUI(){
    ImGui::Begin("Image Settings", &g_show_gui, ImVec2(300, 500));
    static float f;
    g_over_gui = ImGui::IsMouseHoveringAnyWindow();

    // Calculate and show frame rate
    static ImVector<float> ms_per_frame; if (ms_per_frame.empty()) { ms_per_frame.resize(400); memset(&ms_per_frame.front(), 0, ms_per_frame.size()*sizeof(float)); }
    static int ms_per_frame_idx = 0;
    static float ms_per_frame_accum = 0.0f;
    if (!g_pause){
        ms_per_frame_accum -= ms_per_frame[ms_per_frame_idx];
        ms_per_frame[ms_per_frame_idx] = ImGui::GetIO().DeltaTime * 1000.0f;
        ms_per_frame_accum += ms_per_frame[ms_per_frame_idx];

        ms_per_frame_idx = (ms_per_frame_idx + 1) % ms_per_frame.size();
    }
    const float ms_per_frame_avg = ms_per_frame_accum / ms_per_frame.size();

    if (ImGui::CollapsingHeader("Task", 0, true, true))
    {
        ImGui::Text("Data");
        ImGui::RadioButton("Max Intensity Projection", &g_task_chosen, 21);
        ImGui::RadioButton("Average Intensity Projection", &g_task_chosen, 22);

        if (g_task_chosen != g_task_chosen_old){
            g_reload_shader = true;
            g_task_chosen_old = g_task_chosen;
        }
    }

    if (ImGui::CollapsingHeader("Load Volumes", 0, true, false))
    {
		bool load_LIS_1 = false;
        bool load_LIS_2 = false;
        bool load_LIS_3 = false;

        ImGui::Text("Volumes");
		load_LIS_1 ^= ImGui::Button("Load Volume Head");

		
        if (load_LIS_1){
            g_file_string = "../../../data/head_w256_h256_d225_c1_b8.raw";
            
        }
        
}



    if (ImGui::CollapsingHeader("Quality Settings"))
    {
        ImGui::Text("Interpolation");
        ImGui::RadioButton("Nearest Neighbour", &g_bilinear_interpolation, 0);
        ImGui::RadioButton("Bilinear", &g_bilinear_interpolation, 1);

  }

    if (ImGui::CollapsingHeader("Shader", 0, true, true))
    {
        static ImVec4 text_color(1.0, 1.0, 1.0, 1.0);

        if (g_reload_shader_error) {
            text_color = ImVec4(1.0, 0.0, 0.0, 1.0);
        }
        else
        {
            text_color = ImVec4(0.0, 1.0, 0.0, 1.0);
        }

        ImGui::TextColored(text_color, "Shader OK");
        ImGui::TextWrapped(g_error_message.c_str());

        g_reload_shader ^= ImGui::Button("Reload Shader");

    }

    if (ImGui::CollapsingHeader("Timing"))
    {
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", ms_per_frame_avg, 1000.0 / ms_per_frame_avg);

        float min = *std::min_element(ms_per_frame.begin(), ms_per_frame.end());
        float max = *std::max_element(ms_per_frame.begin(), ms_per_frame.end());

        if (max - min < 10.0f){
            float mid = (max + min) * 0.5f;
            min = mid - 5.0f;
            max = mid + 5.0f;
        }

        static size_t values_offset = 0;

        char buf[50];
        sprintf(buf, "avg %f", ms_per_frame_avg);
        ImGui::PlotLines("Frame Times", &ms_per_frame.front(), (int)ms_per_frame.size(), (int)values_offset, buf, min - max * 0.1f, max * 1.1f, ImVec2(0, 70));

        ImGui::SameLine(); ImGui::Checkbox("pause", &g_pause);

    }

    if (ImGui::CollapsingHeader("Window options"))
    {

        if (ImGui::TreeNode("Style Editor"))
        {
            ImGui::ShowStyleEditor();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Logging"))
        {
            ImGui::LogButtons();
            ImGui::TreePop();
        }
    }

    ImGui::End();
}

int main(int argc, char* argv[])
{
    //g_win = Window(g_window_res);
    InitImGui();


    ///NOTHING TODO HERE-------------------------------------------------------------------------------

    // init and upload volume texture
    bool check = read_bitmap(g_file_string);
	check = generate_marker_ssbo(g_number_of_markers.x, 20, 0.1f);
   
    // loading actual raytracing shader code (volume.vert, volume.frag)
    // edit volume.frag to define the result of our volume raycaster  
    try {
        g_LIS_program = loadShaders(g_file_vertex_shader, g_file_fragment_shader);
    }
    catch (std::logic_error& e) {
        //std::cerr << e.what() << std::endl;
        std::stringstream ss;
        ss << e.what() << std::endl;
        g_error_message = ss.str();
        g_reload_shader_error = true;
    }

    // init object manipulator (turntable)
    Manipulator manipulator;

	// init current frame swap nbr
	int current_download_buffer = 0; //0 or 1
	int current_upload_buffer = 1; //0 or 1

    // manage keys here
    // add new input if neccessary (ie changing sampling distance, isovalues, ...)
    while (!g_win.shouldClose()) {

        // exit window with escape
        if (g_win.isKeyPressed(GLFW_KEY_ESCAPE)) {
            g_win.stop();
        }

        /// reload shader if key R ist pressed
        if (g_reload_shader){

            GLuint newProgram(0);
            try {
                //std::cout << "Reload shaders" << std::endl;
                newProgram = loadShaders(g_file_vertex_shader, g_file_fragment_shader);
                g_error_message = "";
            }
            catch (std::logic_error& e) {
                //std::cerr << e.what() << std::endl;
                std::stringstream ss;
                ss << e.what() << std::endl;
                g_error_message = ss.str();
                g_reload_shader_error = true;
                newProgram = 0;
            }
            if (0 != newProgram) {
                glDeleteProgram(g_LIS_program);
				g_LIS_program = newProgram;
                g_reload_shader_error = false;

            }
            else
            {
                g_reload_shader_error = true;

            }
        }

        if (g_win.isKeyPressed(GLFW_KEY_R)) {
            if (g_reload_shader_pressed != true) {
                g_reload_shader = true;
                g_reload_shader_pressed = true;
            }
            else{
                g_reload_shader = false;
            }
        }
        else {
            g_reload_shader = false;
            g_reload_shader_pressed = false;
        }



        glm::ivec2 size = g_win.windowSize();
        glViewport(0, 0, size.x, size.y);
        glClearColor(g_background_color.x, g_background_color.y, g_background_color.z, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float fovy = 45.0f;
        float aspect = (float)size.x / (float)size.y;
        float zNear = 0.025f, zFar = 10.0f;
        glm::mat4 projection = glm::perspective(fovy, aspect, zNear, zFar);

        glm::vec3 translate_rot = glm::vec3(-0.5f, -0.5f, -0.5f);
        glm::vec3 translate_pos = glm::vec3(+0.5f, -0.0f, -0.0f);

        glm::vec3 eye = glm::vec3(0.0f, 0.0f, 0.5f);
        glm::vec3 target = glm::vec3(0.0f);
        glm::vec3 up(0.0f, 1.0f, 0.0f);

        glm::detail::tmat4x4<float, glm::highp> view = glm::lookAt(eye, target, up);

        glm::detail::tmat4x4<float, glm::highp> turntable_matrix = manipulator.matrix();// manipulator.matrix(g_win);

        if (!g_over_gui){
            turntable_matrix = manipulator.matrix(g_win);
        }

        glm::detail::tmat4x4<float, glm::highp> model_view = view
            //* glm::inverse(glm::translate(translate_pos))
            //* glm::translate(translate_rot)
            //* glm::translate(translate_pos)
            * turntable_matrix
            // rotate head upright
            * glm::rotate(float(M_PI), glm::vec3(0.0f, 1.0f, 0.0f))
            * glm::rotate(float(M_PI), glm::vec3(1.0f, 0.0f, 0.0f))
            * glm::translate(translate_rot)
            ;

        glm::vec4 camera_translate = glm::column(glm::inverse(model_view), 3);
        glm::vec3 camera_location = glm::vec3(camera_translate.x, camera_translate.y, camera_translate.z);

        camera_location /= glm::vec3(camera_translate.w);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_bitmap_SSBO);
		//download
		GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
		memcpy(&g_image_data[current_download_buffer][0], p, sizeof(g_image_byte_data_size));
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		glFinish();

		//upload
		p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
		memcpy(p, &g_image_data[current_upload_buffer][0], sizeof(g_image_byte_data_size));
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		glFinish();


		int tmp_buffer = current_download_buffer;
		current_download_buffer = current_upload_buffer;
		current_upload_buffer = tmp_buffer;

		GLuint error = glGetError();
		if(error)
			std::cout << "Pre Render: " << error << std::endl;

        glUseProgram(g_LIS_program);
		
		GLuint block_index = 0;
		block_index = glGetProgramResourceIndex(g_LIS_program, GL_SHADER_STORAGE_BLOCK, "bit_array_buffer");
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, g_bitmap_SSBO);

		block_index = glGetProgramResourceIndex(g_LIS_program, GL_SHADER_STORAGE_BLOCK, "marker_vector_buffer");
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, g_marker_vector_SSBO);

		block_index = glGetProgramResourceIndex(g_LIS_program, GL_SHADER_STORAGE_BLOCK, "marker_aaba_buffer");
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, g_marker_aaba_SSBO);

		block_index = glGetProgramResourceIndex(g_LIS_program, GL_SHADER_STORAGE_BLOCK, "marker_bah_buffer");
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, g_marker_aaba_SSBO);

		glUniform1ui(glGetUniformLocation(g_LIS_program, "image_byte_length"), (GLuint)g_image_byte_data_size);
		glUniform2ui(glGetUniformLocation(g_LIS_program, "image_dimensions"), g_image_dimensions.x, g_image_dimensions.y);
		glUniform2ui(glGetUniformLocation(g_LIS_program, "nbr_of_marker"), g_number_of_markers.x, g_number_of_markers.y);
		

        glUniform3fv(glGetUniformLocation(g_LIS_program, "camera_location"), 1, glm::value_ptr(camera_location));

        glUniformMatrix4fv(glGetUniformLocation(g_LIS_program, "Projection"), 1, GL_FALSE,
            glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(g_LIS_program, "Modelview"), 1, GL_FALSE,
            glm::value_ptr(model_view));
        if (!g_pause)
            g_plane.draw();
        glUseProgram(0);

		error = glGetError();
		if (error)
			std::cout << "Post Render: " << glGetError() << std::endl;


        //IMGUI ROUTINE begin    
        ImGuiIO& io = ImGui::GetIO();
        io.MouseWheel = 0;
        mousePressed[0] = mousePressed[1] = false;
        glfwPollEvents();
        UpdateImGui();
        showGUI();

        // Rendering
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        ImGui::Render();

        g_win.update();

    }

    //IMGUI shutdown
    if (vao_handle) glDeleteVertexArrays(1, &vao_handle);
    if (vbo_handle) glDeleteBuffers(1, &vbo_handle);
    glDetachShader(g_gui_program, vert_handle);
    glDetachShader(g_gui_program, frag_handle);
    glDeleteShader(vert_handle);
    glDeleteShader(frag_handle);
    glDeleteProgram(g_gui_program);
    //IMGUI shutdown end

    ImGui::Shutdown();

    return 0;
}
