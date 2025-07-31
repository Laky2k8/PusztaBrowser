#pragma once

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <unordered_map>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <codecvt>
#include <locale>
#include "laky_shader.h"

// FreeType header
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H

using namespace std;

FT_Library ft; // FreeType library handle
FT_Face face; // FreeType font face handle

struct Character {
    unsigned int TextureID;  // ID handle of the glyph texture
    glm::ivec2   Size;       // Size of glyph
    glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
    unsigned int Advance;    // Offset to advance to next glyph
};


struct VariationAxis {
    FT_Tag tag;          // 4-character axis tag (e.g., 'wght', 'wdth')
    std::string name;    // Human-readable name
    FT_Fixed minimum;    // Minimum value
    FT_Fixed maximum;    // Maximum value  
    FT_Fixed def;        // Default value
};

struct VariationSettings {
    std::unordered_map<FT_Tag, FT_Fixed> values;
    
    void set_weight(float weight) {
        values[FT_MAKE_TAG('w','g','h','t')] = static_cast<FT_Fixed>(weight * 65536);
    }
    
    void set_width(float width) {
        values[FT_MAKE_TAG('w','d','t','h')] = static_cast<FT_Fixed>(width * 65536);
    }
    
    void set_slant(float slant) {
        values[FT_MAKE_TAG('s','l','n','t')] = static_cast<FT_Fixed>(slant * 65536);
    }
    
    void set_optical_size(float size) {
        values[FT_MAKE_TAG('o','p','s','z')] = static_cast<FT_Fixed>(size * 65536);
    }
    
    void set_custom(const std::string& tag_str, float value) {
        if (tag_str.length() == 4) {
            FT_Tag tag = FT_MAKE_TAG(tag_str[0], tag_str[1], tag_str[2], tag_str[3]);
            values[tag] = static_cast<FT_Fixed>(value * 65536);
        }
    }
};

struct FontInfo {
    FT_Face face;
    std::map<std::string, std::map<char32_t, Character>> character_cache; // Cache by variation key
    std::string font_path;
    bool loaded;
    bool is_variable;
    std::vector<VariationAxis> axes;
    VariationSettings current_variation;
    
    FontInfo() : face(nullptr), loaded(false) {}

    // Generate a key string for the current variation settings
    std::string get_variation_key() const {
        std::string key;
        for (const auto& pair : current_variation.values) {
            key += std::to_string(pair.first) + ":" + std::to_string(pair.second) + ";";
        }
        return key;
    }
};


glm::mat4 projection;

map<string, FontInfo> fonts;
string current_font = ""; // Track current active font

unsigned int global_text_VAO = 0;
unsigned int global_text_VBO = 0;

void ensure_characters_for_variation(FontInfo& font_info, int glyph_size);

// Initialize once during startup
void init_text_rendering_buffers() {
    if (global_text_VAO == 0) {
        glGenVertexArrays(1, &global_text_VAO);
        glGenBuffers(1, &global_text_VBO);
        glBindVertexArray(global_text_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, global_text_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}


int lfh_init_freetype(float screen_width, float screen_height)
{
    setlocale(LC_ALL, "");
    
    // Use screen coordinates: (0,0) at top-left, (width,height) at bottom-right
    projection = glm::ortho(0.0f, screen_width, screen_height, 0.0f);
    
    cout << "Initializing FreeType with screen size: " << screen_width << "x" << screen_height << endl;

    // Initialize FreeType library once
    if (FT_Init_FreeType(&ft))
    {
        cerr << "LFH ERROR::FREETYPE: Could not init FreeType Library" << endl;
        return -1;
    }
    
    return 0;
}

// VARIABLE FONTS
// Function to check if a font is variable and get its axes
bool analyze_variable_font(FontInfo& font_info) {
    FT_Face& face = font_info.face;
    
    // Check if font supports variations
    if (!(face->face_flags & FT_FACE_FLAG_MULTIPLE_MASTERS)) {
        font_info.is_variable = false;
        return false;
    }
    
    font_info.is_variable = true;
    
    // Get number of variation axes
    FT_MM_Var* master = nullptr;
    if (FT_Get_MM_Var(face, &master) != 0) {
        return false;
    }
    
    // Store axis information
    font_info.axes.clear();
    for (unsigned int i = 0; i < master->num_axis; i++) {
        VariationAxis axis;
        axis.tag = master->axis[i].tag;
        axis.name = master->axis[i].name ? std::string(master->axis[i].name) : "Unknown";
        axis.minimum = master->axis[i].minimum;
        axis.maximum = master->axis[i].maximum;
        axis.def = master->axis[i].def;
        
        font_info.axes.push_back(axis);
        
        // Set default values
        font_info.current_variation.values[axis.tag] = axis.def;
    }
    
    FT_Done_MM_Var(ft, master);
    
    std::cout << "Variable font detected with " << font_info.axes.size() << " axes:" << std::endl;
    for (const auto& axis : font_info.axes) {
        char tag_str[5] = {0};
        tag_str[0] = (axis.tag >> 24) & 0xFF;
        tag_str[1] = (axis.tag >> 16) & 0xFF;
        tag_str[2] = (axis.tag >> 8) & 0xFF;
        tag_str[3] = axis.tag & 0xFF;
        
        std::cout << "  " << tag_str << " (" << axis.name << "): " 
                  << (axis.minimum / 65536.0f) << " to " << (axis.maximum / 65536.0f)
                  << " (default: " << (axis.def / 65536.0f) << ")" << std::endl;
    }
    
    return true;
}

// Apply variation settings to font face
bool apply_variation_settings(FontInfo& font_info) {
    if (!font_info.is_variable) {
        return true; // Not a variable font, nothing to do
    }
    
    FT_Face& face = font_info.face;
    
    // Prepare coordinate array
    std::vector<FT_Fixed> coords;
    coords.reserve(font_info.axes.size());
    
    for (const auto& axis : font_info.axes) {
        auto it = font_info.current_variation.values.find(axis.tag);
        if (it != font_info.current_variation.values.end()) {
            // Clamp value to axis range
            FT_Fixed value = it->second;
            if (value < axis.minimum) value = axis.minimum;
            if (value > axis.maximum) value = axis.maximum;
            coords.push_back(value);
        } else {
            coords.push_back(axis.def);
        }
    }
    
    // Set design coordinates
    if (FT_Set_Var_Design_Coordinates(face, coords.size(), coords.data()) != 0) {
        std::cerr << "LFH ERROR: Failed to set variation coordinates" << std::endl;
        return false;
    }
    
    return true;
}

// Function to set font variation
bool set_font_variation(const std::string& font_name, const VariationSettings& settings) {
    auto font_it = fonts.find(font_name);
    if (font_it == fonts.end() || !font_it->second.loaded) {
        std::cerr << "LFH ERROR: Font '" << font_name << "' not found or not loaded" << std::endl;
        return false;
    }
    
    FontInfo& font_info = font_it->second;
    
    if (!font_info.is_variable) {
        std::cerr << "LFH ERROR: Font '" << font_name << "' is not a variable font" << std::endl;
        return false;
    }
    
    font_info.current_variation = settings;
    bool success = apply_variation_settings(font_info);
    
    if (success) {
        // Auto-load characters for the new variation with size 48 (your default)
        ensure_characters_for_variation(font_info, 48);
    }
    
    return success;
}

bool set_font_weight(const std::string& font_name, float weight) {
    auto font_it = fonts.find(font_name);
    if (font_it == fonts.end()) return false;
    
    font_it->second.current_variation.set_weight(weight);
    bool success = apply_variation_settings(font_it->second);
    
    if (success) {
        // Auto-load characters for the new variation with size 48 (your default)
        ensure_characters_for_variation(font_it->second, 48);
    }
    
    return success;
}

bool set_font_width(const std::string& font_name, float width) {
    auto font_it = fonts.find(font_name);
    if (font_it == fonts.end()) return false;
    
    font_it->second.current_variation.set_width(width);
    bool success = apply_variation_settings(font_it->second);
    
    if (success) {
        // Auto-load characters for the new variation with size 48 (your default)
        ensure_characters_for_variation(font_it->second, 48);
    }
    
    return success;
}


// Check if font is variable
bool is_variable_font(const std::string& font_name) {
    auto font_it = fonts.find(font_name);
    if (font_it == fonts.end() || !font_it->second.loaded) {
        return false;
    }
    
    return font_it->second.is_variable;
}

// FONT LOADING

int load_font(const string& font_name, const char* font_path)
{
    // Check if font already exists
    if (fonts.find(font_name) != fonts.end() && fonts[font_name].loaded) {
        cout << "Font '" << font_name << "' already loaded" << endl;
        return 0;
    }
    
    FontInfo& font_info = fonts[font_name];
    
    // Load the font face
    if (FT_New_Face(ft, font_path, 0, &font_info.face))
    {
        cerr << "LFH ERROR::FREETYPE: Failed to load font: " << font_path << endl;
        fonts.erase(font_name); // Remove failed entry
        return -1;
    }
    
    FT_Select_Charmap(font_info.face, FT_ENCODING_UNICODE);
    
    font_info.font_path = font_path;
    font_info.loaded = true;

    // Is this silly billy a variable font or no
    analyze_variable_font(font_info);
    
    std::cout << "Successfully loaded font '" << font_name << "': " << font_path;
    if (font_info.is_variable) {
        std::cout << " (Variable Font)";
    }
    std::cout << std::endl;
    
    // Set as current font if it's the first one loaded
    if (current_font.empty()) {
        current_font = font_name;
    }
    
    return 0;
}

void ensure_characters_for_variation(FontInfo& font_info, int glyph_size) {
    std::string variation_key = font_info.get_variation_key() + "_" + std::to_string(glyph_size);
    
    // Check if we already have characters for this variation
    if (font_info.character_cache.find(variation_key) != font_info.character_cache.end()) {
        return; // Already loaded
    }
    
    // Load characters for this variation
    FT_Face& face = font_info.face;
    
    // Apply current variation settings
    apply_variation_settings(font_info);
    
    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    FT_Set_Pixel_Sizes(face, 0, glyph_size);
    
    std::map<char32_t, Character>& characters = font_info.character_cache[variation_key];
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    for (wchar_t c = 0x0; c <= 0x024F; ++c) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            continue; // Skip failed characters
        }
        
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                     face->glyph->bitmap.width, face->glyph->bitmap.rows,
                     0, GL_RED, GL_UNSIGNED_BYTE,
                     face->glyph->bitmap.buffer);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        characters.insert({c, character});
    }
    
    std::cout << "Auto-loaded " << characters.size() << " characters for variation '" 
              << variation_key << "'" << std::endl;
}

void load_characters(const std::string& font_name, int glyph_size) {
    auto font_it = fonts.find(font_name);
    if (font_it == fonts.end() || !font_it->second.loaded) {
        std::cerr << "LFH ERROR: Font '" << font_name << "' not found or not loaded" << std::endl;
        return;
    }
    
    FontInfo& font_info = font_it->second;
    FT_Face& face = font_info.face;
    
    // Apply current variation settings
    apply_variation_settings(font_info);
    
    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    FT_Set_Pixel_Sizes(face, 0, glyph_size);
    
    // Get variation key for caching
    std::string variation_key = font_info.get_variation_key() + "_" + std::to_string(glyph_size);
    
    // Check if we already have characters for this variation
    if (font_info.character_cache.find(variation_key) != font_info.character_cache.end()) {
        std::cout << "Characters already loaded for this variation of '" << font_name << "'" << std::endl;
        return;
    }
    
    std::map<char32_t, Character>& characters = font_info.character_cache[variation_key];
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    for (wchar_t c = 0x0; c <= 0x024F; ++c) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "LFH ERROR::FREETYPE: Failed to load glyph " << c << std::endl;
            continue;
        }
        
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                     face->glyph->bitmap.width, face->glyph->bitmap.rows,
                     0, GL_RED, GL_UNSIGNED_BYTE,
                     face->glyph->bitmap.buffer);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        characters.insert({c, character});
    }
    
    std::cout << "Loaded " << characters.size() << " characters for variation '" 
              << variation_key << "' of font '" << font_name << "'" << std::endl;
}

// Set the active font for rendering
bool set_active_font(const string& font_name)
{
    auto font_it = fonts.find(font_name);
    if (font_it == fonts.end() || !font_it->second.loaded) {
        cerr << "LFH ERROR: Cannot set active font '" << font_name << "' - not found or not loaded" << endl;
        return false;
    }
    
    current_font = font_name;


    return true;
}

// Get current active font name
string get_active_font()
{
    return current_font;
}

// Render text with specified font
void render_text(Shader &s, const string& font_name, string text, float x, float y, float scale, glm::vec3 color, float screen_width, float screen_height)
{
    auto font_it = fonts.find(font_name);
    if (font_it == fonts.end() || !font_it->second.loaded) {
        std::cerr << "LFH ERROR: Font '" << font_name << "' not found or not loaded" << std::endl;
        return;
    }
    
    const FontInfo& font_info = font_it->second;
    
    // Get the appropriate character map for current variation
    std::string variation_key = font_info.get_variation_key();
    
    // Find the best matching character cache 
    const std::map<char32_t, Character>* Characters = nullptr;
    for (const auto& cache_entry : font_info.character_cache) {
        if (cache_entry.first.find(variation_key) == 0) {
            Characters = &cache_entry.second;
            break;
        }
    }
    
    if (!Characters || Characters->empty()) {
        std::cerr << "LFH ERROR: No characters loaded for current variation of font '" << font_name << "'" << std::endl;
        return;
    }
    
    projection = glm::ortho(0.0f, screen_width, 0.0f, screen_height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    glBindVertexArray(global_text_VAO);

    // Only set shader state once per call
    s.use();
    s.setVec3f("textColor", color);
    s.setMat4("projection", projection);
    s.setInt("text", 0);

    glActiveTexture(GL_TEXTURE0);

    float initial_x = x;

    // Convert to wide string for Unicode support
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    std::u32string utf32;
    bool unicode_rendering = true;
    try {
        utf32 = conv.from_bytes(text);
    } catch (const std::range_error& e) {
        unicode_rendering = false;
    }

    // iterate through all characters
    if(unicode_rendering)
    {
        for (char32_t codepoint : utf32)
        {
            auto it = Characters->find(codepoint);  
            if (it == Characters->end()) continue;
            const Character& ch = it->second;

            if (codepoint == U'\n') {
                x = initial_x;
                y -= ch.Size.y * scale;
                continue;
            }

            if (codepoint == U'\t') {
                auto space_it = Characters->find(' ');  
                if (space_it != Characters->end()) {
                    x += 8 * (space_it->second.Advance >> 6) * scale;
                }
                continue;
            }

            if (codepoint < 32) continue;

            float xpos = x + ch.Bearing.x * scale;
            float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
            float w = ch.Size.x * scale;
            float h = ch.Size.y * scale;


            if (xpos + w < 0 || xpos > screen_width || ypos + h < 0 || ypos > screen_height) {
                x += (ch.Advance >> 6) * scale;
                continue;
            }

            float vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },            
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }           
            };

            glBindTexture(GL_TEXTURE_2D, ch.TextureID);
            glBindBuffer(GL_ARRAY_BUFFER, global_text_VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            x += (ch.Advance >> 6) * scale;
        }
    }
    else
    {
        for (char c : text)
        {
            auto it = Characters->find(c); 
            if (it == Characters->end()) continue;
            const Character& ch = it->second;

            if (c == '\n') {
                x = initial_x;
                y -= ch.Size.y * scale;
                continue;
            }

            if (c == '\t') {
                auto space_it = Characters->find(' '); 
                if (space_it != Characters->end()) {
                    x += 8 * (space_it->second.Advance >> 6) * scale;
                }
                continue;
            }

            if (c < 32 || c > 126) continue;

            float xpos = x + ch.Bearing.x * scale;
            float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
            float w = ch.Size.x * scale;
            float h = ch.Size.y * scale;

            if (xpos + w < 0 || xpos > screen_width || ypos + h < 0 || ypos > screen_height) {
                x += (ch.Advance >> 6) * scale;
                continue;
            }

            float vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },            
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }           
            };

            glBindTexture(GL_TEXTURE_2D, ch.TextureID);
            glBindBuffer(GL_ARRAY_BUFFER, global_text_VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            x += (ch.Advance >> 6) * scale;
        }
    }
    
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Convenience function to render with current active font
void render_text(Shader &s, string text, float x, float y, float scale, glm::vec3 color, float screen_width, float screen_height)
{
    if (current_font.empty()) {
        cerr << "LFH ERROR: No active font set" << endl;
        return;
    }
    render_text(s, current_font, text, x, y, scale, color, screen_width, screen_height);
}

float metrics(const string& font_name, const string& metric) {
    auto font_it = fonts.find(font_name);
    if (font_it == fonts.end() || !font_it->second.loaded) {
        cerr << "LFH ERROR: Font '" << font_name << "' not found or not loaded" << endl;
        return 0.0f;
    }
    
    const FontInfo& font_info = font_it->second;
    
    if (metric == "ascent") {
        return static_cast<float>(font_info.face->ascender) / 64.0f; // Convert from 26.6 fixed point
    } else if (metric == "descent") {
        return static_cast<float>(font_info.face->descender) / 64.0f; // Convert from 26.6 fixed point
    } else if (metric == "linespace") {
        return static_cast<float>(font_info.face->height) / 64.0f; // Convert from 26.6 fixed point
    } else if (metric == "fixed") {
        return static_cast<float>(font_info.face->max_advance_width) / 64.0f; // Convert from 26.6 fixed point
    } else {
        cerr << "LFH ERROR: Unknown metric '" << metric << "'" << endl;
        return 0.0f;
    }
}

float metrics(const string& metric) {
    if (current_font.empty()) {
        cerr << "LFH ERROR: No active font set" << endl;
        return 0.0f;
    }
    return metrics(current_font, metric);
}


float measure_text(const std::string& font_name,const std::string& text,float scale)
{
    // Locate the font
    auto f_it = fonts.find(font_name);
    if (f_it == fonts.end() || !f_it->second.loaded)
    {
        std::cerr << "LFH measure(): font '" << font_name
                  << "' not loaded\n";
        return 0.0f;
    }
    const FontInfo& fi = f_it->second;

    // Select the character cache that matches the current variation
    const std::string var_key = fi.get_variation_key();
    const std::map<char32_t, Character>* chars = nullptr;
    for (const auto& kv : fi.character_cache)
        if (kv.first.rfind(var_key, 0) == 0) {          // starts-with
            chars = &kv.second;
            break;
        }
    if (!chars || chars->empty()) {
        std::cerr << "LFH measure(): no glyphs cached for variation\n";
        return 0.0f;
    }

    // Walk the UTF-8 string and sum advances
    float total = 0.0f;

    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt;
    std::u32string u32;
    bool unicode_ok = true;
    try   { u32 = cvt.from_bytes(text); }
    catch (const std::range_error&) { unicode_ok = false; }

    auto add_advance = [&](char32_t cp)
    {
        auto it = chars->find(cp);
        if (it == chars->end()) return;
        if (cp == U'\t') {                 // 8-space tab expansion
            auto sp = chars->find(U' ');
            if (sp != chars->end())
                total += 8 * ((sp->second.Advance >> 6) * scale);
            return;
        }
        if (cp < 32 || cp == U'\n') return;   // control chars
        total += (it->second.Advance >> 6) * scale;  // 26.6 → px
    };

    if (unicode_ok)
        for (char32_t cp : u32) add_advance(cp);
    else
        for (unsigned char c : text) add_advance(c);

    return total;
}

float measure_text(const std::string& text, float scale)
{
    if (current_font.empty()) {
        std::cerr << "LFH measure(): no active font\n";
        return 0.0f;
    }
    return measure_text(current_font, text, scale);
}


// Updated calculate_content_height to work with character_cache
float calculate_content_height(const string& font_name, const std::string& text, float scale, float screen_width) {
    auto font_it = fonts.find(font_name);
    if (font_it == fonts.end() || !font_it->second.loaded) {
        cerr << "LFH ERROR: Font '" << font_name << "' not found or not loaded" << endl;
        return 0.0f;
    }
    
    const FontInfo& font_info = font_it->second;
    
    // Get the appropriate character map for current variation
    std::string variation_key = font_info.get_variation_key();
    
    // Find the best matching character cache 
    const std::map<char32_t, Character>* Characters = nullptr;
    for (const auto& cache_entry : font_info.character_cache) {
        if (cache_entry.first.find(variation_key) == 0) {
            Characters = &cache_entry.second;
            break;
        }
    }
    
    if (!Characters || Characters->empty()) {
        cerr << "LFH ERROR: No characters loaded for current variation of font '" << font_name << "'" << endl;
        return 0.0f;
    }
    
    if (text.empty()) return 0.0f;
    
    float x = 0.0f;
    float y = 0.0f;
    float max_y = 0.0f;
    float line_height = 0.0f;
    
    // Calculate line height from the tallest character
    if (Characters->find('A') != Characters->end()) {
        line_height = Characters->find('A')->second.Size.y * scale;
    } else if (!Characters->empty()) {
        line_height = Characters->begin()->second.Size.y * scale;
    }
    
    // Convert to wide string for Unicode support
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    std::u32string utf32;
    bool unicode_rendering = true;
    
    try {
        utf32 = conv.from_bytes(text);
    } catch (const std::range_error& e) {
        unicode_rendering = false;
    }
    
    if (unicode_rendering) {
        for (char32_t codepoint : utf32) {
            // Check if codepoint is in your Characters map
            auto it = Characters->find(codepoint);
            if (it == Characters->end()) continue;
            
            const Character& ch = it->second;
            
            // Handle newline character
            if (codepoint == U'\n') {
                x = 0.0f;
                y += line_height; 
                max_y = std::max(max_y, y + line_height);
                continue;
            }
            
            // Handle tab character
            if (codepoint == U'\t') {
                auto space_it = Characters->find(' ');
                if (space_it != Characters->end()) {
                    x += 8 * (space_it->second.Advance >> 6) * scale;
                }
                continue;
            }
            
            // Skip control characters
            if (codepoint < 32) {
                continue;
            }
            
            float char_width = (ch.Advance >> 6) * scale;
            float char_height = ch.Size.y * scale;
            
            // Check if character would exceed screen width (word wrapping)
            if (x + char_width > screen_width && x > 0) {
                x = 0.0f; 
                y += line_height;
            }
            
            // Update max_y to track the bottom of the content
            max_y = std::max(max_y, y + char_height);
            
            // Advance x position
            x += char_width;
        }
    } else {
        // Fallback to ASCII rendering
        for (char c : text) {
            auto it = Characters->find(c);
            if (it == Characters->end()) continue;
            
            const Character& ch = it->second;
            
            // Handle newline character
            if (c == '\n') {
                x = 0.0f;
                y += line_height;
                max_y = std::max(max_y, y + line_height);
                continue;
            }
            
            // Handle tab character
            if (c == '\t') {
                auto space_it = Characters->find(' ');
                if (space_it != Characters->end()) {
                    x += 8 * (space_it->second.Advance >> 6) * scale;
                }
                continue;
            }
            
            // Skip control characters
            if (c < 32 || c > 126) {
                continue;
            }
            
            float char_width = (ch.Advance >> 6) * scale;
            float char_height = ch.Size.y * scale;
            
            // Check if character would exceed screen width (word wrapping)
            if (x + char_width > screen_width && x > 0) {
                x = 0.0f;
                y += line_height;
            }
            
            // Update max_y to track the bottom of the content
            max_y = std::max(max_y, y + char_height);
            
            // Advance x position
            x += char_width;
        }
    }
    
    // Return the total height needed
    return max_y > 0 ? max_y : line_height;
}

// Convenience function for current active font
float calculate_content_height(const std::string& text, float scale, float screen_width) {
    if (current_font.empty()) {
        cerr << "LFH ERROR: No active font set" << endl;
        return 0.0f;
    }
    return calculate_content_height(current_font, text, scale, screen_width);
}

// Get list of loaded fonts
vector<string> get_loaded_fonts()
{
    vector<string> font_names;
    for (const auto& pair : fonts) {
        if (pair.second.loaded) {
            font_names.push_back(pair.first);
        }
    }
    return font_names;
}

// Unload a specific font
void unload_font(const string& font_name)
{
    auto font_it = fonts.find(font_name);
    if (font_it == fonts.end()) {
        return;
    }
    
    FontInfo& font_info = font_it->second;
    
    // Clean up character textures from all cached variations
    for (auto& cache_pair : font_info.character_cache) {
        for (auto& char_pair : cache_pair.second) {
            glDeleteTextures(1, &char_pair.second.TextureID);
        }
    }
    
    // Clean up FreeType face
    if (font_info.face) {
        FT_Done_Face(font_info.face);
    }
    
    // Remove from map
    fonts.erase(font_it);
    
    // If this was the current font, clear current font
    if (current_font == font_name) {
        current_font = "";
        // Optionally set to first available font
        if (!fonts.empty()) {
            current_font = fonts.begin()->first;
        }
    }
    
    cout << "Unloaded font: " << font_name << endl;
}

// Add cleanup function
void lfh_cleanup()
{
    // Clean up all fonts
    for (auto& pair : fonts) {
        FontInfo& font_info = pair.second;
        
        // Clean up character textures from all cached variations
        for (auto& cache_pair : font_info.character_cache) {
            for (auto& char_pair : cache_pair.second) {
                glDeleteTextures(1, &char_pair.second.TextureID);
            }
        }
        
        // Clean up FreeType face
        if (font_info.face) {
            FT_Done_Face(font_info.face);
        }
    }
    
    fonts.clear();
    current_font = "";

    glDeleteVertexArrays(1, &global_text_VAO);
    glDeleteBuffers(1, &global_text_VBO);
    global_text_VAO = 0;
    global_text_VBO = 0;
    
    // Clean up FreeType library
    FT_Done_FreeType(ft);
}