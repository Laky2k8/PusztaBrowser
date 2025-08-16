#ifndef PUSZTAPARSER_HPP
#define PUSZTAPARSER_HPP

#include <string>
#include <vector>
#include <variant>
#include <iostream>
#include <map>
#include <algorithm>
#include <stack>
#include <memory>
#include <cctype>
#include <stdexcept>

#include "laky_shader.h"
#include "lakys-freetype-handler.hpp"
#include "lakys-string-helper.hpp"

using namespace std;


vector<string> tags_to_skip = { "head", "script", "style" };

class Text
{
	private:
		string text;

	public:
		Text(const string& text) : text(text) {}

		string get_text() const
		{
			return text;
		}

		void set_text(const string& new_text)
		{
			text = new_text;
		}
};

class Element
{
	private:
		string tag;
		map<string, string> attributes;
		map<string, string> style;
		bool is_closing;

	public:
		Element(const string& tag, map<string, string> attributes, bool is_closing) : tag(tag), attributes(attributes), is_closing(is_closing) {}

		string get_tag() const
		{
			return tag;
		}

		void set_tag(const string& new_tag)
		{
			tag = new_tag;
		}

		void get_attribute(string name)
		{
			auto it = attributes.find(name);
			if (it != attributes.end()) {
				cout << "Attribute " << name << ": " << it->second << endl;
			} else {
				cout << "Attribute " << name << " not found." << endl;
			}
		}

		bool is_closing_tag() const
		{
			return is_closing;
		}
};

using Token = std::variant<Text, Element>;

string trim(const string& str) 
{
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}


static string find_font_variant(
    const vector<string>& available_fonts,
    const string& base_name,
    const string& style)
{
    string key = base_name + "_" + style;
    auto it = find(available_fonts.begin(), available_fonts.end(), key);
    return (it != available_fonts.end()) ? *it : string();
}

struct RenderedTextSegment
{
	string text;
	float x, y;
	float scale;
	glm::vec3 color;

	string font_type;
	float weight;
};
std::vector<RenderedTextSegment> display_list;



tuple<string, map<string, string>, bool> parse_tag(string body) {
	string tag_name = "";
	map<string, string> attributes;

	cout << "Starting parsing tag " << body << endl;

	int i = 0;
	bool is_closing = false;

	// Check if empty tag
	if (body.empty()) {
		return make_tuple(tag_name, attributes, is_closing);
	}
	
	// Skip the '<'
	//i++;
	
	// Check if the tag is a closing tag
	if (body[i] == '/') {
		is_closing = true;
		i++; // Skip '/'
	}

	// Parse tag name
    while (i < body.length() && body[i] != '>' && !isspace(body[i])) {
        tag_name += body[i];
        i++; 
    }
    
    // Skip whitespace after tag name
    while (i < body.length() && isspace(body[i])) {
        i++;
    }
	
	// Parse attributes
	while (i < body.length() && body[i] != '>') {
		// skip whitespace
		while (i < body.length() && isspace(body[i])) {
			i++;
		}
		
		if (i >= body.length() || body[i] == '>') break;
		
		// Parse attribute name
		string attr_name = "";
		while (i < body.length() && body[i] != '=' && body[i] != '>' && !isspace(body[i])) {
			attr_name += body[i];
			i++;
		}
		
		// skip whitespace
		while (i < body.length() && isspace(body[i])) {
			i++;
		}
		
		string attr_value = "";
		if (i < body.length() && body[i] == '=') {
			i++; // Skip '='

			// skip whitespace
			while (i < body.length() && isspace(body[i])) {
				i++;
			}
			
			// Parse attribute value
			if (i < body.length() && (body[i] == '"' || body[i] == '\'')) {
				char quote = body[i];
				i++; // Skip opening quote
				
				while (i < body.length() && body[i] != quote) {
					attr_value += body[i];
					i++;
				}
				
				if (i < body.length()) i++; // Skip closing quote
			} else {
				// Unquoted attribute value
				while (i < body.length() && body[i] != '>' && !isspace(body[i])) {
					attr_value += body[i];
					i++;
				}
			}
		}
		
		if (!attr_name.empty()) {
			attributes[attr_name] = attr_value;
		}
	}
	
	// Skip the '>'
	if (i < body.length() && body[i] == '>') {
		i++;
	}
	
	return make_tuple(tag_name, attributes, is_closing);
}

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
		float font_size;

		vector<Token> tokens;

		float HSTEP = 13.0f; // Horizontal step for text rendering
		float VSTEP = 18.0f; // Vertical step for text rendering

		float base_font_size;

		float desired_px;
		float glyph_px;

		float line_height;


		float cursor_x, cursor_y;
		float scale;
		vector<RenderedTextSegment> line;

		string page_title;

	public:
		Layout(Shader& shader, float start_x, float start_y, map<string,string> types)
		: s(shader), start_x(start_x), start_y(start_y), weight(400.0f), style("regular"), font_types(types), base_font_size(12.0f), glyph_px(48.0f), cursor_x(start_x), cursor_y(start_y) // Initializer list???? what the C++
		{

			font_size = base_font_size;

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

			this->cursor_x = new_x;
			return this->cursor_x; 

		}

		
		float set_cursor_y(float new_y)
		{

			this->cursor_y = new_y;
			return this->cursor_y; 

		}

		string get_title() const
		{
			return this->page_title;
		}

		void lex(string body)
		{
			string buffer = "";
			vector<Token> out;
			bool in_tag = false;
			string body_clean = "";
			int skip_depth = 0;
			bool in_title = false;

			// Remove all comments
			for (size_t i = 0; i < body.size(); i++) 
			{
				if (body[i] == '<' && i + 3 < body.size() && body.substr(i, 4) == "<!--") 
				{
					// Skip comment
					while (i < body.size() && !(body[i] == '-' && i + 2 < body.size() && body.substr(i, 3) == "-->")) 
					{
						i++;
					}
					i += 2; // Skip closing comment
				} else {
					body_clean += body[i];
				}
			}


			for (int i = 0; i < body_clean.size(); i++)
			{
				char c = body_clean[i];

				if(c == '<')
				{
					in_tag = true; // Start of a tag
					bool is_closing = (body_clean[i+1] == '/');

					string text_trimmed = trim(buffer);
					if(!text_trimmed.empty() && skip_depth == 0)
					{
						Text textToken(text_trimmed);
						out.emplace_back(textToken);
					}
					else if(!text_trimmed.empty() && in_title)
					{
						this->page_title = buffer;
					}
					buffer.clear();
				}
				else if(c == '>')
				{
					in_tag = false; // End of a tag

					// skip !DOCTYPE and comments
					if(buffer.find("!DOCTYPE") != string::npos || buffer.find("!--") != string::npos)
					{
						buffer.clear();
						continue;
					}

					if(!buffer.empty())
					{

						auto tag_tuple = parse_tag(buffer);

						// print tag name and attributes
						string tagName = get<0>(tag_tuple);
						map<string, string> attributes = get<1>(tag_tuple);
						bool is_closing = get<2>(tag_tuple);
						/*cout << "Tag: " << tagName << (is_closing ? " (closing)" : "") << ", with attributes:" << endl;
						for (const auto& attr : attributes) 
						{
							cout << "  " << attr.first << " = " << attr.second << endl;
						}*/

						std::transform(tagName.begin(), tagName.end(), tagName.begin(), ::tolower);
						tagName = trim(tagName);

						if(tagName == "title")
						{
							in_title = !in_title;
						}

						// Check if tag is in the skip list
						if(find(tags_to_skip.begin(), tags_to_skip.end(), tagName) != tags_to_skip.end())
						{
							if (!is_closing) 
							{
								++skip_depth;
							} else if (skip_depth > 0) 
							{
								--skip_depth;
							}
							buffer.clear();
							continue;
						}

						if(tagName != "--" && skip_depth == 0)
						{
							cout << "Rendering tag: " << tagName << (is_closing ? " (closing)" : "") << endl;
							Element elementToken(tagName, attributes, is_closing);
							out.emplace_back(elementToken);
						}
						else
						{
							cout << "Skipping tag: " << tagName << endl;
						}
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
				string text_trimmed = trim(buffer);
				if(!text_trimmed.empty() && skip_depth == 0)
				{
					Text textToken(text_trimmed);
					out.emplace_back(textToken);
				}
			}

			this->tokens = out;

		}

		// RENDERING

		void flush(float screen_width, float screen_height)
		{
			if(line.empty()) return; // Nothing to render

			/*std::vector<float> ascents;

			for (const auto& segment : line) {
				ascents.push_back(metrics(segment.font_type, "ascent"));
			}

			// Find maximum ascent
			float max_ascent = 0.0f;
			if (!ascents.empty()) {
				max_ascent = *std::max_element(ascents.begin(), ascents.end());
			}

			float baseline = cursor_y + 1.25f * max_ascent;*/

			float line_baseline;

			if (line.empty()) 
			{
				line_baseline = cursor_y + 1.25f * metrics(this->font, "ascent");
			}
			else
			{
				line_baseline = cursor_y + 1.25f * metrics(line[0].font_type, "ascent");
			}

			for (const auto& segment : line) 
			{

				RenderedTextSegment segment_corrected = segment;
				// segment_corrected.y = baseline - metrics(segment_corrected.font_type, "ascent");
				segment_corrected.y = line_baseline - metrics(segment_corrected.font_type, "ascent");

				try
				{
					set_active_font(segment_corrected.font_type);
				}

				catch(const std::exception& e)
				{
					std::cerr << "PUSZTABROWSER: Setting font failed: " << e.what() << '\n';
					exit(1);
				}

				if(is_variable_font(segment_corrected.font_type))
				{
					set_font_weight(segment_corrected.font_type, segment_corrected.weight);
				}

				render_text(s, segment_corrected.text, segment_corrected.x, segment_corrected.y, segment_corrected.scale, segment_corrected.color, screen_width, screen_height);
			}

			cursor_x = start_x; 
			line.clear();
		}

		void word(string word_text, float screen_width, float screen_height)
		{

			try
			{
				set_active_font(this->font);
			}
			catch(const std::exception& e)
			{
				std::cerr << "PUSZTABROWSER: Setting font failed in word(): " << e.what() << '\n';
				return;
			}

			if(is_variable_font(this->font))
			{
				set_font_weight(this->font, this->weight);
			}

			string word_clean = trim(word_text);

			if(!word_clean.empty())
			{
				float w = measure_text(word_clean, scale);
				float space_width = measure_text(" ", scale);

				if (line.empty() && cursor_x > start_x + 100.0f) 
				{
					std::cout << "DEBUG: Resetting cursor_x from " << cursor_x << " to " << start_x << std::endl;
					cursor_x = start_x;
				}

				// Check if this word would exceed the line width
				bool would_exceed = (cursor_x + w > screen_width - HSTEP);
				bool is_first_word_on_line = (cursor_x <= start_x + 1.0f); 
				
				if (would_exceed && !is_first_word_on_line)
				{
					flush(screen_width, screen_height);
					cursor_y -= VSTEP;
					cursor_x = start_x;
				}

				RenderedTextSegment segment = {
					word_clean,
					cursor_x,
					cursor_y,
					scale,
					glm::vec3(0.0f, 0.0f, 0.0f),
					this->font,
					this->weight
				};
				line.push_back(segment);
				
				cursor_x += w + space_width;
			}


		}

		void token(Token token, float screen_width, float screen_height)
		{
			if (const Text* pText = get_if<Text>(&token)) 
			{

				/*render_text(s, pText->get_text(), cursor_x, cursor_y, font_size, glm::vec3(0.0f, 0.0f, 0.0f), screen_width, screen_height);
				

				float text_width = text.size() * font_size * 0.6f;

				// Next line
				cursor_y -= calculate_content_height(this->font, pText->get_text(), font_size, screen_width) * 1.2f;*/

				// Check if token is empty or whitespace
				string text = pText->get_text();
				if (text.empty() || all_of(text.begin(), text.end(), [](unsigned char c) { return ::isspace(c); })) 
				{
					return; 
				}

				// don't render if off screen
				/*if (cursor_y > screen_height + this->line_height || cursor_y < -this->line_height) 
				{
					cursor_y -= metrics("linespace") * 1.25;
					cursor_x = this->start_x;
					continue;
				}*/

				for(string word_text : split(text, " "))
				{
					word(word_text, screen_width, screen_height);
				}
			}
			else if (const Element* pTag = get_if<Element>(&token)) 
			{


				// tok holds a Element
				string tagName = pTag->get_tag();


				if(!pTag->is_closing_tag())
				{
					if(tagName == "b" || tagName == "strong")
					{
						this->weight = 700.0f; 
					}

					else if(tagName == "i" || tagName == "em")
					{
						float old_ascent = metrics(this->font, "ascent");

						this->font = this->font_types["italic"];

						float new_ascent = metrics(this->font, "ascent");
						cursor_y += (old_ascent - new_ascent) * scale;
					}

					else if(tagName == "h1")
					{
						font_size = base_font_size + 8;
						flush(screen_width, screen_height);
						cursor_y -= VSTEP * 2;
						cursor_x = this->start_x;
					}

					else if(tagName == "big")
					{
						font_size = base_font_size + 4;
					}

					else if(tagName == "small")
					{
						font_size = base_font_size - 2;
					}



					else if(tagName == "p")
					{
						font_size = 12.0f;
						flush(screen_width, screen_height);
						cursor_y -= VSTEP;
						cursor_x = this->start_x;
					}

					else
					{
						float old_ascent = metrics(this->font, "ascent");

						this->font = this->font_types["regular"];

						float new_ascent = metrics(this->font, "ascent");
						cursor_y += (old_ascent - new_ascent) * scale;

						this->weight = 400.0f;
						font_size = base_font_size;			
					}
				}
				else
				{
					if(tagName == "b" || tagName == "strong")
					{
						this->weight = 400.0f;
					}

					else if(tagName == "i" || tagName == "em")
					{
						float old_ascent = metrics(this->font, "ascent");

						this->font = this->font_types["regular"];

						float new_ascent = metrics(this->font, "ascent");
						cursor_y += (old_ascent - new_ascent) * scale;
					}

					else if(tagName == "h1")
					{
						font_size = base_font_size;
						flush(screen_width, screen_height);
						cursor_y -= VSTEP * 2;
						cursor_x = this->start_x;
					}

					else if(tagName == "big")
					{
						font_size = base_font_size;
					}

					else if(tagName == "small")
					{
						font_size = base_font_size;
					}

					else if(tagName == "p")
					{
						font_size = base_font_size;
						flush(screen_width, screen_height);
						cursor_y -= VSTEP;
						cursor_x = this->start_x;
					}
					else
					{
						float old_ascent = metrics(this->font, "ascent");

						this->font = this->font_types["regular"];

						float new_ascent = metrics(this->font, "ascent");
						cursor_y += (old_ascent - new_ascent) * scale;

						this->weight = 400.0f;
						font_size = base_font_size;			
					}
				}

				/*try
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
				}*/



			}
		}

		void render(float screen_width, float screen_height, float dpi_scale)
		{
			stack<bool> render_stack;
			render_stack.push(true); // Start with rendering enabled

			cursor_x = this->start_x;


			//render_text(s, "A KURVA ANYÁD", 50.0f, (float)(screen_height - 200), font_size, glm::vec3(0.0f, 0.0f, 0.0f), screen_width, screen_height);

			//cout << "Rendering layout with " << this->tokens.size() << " tokens" << endl;

			this->font = this->font_types["regular"];
			font_size = base_font_size; // Font size in points

			float last_font_size = font_size;
			desired_px = pixel_dpi(font_size, dpi_scale);
			scale = desired_px / glyph_px;

			// Ensure minimum readable size
			if (scale < 0.2f) scale = 0.2f;

			line_height = desired_px * 1.2f; // Standard line height
			VSTEP = line_height; // Vertical step for text rendering

			//cout << "Font size: " << font_size << "pt, Scale: " << scale << ", Line height: " << line_height << endl;

			for (const auto& tok : this->tokens)
			{

				token(tok, screen_width, screen_height);
				if (font_size != last_font_size) 
				{
					desired_px = pixel_dpi(font_size, dpi_scale);
					scale = desired_px / glyph_px;
					last_font_size = font_size;
					VSTEP = line_height;
				}

			}

			flush(screen_width, screen_height);
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
				total_height += text_size * 2; 
			}
			else if(tagName == "p")
			{
				total_height += text_size; 
			}
		}
	}

	return total_height;
}*/





#endif