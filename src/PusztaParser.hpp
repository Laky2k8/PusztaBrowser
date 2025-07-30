#ifndef PUSZTAPARSER_HPP
#define PUSZTAPARSER_HPP

#include <string>
#include <vector>
#include <variant>
#include <iostream>
#include "laky_shader.h"
#include "lakys-freetype-handler.hpp"
#include "StringSplitter.hpp"
#include <map>
#include <algorithm>
#include <memory>
#include <stack>

using namespace std;

class BaseNode;
class BaseNode 
{
protected:
    BaseNode* parent = nullptr;
    std::vector<std::unique_ptr<BaseNode>> children;

public:
    virtual ~BaseNode() = default;
    
    // Common methods for all nodes
    virtual std::string get_type() const = 0;
    
    // Parent-child relationship management
    BaseNode* get_parent() const { return parent; }
    const std::vector<std::unique_ptr<BaseNode>>& get_children() const { return children; }
    
    void add_child(std::unique_ptr<BaseNode> child) {
        if (child) {
            child->parent = this;
            children.push_back(std::move(child));
        }
    }
    
    void remove_child(BaseNode* child) {
        children.erase(
            std::remove_if(children.begin(), children.end(),
                [child](const std::unique_ptr<BaseNode>& ptr) {
                    return ptr.get() == child;
                }),
            children.end()
        );
    }

};

class Text : public BaseNode
{
	private:
		string text;

	public:
		Text(const string& text, BaseNode* parent) : text(text) : text(text) 
		{
			this->parent = parent;
		}

		string get_text() const
		{
			return text;
		}

		void set_text(const string& new_text)
		{
			text = new_text;
		}

		std::string get_type() const override {
        	return "Text";
		}
};

class Element : public BaseNode
{
	private:
		string tag;

	public:
		Element(const string& tag, BaseNode* parent) : tag(tag) 
		{
			this->parent = parent;
		}

		string get_tag() const
		{
			return tag;
		}

		void set_tag(const string& new_tag)
		{
			tag = new_tag;
		}

		std::string get_type() const override 
		{
        	return "Element";
		}
};

using Token = std::variant<std::unique_ptr<Text>, std::unique_ptr<Element>>;

static string find_font_variant(
    const vector<string>& available_fonts,
    const string& base_name,
    const string& style)
{
    string key = base_name + "_" + style;
    auto it = find(available_fonts.begin(), available_fonts.end(), key);
    return (it != available_fonts.end()) ? *it : string();
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}


// PARSER
class HTMLParser
{

};

// RENDERER
class Layout
{
	private:

		Shader& s; // Reference to the shader used for rendering

		float start_x, start_y;
		float weight;
		string style;

		map<string,string> font_types;
		string font;
		float default_font_size = 12.0f; 
		float font_size = 12.0f;

		vector<Token> tokens;

	public:
		Layout(Shader& shader, float start_x, float start_y, map<string,string> types)
		: s(shader), start_x(start_x), start_y(start_y), weight(400.0f), style("regular"), font_types(types) // Initializer list???? what the C++
		{

			try
			{
				std::cout << "Setting font: " << this->font_types[this->style] << std::endl;
				this->font = this->font_types[this->style];
			}
			catch(const std::exception& e)
			{
				std::cerr << "PUSZTABROWSER: Setting " << this->style << " fonttype failed: " << e.what() << '\n';
				exit(1);
			}

			set_active_font(this->font);

			if(is_variable_font(this->font))
			{
				set_font_weight(this->font, this->weight);
			}
		}

		float point_to_pixel(float pt, float dpi) 
		{
			return pt * dpi / 72.0f;
		}

		float pixel_dpi(float px, float dpi) 
		{
			return px * dpi;
		}

		bool are_tokens_empty() const
		{
			return this->tokens.empty();
		}

		float set_cursor_x(float new_x)
		{
			this->start_x = new_x;
		}
		float set_cursor_y(float new_y)
		{
			this->start_y = new_y;
		}

		void lex(string body)
		{
			string buffer = "";
			vector<Token> out;
			bool in_tag = false;


			for (int i = 0; i < body.size(); i++)
			{
				char c = body[i];

				if(c == '<')
				{
					in_tag = true; // Start of a tag

					if(!buffer.empty())
					{
						Text textToken(buffer);
						out.emplace_back(textToken);
						buffer.clear();
					}
				}
				else if(c == '>')
				{
					in_tag = false; // End of a tag
					cout << "Found tag: " << buffer << endl;

					if(!buffer.empty())
					{
						Element tagToken(buffer);
						out.emplace_back(tagToken);
						buffer.clear();
					}
				}
				else
				{
					buffer += c;

					//cout << "Buffer: " << buffer << endl;
				}


			}

			if(!in_tag && !buffer.empty())
			{
				Text textToken(buffer);
				out.emplace_back(textToken);
			}

			this->tokens = out;

		}

		// RENDERING

		void render(float screen_width, float screen_height, float dpi_scale)
		{

			stack<bool> render_stack;
			render_stack.push(true); // Start with rendering enabled

			float cursor_x = this->start_x;
			float cursor_y = this->start_y;

			float HSTEP = 13.0f; // Horizontal step for text rendering
			float VSTEP = 18.0f; // Vertical step for text rendering

			//render_text(s, "A KURVA ANYÁD", 50.0f, (float)(screen_height - 200), font_size, glm::vec3(0.0f, 0.0f, 0.0f), screen_width, screen_height);

			//cout << "Rendering layout with " << this->tokens.size() << " tokens." << endl;

			this->font = this->font_types["regular"];
			font_size = 12.0f; // Font size in points

			// Calculate scale once at the beginning and update when font_size changes
			float desired_px = pixel_dpi(font_size, dpi_scale);
			float glyph_px = 48.0f; // Your loaded glyph size
			float scale = desired_px / glyph_px;

			// Ensure minimum readable size
			if (scale < 0.2f) scale = 0.2f;

			float line_height = desired_px * 1.2f; // Standard line height

			//cout << "Font size: " << font_size << "pt, Scale: " << scale << ", Line height: " << line_height << endl;

			for (const auto& tok : this->tokens)
			{

				// Print if token is Element

				if (const Text* pText = get_if<Text>(&tok)) 
				{

					// Check if token is empty or whitespace
					string text = pText->get_text();
					if (text.empty() || all_of(text.begin(), text.end(), [](unsigned char c) { return ::isspace(c); })) 
					{
						continue; 
					}

					// Early culling - don't render if way off screen
					if (cursor_y > screen_height + line_height || cursor_y < -line_height) 
					{
						cursor_y -= metrics("linespace") * 1.25;
						cursor_x = this->start_x;
						continue;
					}

					/*render_text(s, pText->get_text(), cursor_x, cursor_y, font_size, glm::vec3(0.0f, 0.0f, 0.0f), screen_width, screen_height);
					

					float text_width = text.size() * font_size * 0.6f;

					// Next line
					cursor_y -= calculate_content_height(this->font, pText->get_text(), font_size, screen_width) * 1.2f;*/

					for(string word : split(text, " "))
					{


						float w = measure_text(word, scale);

						if((cursor_x + w > screen_width - HSTEP))
						{
							cursor_y -= metrics("linespace") * 1.25;
							cursor_x = this->start_x;
						}

						render_text(s, word, cursor_x, cursor_y, scale, glm::vec3(0.0f, 0.0f, 0.0f), screen_width, screen_height);
						cursor_x += w + measure_text(" ", scale);
					}
					cursor_y -= metrics("linespace") * 1.25;
					cursor_x = this->start_x;
				
				}
				else if (const Element* pTag = get_if<Element>(&tok)) 
				{

					this->font = this->font_types["regular"];
					font_size = 12.0f;

					// tok holds a Element
					string tagName = pTag->get_tag();

					vector<string> supported_tags = {"b", "strong", "i", "em", "h1", "p", "big", "small"};
					bool isClosingTag = (tagName[0] == '/');
					string tag_clean = "";

					// For opening tags, just clean the tag name
					vector<string> tag_parts = split(tagName, " ");
					if (!tag_parts.empty()) {
						tag_clean = tag_parts[0];
					} else {
						tag_clean = tagName;
					}

					if(tag_clean == "b" || tag_clean == "strong")
					{
						this->weight = 700.0f; 
					}

					else if(tag_clean == "i" || tag_clean == "em")
					{
						this->font = this->font_types["italic"];
					}

					else if(tag_clean == "h1")
					{
						font_size = default_font_size + 8.0f; 
						
					}

					else if(tag_clean == "big")
					{
						font_size = default_font_size + 4.0f;
					}

					else if(tag_clean == "small")
					{
						font_size = default_font_size - 2.0f;
					}



					else if(tag_clean == "p")
					{
						font_size = 12.0f;
						
					}

					else if(tag_clean == "/b" || tag_clean == "/strong")
					{
						this->weight = 400.0f;
					}

					else if(tag_clean == "/i" || tag_clean == "/em")
					{
						this->font = this->font_types["regular"];
					}

					else if(tag_clean == "/h1")
					{
						font_size = default_font_size;
						
					}

					else if(tag_clean == "/big")
					{
						font_size = default_font_size;
					}

					else if(tag_clean == "/small")
					{
						font_size = default_font_size;
					}

					else if(tag_clean == "/p")
					{
						font_size = default_font_size;
					}
					else
					{
						this->font = this->font_types["regular"];
						this->weight = 400.0f;
						font_size = default_font_size;	
					}

					desired_px = pixel_dpi(font_size, dpi_scale);
					scale = desired_px / glyph_px;
					if (scale < 0.2f) scale = 0.2f;
					line_height = desired_px * 1.2f;

					try
					{
						set_active_font(this->font);
					}
					catch(const std::exception& e)
					{
						std::cerr << "PUSZTABROWSER: Setting " << style << " fonttype failed: " << e.what() << '\n';
						exit(1);
					}

					if(is_variable_font(this->font))
					{
						set_font_weight(this->font, this->weight);
					}



				}
			}
		}


};




/*string parse_remove_tags(string body)
{
	string cleaned = "";

	bool in_tag = false;

	for (int i = 0; i < body.size(); i++)
	{
		char c = body[i];

		if(c == '<')
		{
			in_tag = true; // Start of a tag
		}
		else if(c == '>')
		{
			in_tag = false; // End of a tag
			cleaned += ' '; // Add space after the tag
		}

		// Only return content that is not a tag
		else if(!in_tag)
		{
			cleaned += c;
		}
	}

	return cleaned;

}*/

// RENDERING




/*float calculate_token_content_height(const vector<string>& available_fonts, const vector<Token>& tokens, float text_size, float screen_width)
{
	// Font naming convention: <font_name>_<style>
	string regular_font = find_font_variant(available_fonts, "rubik", "regular");
	string bold_font = find_font_variant(available_fonts, "rubik", "bold");
	string italic_font = find_font_variant(available_fonts, "rubik", "italic");
	string bold_italic_font = find_font_variant(available_fonts, "rubik", "bold_italic");

	float total_height = 0.0f;
	float current_y = 0.0f;

	for (const auto& tok : tokens) {
		if (const Text* pText = get_if<Text>(&tok)) {
			total_height += calculate_content_height(regular_font, pText->get_text(), text_size, screen_width);
		}
		else if (const Element* pTag = get_if<Element>(&tok)) {
			string tagName = pTag->get_tag();

			if(tagName == "h1")
			{
				total_height += text_size * 2; // Example height for h1
			}
			else if(tagName == "p")
			{
				total_height += text_size; // Example height for paragraph
			}
		}
	}

	return total_height;
}*/





#endif