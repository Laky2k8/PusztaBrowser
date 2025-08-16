#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include "lakys-freetype-handler.hpp"
#include "lakys-socket-handler.hpp"
#include <string>
#include <map>
#include <memory>

#include "lakys-string-helper.hpp"

// Imgui
#include"imgui.h"
#include"imgui_impl_glfw.h"
#include"imgui_impl_opengl3.h"
#include "IconsFontAwesome6.h"

// STB_Image
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "PusztaParser.hpp"

// SETTINGS
unsigned int SCR_WIDTH = 1280;
unsigned int SCR_HEIGHT = 720;
const std::string TITLE = "Puszta Browser";
const std::string VERSION = "0.5";
std::string SSL_CERT_PATH = "assets/ca_cert.pem";

#define FONT_PATH "assets/fonts/"
float percentage_from_top_to_y(float screen_height, float percentage_from_top);
float y_to_percentage_from_top(float y, float screen_height);

// BROWSER SETTINGS
float cursor_y_default = percentage_from_top_to_y(SCR_HEIGHT, 0.2f);
float cursor_y = cursor_y_default;
float site_top = 0.0f; // Top of the site content
float site_bottom = 1000.0f;
bool show_settings = false; // Settings window visibility

vector<string> history;
int history_index = -1;

float dpi_scale = 1.0f;
const float BASE_DPI = 96.0f;
const float TARGET_TEXT_SIZE = 12.0f; // In points
float text_base_size;
vector<string> loaded_fonts;


// CALLBACKS
void framebuffer_size_callback(GLFWwindow* window, int width, int height);  // Resize callback
static void error_callback(int error, const char* description); // Error callback

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods); // Key callback
char keys[1024]; // Array to store the state of each key

//void mouse_callback(GLFWwindow* window, double xposIn, double yposIn); // Mouse callback
void scroll_callback(GLFWwindow* window, double x_offset, double y_offset); // Scroll callback

// FUNCTIONS
void processInput(GLFWwindow *window);
void calculate_dpi_scale(GLFWwindow* window);

// DELTATIME
float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame
float currentFrame = 0.0f; // Time of current frame

//void centered_text(Shader &s, const std::string &text, float y, float scale, glm::vec3 color);

void DarkMode();
void LightMode();

HTTP web;
std::unique_ptr<Layout> layout;
void search(std::string url, char* url_input);
Shader* text_shader = nullptr;
std::string site_content = "";
string site_title = "New Page";

int main()
{

	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, (TITLE + " v" + VERSION).c_str(), NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Load icon
	GLFWimage icons[1];
	int width, height, channels;
	unsigned char* icon = stbi_load("assets/images/icon.png", &width, &height, &channels, 0);
	icons[0].width = width;
	icons[0].height = height;
	icons[0].pixels = icon;
	glfwSetWindowIcon(window, 1, icons);

	// Set callbacks
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetErrorCallback(error_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, SCR_WIDTH, 0.0f, SCR_HEIGHT, 0.0f, 1.0f);

	text_shader = new Shader("assets/shaders/text.vert", "assets/shaders/text.frag");
	if (!text_shader) {
		std::cout << "Failed to create shader" << std::endl;
		glfwTerminate();
		return -1;
	}

	// Setup Dear ImGui context
    IMGUI_CHECKVERSION(); // Check imgui version
	ImGui::CreateContext(); // Create imgui context
	ImGuiIO& io = ImGui::GetIO(); (void)io; // Set up I/O:
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImFont* defaultFont = io.Fonts->AddFontFromFileTTF(FONT_PATH "non_variable/Rubik-Regular.ttf",20.0f);
	io.FontDefault = defaultFont;

	static const ImWchar fa_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	ImFontConfig icons_config;
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;
	ImFont* fa_font = io.Fonts->AddFontFromFileTTF(FONT_PATH "fa-solid-900.ttf", 20.0f, &icons_config, fa_ranges);

	ImGui::StyleColorsDark(); // Set dark mode
	DarkMode();
    ImGui_ImplGlfw_InitForOpenGL(window, true); // Initialize imgui for our window
    ImGui_ImplOpenGL3_Init("#version 330"); // For OpenGL 3.3
	bool show_demo_window = true;

	if (lfh_init_freetype(SCR_WIDTH, SCR_HEIGHT) != 0) {
		std::cout << "Failed to initialize FreeType" << std::endl;
		return -1;
	}
	init_text_rendering_buffers();

	// load fonts (FONT_PATH + font name)
	load_font("rubik_regular", FONT_PATH "Rubik_Regular.ttf");
	load_font("rubik_italic", FONT_PATH "Rubik_Italic.ttf");

	// Load characters for all loaded fonts
	load_characters("rubik_regular", 48);
	load_characters("rubik_italic", 48);

	loaded_fonts = get_loaded_fonts();
	map<string, string> font_types = {
		{"regular", "rubik_regular"},
		{"italic", "rubik_italic"}
	};

    layout = std::make_unique<Layout>(
        *text_shader,
        50.0f,
        cursor_y,
        font_types
    );

	calculate_dpi_scale(window);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// set title to FPS
		char title[256];
		sprintf(title, "%s v%s - %s (%.2f FPS)", TITLE.c_str(), VERSION.c_str(), site_title.c_str(), 1.0f / deltaTime);
		glfwSetWindowTitle(window, title);

		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Start imgui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

		/*// Size text according to the screen size
		text_base_size = (TARGET_TEXT_SIZE * dpi_scale) / 48.0f;
		
		text_base_size = std::max(0.2f, std::min(0.8f, text_base_size));*/

		// Calculate diagonal resolution
		float current_diagonal = sqrt(SCR_WIDTH * SCR_WIDTH + SCR_HEIGHT * SCR_HEIGHT);
		float reference_diagonal = sqrt(1920.0f * 1920.0f + 1080.0f * 1080.0f); // 1080p reference
		
		float scale_factor = current_diagonal / reference_diagonal;
		text_base_size = 0.3f * scale_factor;

		if(SCR_WIDTH <= 1300)
		{
			text_base_size *= 1.5;
		}
		
		// Apply bounds
		text_base_size = std::max(0.3f, std::min(0.8f, text_base_size));

		//cout << "Text base size: " << text_base_size << endl;

		render_text(*text_shader, to_string(cursor_y), 50.0f, (float)(SCR_HEIGHT - 150), text_base_size, glm::vec3(0.0f, 0.0f, 0.0f), SCR_WIDTH, SCR_HEIGHT);

		layout->set_cursor_y(cursor_y);
 
		// Render text
		if(!layout->are_tokens_empty())
		{
			if(web.should_parse)
			{
				layout->render(SCR_WIDTH, SCR_HEIGHT, dpi_scale);
				//cout << "Content height: " << content_height << endl;
			}
			else if(!site_content.empty())
			{
				//std::cout << "view source time" << std::endl;
				render_text(*text_shader, site_content, 50.0f, cursor_y, text_base_size, glm::vec3(0.0f, 0.0f, 0.0f), SCR_WIDTH, SCR_HEIGHT);
			}
			else
			{
				render_text(*text_shader, "Please enter a valid URL.", 50.0f, cursor_y, text_base_size, glm::vec3(0.0f, 0.0f, 0.0f), SCR_WIDTH, SCR_HEIGHT);
			}
		}
		else
		{
			render_text(*text_shader, "Welcome to Puszta Browser!\n\nEnter a URL to load its content.\n\nExample: https://example.com", 50.0f, cursor_y, text_base_size, glm::vec3(0.0f, 0.0f, 0.0f), SCR_WIDTH, SCR_HEIGHT);
		}

		/*render_text(text_shader, "Hello", 100.0f, 100.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
		render_text(text_shader, "This is sample text", 25.0f, 25.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));*/
		//centered_text(text_shader, (TITLE + " v" + VERSION), (float)(SCR_HEIGHT - 100), 1.0f, glm::vec3(0.3, 0.7f, 0.9f));
		//render_text(*text_shader, (site_contents + "\n" + to_string(cursor_y) + "\nSite top:" + to_string(site_top) + " ,Site bottom" + to_string(site_bottom)).c_str(), 50.0f, cursor_y, text_base_size, glm::vec3(0.0f, 0.0f, 0.0f), SCR_WIDTH, SCR_HEIGHT);

		// imgui time

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y));
		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 60));
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		if (ImGui::Begin("StatusBar", nullptr, window_flags)) {
			/*if (ImGui::BeginMenuBar()) {
				ImGui::Text((TITLE + " v" + VERSION).c_str());
				ImGui::EndMenuBar();
			}*/

			// empty space looks better
			float cursor_y_default = ImGui::GetCursorPosY();
			float cursor_y = cursor_y_default;
			ImGui::SetCursorPosY(cursor_y + 5.0f); 

			// Control buttons
			bool back_pressed = ImGui::ArrowButton("##left", ImGuiDir_Left);
			ImGui::SameLine();
			bool forward_pressed = ImGui::ArrowButton("##right", ImGuiDir_Right);
			ImGui::SameLine();

			ImGui::PushFont(fa_font);
			bool reload_pressed = ImGui::Button(ICON_FA_ARROW_ROTATE_RIGHT);
			ImGui::PopFont();


			ImGui::SameLine();

			// URL input
			static char url_input[256];
			bool should_search = ImGui::InputTextWithHint("##URL", "URL", url_input, IM_ARRAYSIZE(url_input), ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			//if (ImGui::Button("Go") || (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
			if(should_search)
			{
				// Check if the url_input is not empty
				if (strlen(url_input) > 0) 
				{

					// Search the URL
					std::cout << "Searching: " << url_input << std::endl;
					search(url_input, url_input); // Pass the URL to the search function

					cursor_y = cursor_y_default;

				} 
				else 
				{
					site_content = "Please enter a valid URL.";
				}
			}


			if(back_pressed)
			{
				if(history_index > 0)
				{
					history_index--;
					std::string url = history[history_index];
					std::cout << "Searching: " << url << std::endl;
					search(url, url_input); // Pass the URL to the search function

					cursor_y = cursor_y_default;
				}
			}

			if(forward_pressed)
			{
				if(history_index < history.size() - 1)
				{
					history_index++;
					std::string url = history[history_index];
					std::cout << "Searching: " << url << std::endl;
					search(url, url_input); // Pass the URL to the search function

					cursor_y = cursor_y_default;
				}
			}

			if(reload_pressed)
			{
				if(history_index >= 0 && history_index < history.size())
				{
					std::string url = history[history_index];
					std::cout << "Reloading: " << url << std::endl;
					search(url, url_input); 

					cursor_y = cursor_y_default;
				}
			}

			ImGui::SameLine();


			// Calculate available space and position at the right edge
			float avail = ImGui::GetContentRegionAvail().x;
			float button_width = ImGui::CalcTextSize(ICON_FA_GEAR).x + ImGui::GetStyle().FramePadding.x * 2.0f;
			ImGui::SameLine(ImGui::GetCursorPosX() + avail - button_width);

			if (ImGui::Button(ICON_FA_GEAR))
				show_settings = true;

			if(show_settings)
			{
				// Get the main viewport and calculate the center from it
				ImGuiViewport* main_viewport = ImGui::GetMainViewport(); 
				ImVec2 viewport_center = ImVec2(
					main_viewport->Pos.x + main_viewport->Size.x * 0.5f,
					main_viewport->Pos.y + main_viewport->Size.y * 0.5f
				);
				
				// Center the settings window
				ImVec2 window_size = ImVec2(400, 300);
				ImGui::SetNextWindowPos(ImVec2(
					viewport_center.x - window_size.x * 0.5f,
					viewport_center.y - window_size.y * 0.5f
				), ImGuiCond_Always);
				
				ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);
				ImGui::SetNextWindowBgAlpha(0.95f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);

				ImGui::SetNextWindowFocus();

				if (ImGui::Begin("Settings", &show_settings, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
				{
					ImGui::Text("Settings");
					ImGui::Separator();
					
					ImGui::Text("Theme:");
					ImGui::SameLine();
					ImGui::SetNextItemWidth(150.0f);

					// Dropdown, no theme selection yet just an empty list
					static int selected_theme = 0;
					const char* themes[] = { "Dark", "Light" };

					// ImGui::ListBox
					if(ImGui::ListBox("##theme", &selected_theme, themes, IM_ARRAYSIZE(themes), 2))
					{
						if(selected_theme == 0)
						{
							DarkMode();
						}
						else if(selected_theme == 1)
						{
							LightMode();
						}
					}
	
				}
				ImGui::End();
				ImGui::PopStyleVar(2);
			}


			ImGui::End();
		}
		ImGui::PopStyleVar();

        // Render the GUI
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	delete text_shader;

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	lfh_cleanup();
	return 0;
}

void search(std::string url = "", char* url_input = nullptr)
{

	try
	{
		if(!url.empty())
		{
			
			web.set(url);
			site_content = web.request();
			if (site_content.empty()) 
			{
				site_content = "Failed to load content after redirect. The website may not have sent any data, or there was a parsing error.";
			}	
				
			// Set text input content to the URL
			string current_url = web.get_url();
			if (url_input != nullptr && strlen(url_input) > 0) 
			{
				strncpy(url_input, current_url.c_str(), 255);
				url_input[255] = '\0'; // null termination
			}

			// Save to history
			if (history_index < 0 || history_index >= history.size() || history[history_index] != current_url) {
				/*if (history_index < history.size() - 1) {
					history.erase(history.begin() + history_index + 1, history.end());
				}*/
				history.push_back(current_url);
				history_index = history.size() - 1; // Set to the last index
			}

			layout->lex(site_content);

			site_title = layout->get_title();


		}
	}
	catch (...)
	{
		std::cerr << "Invalid URL";
	}

}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}


// CALLBACKS

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{

    if (height == 0) height = 1;
    
    // percentage from top
    float current_percentage_from_top = y_to_percentage_from_top(cursor_y, SCR_HEIGHT);

	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	SCR_WIDTH = width;
	SCR_HEIGHT = height;
	glOrtho(0.0f, SCR_WIDTH, SCR_HEIGHT, 0.0f, 0.0f, 1.0f);

	cursor_y = percentage_from_top_to_y(SCR_HEIGHT, current_percentage_from_top);

	calculate_dpi_scale(window);
	

}

// Error callback
static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}
 
// Key callback
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{

	if (action == GLFW_PRESS)
	{
		keys[key] = 1;
	}
	else if (action == GLFW_RELEASE)
	{
		keys[key] = 0;
	}
}

void scroll_callback(GLFWwindow* window, double x_offset, double y_offset)
{
    const float SCROLL_SENSITIVITY = 0.05f;
    float current_percentage = y_to_percentage_from_top(cursor_y, SCR_HEIGHT);
    
    if (y_offset > 0) // Scroll up
    {
        current_percentage = current_percentage + SCROLL_SENSITIVITY;
    }
    else if (y_offset < 0) // Scroll down
    {
        // Calculate maximum scroll based on content height
        current_percentage = current_percentage - SCROLL_SENSITIVITY;
    }
    
    cursor_y = percentage_from_top_to_y(SCR_HEIGHT, current_percentage);
}

void calculate_dpi_scale(GLFWwindow* window) {
    // Get monitor DPI
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor) {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        int width_mm, height_mm;
        glfwGetMonitorPhysicalSize(monitor, &width_mm, &height_mm);
        
        if (width_mm > 0 && height_mm > 0) {
            // Calculate DPI
            float dpi_x = (float)mode->width / ((float)width_mm / 25.4f);
            float dpi_y = (float)mode->height / ((float)height_mm / 25.4f);
            float avg_dpi = (dpi_x + dpi_y) / 2.0f;
            
            dpi_scale = avg_dpi / BASE_DPI;

            std::cout << "Calculated DPI: " << avg_dpi << ", Scale: " << dpi_scale << std::endl;
        } else {
            // Fallback: estimate based on common screen sizes
            if (mode->width >= 2560) {
                dpi_scale = 1.5f; // 1440p and above
            } else if (mode->width >= 1920) {
                dpi_scale = 1.2f; // 1080p
            } else {
                dpi_scale = 1.0f; // 720p and below
            }
            std::cout << "Using fallback DPI scale: " << dpi_scale << std::endl;
        }
    }
}

float percentage_from_top_to_y(float screen_height, float percentage_from_top)
{
    // Convert percentage from top to OpenGL Y coordinate
    return screen_height * (1.0f - percentage_from_top);
}

float y_to_percentage_from_top(float y, float screen_height)
{
    // Convert OpenGL Y coordinate to percentage from top
    return 1.0f - (y / screen_height);
}

void DarkMode()
{
	// Fork of Windark style from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();
	
	style.Alpha = 0.95f;
	style.DisabledAlpha = 0.6000000238418579f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.WindowRounding = 8.399999618530273f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(32.0f, 32.0f);
	style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.ChildRounding = 3.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 3.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(12, 8);
	style.FrameRounding = 12.0f;
	style.FrameBorderSize = 1.0f;
	style.ItemSpacing = ImVec2(8, 8);
	style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
	style.CellPadding = ImVec2(8, 6);
	style.IndentSpacing = 21.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 5.599999904632568f;
	style.ScrollbarRounding = 18.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 3.0f;
	style.TabRounding = 6.0f;
	style.TabBorderSize = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
	
	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.6000000238418579f, 0.6000000238418579f, 0.6000000238418579f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.125490203499794f, 0.125490203499794f, 0.125490203499794f, 1.0f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.2156862765550613f, 0.2078431397676468f, 0.2078431397676468f, 1.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.168627455830574f, 0.168627455830574f, 0.168627455830574f, 1.0f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.1459227204322815f, 0.1396599411964417f, 0.1396599411964417f, 1.0f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2403433322906494f, 0.240340918302536f, 0.240340918302536f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2156862765550613f, 0.2156862765550613f, 0.2156862765550613f, 1.0f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.250980406999588f, 0.250980406999588f, 0.250980406999588f, 1.0f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.125490203499794f, 0.125490203499794f, 0.125490203499794f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.168627455830574f, 0.168627455830574f, 0.168627455830574f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.125490203499794f, 0.125490203499794f, 0.125490203499794f, 1.0f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.2145922780036926f, 0.2072242945432663f, 0.2072242945432663f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.125490203499794f, 0.125490203499794f, 0.125490203499794f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.250980406999588f, 0.250980406999588f, 0.250980406999588f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.3019607961177826f, 0.3019607961177826f, 0.3019607961177826f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.3490196168422699f, 0.3490196168422699f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.4705882370471954f, 0.843137264251709f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.0f, 0.4705882370471954f, 0.843137264251709f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.3294117748737335f, 0.6000000238418579f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.278969943523407f, 0.2789671421051025f, 0.2789671421051025f, 1.0f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3175965547561646f, 0.3175933659076691f, 0.3175933659076691f, 1.0f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.4705882370471954f, 0.843137264251709f, 1.0f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.2156862765550613f, 0.2156862765550613f, 0.2156862765550613f, 1.0f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.250980406999588f, 0.250980406999588f, 0.250980406999588f, 1.0f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.3019607961177826f, 0.3019607961177826f, 0.3019607961177826f, 1.0f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.2156862765550613f, 0.2156862765550613f, 0.2156862765550613f, 1.0f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.250980406999588f, 0.250980406999588f, 0.250980406999588f, 1.0f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.3019607961177826f, 0.3019607961177826f, 0.3019607961177826f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2156862765550613f, 0.2156862765550613f, 0.2156862765550613f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.250980406999588f, 0.250980406999588f, 0.250980406999588f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.3019607961177826f, 0.3019607961177826f, 0.3019607961177826f, 1.0f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.168627455830574f, 0.168627455830574f, 0.168627455830574f, 1.0f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.2156862765550613f, 0.2156862765550613f, 0.2156862765550613f, 1.0f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.250980406999588f, 0.250980406999588f, 0.250980406999588f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.168627455830574f, 0.168627455830574f, 0.168627455830574f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.2156862765550613f, 0.2156862765550613f, 0.2156862765550613f, 1.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.0f, 0.4705882370471954f, 0.843137264251709f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.0f, 0.3294117748737335f, 0.6000000238418579f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.0f, 0.4705882370471954f, 0.843137264251709f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.0f, 0.3294117748737335f, 0.6000000238418579f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.0f, 0.4705882370471954f, 0.843137264251709f, 1.0f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
}


void LightMode()
{
    ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 0.95f;
	style.DisabledAlpha = 0.6000000238418579f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.WindowRounding = 8.399999618530273f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(32.0f, 32.0f);
	style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.ChildRounding = 3.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 3.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(12, 8);
	style.FrameRounding = 12.0f;
	style.FrameBorderSize = 1.0f;
	style.ItemSpacing = ImVec2(8, 8);
	style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
	style.CellPadding = ImVec2(8, 6);
	style.IndentSpacing = 21.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 5.599999904632568f;
	style.ScrollbarRounding = 18.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 3.0f;
	style.TabRounding = 6.0f;
	style.TabBorderSize = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_WindowBg]      = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_ChildBg]       = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_PopupBg]       = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_Text]          = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]  = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_Border]        = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow]  = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg]       = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive]  = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_TitleBg]        = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]  = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_Button]         = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered]  = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.4705882370471954f, 0.843137264251709f, 1.0f);
    style.Colors[ImGuiCol_Header]         = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered]  = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive]   = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    style.Colors[ImGuiCol_Separator]        = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]       = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_Tab]            = ImVec4(0.93f, 0.93f, 0.93f, 1.00f);
    style.Colors[ImGuiCol_TabHovered]     = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    style.Colors[ImGuiCol_TabActive]      = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocused]   = ImVec4(0.93f, 0.93f, 0.93f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]           = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]    = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]       = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]       = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]      = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive]= ImVec4(0.00f, 0.33f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_Button]          = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_DragDropTarget]  = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    style.Colors[ImGuiCol_NavHighlight]    = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
}