#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>

struct ImageData {
    int* data;
    int width;
    int height;
    int channels;
};

ImageData loadImageToArray(const char* filename) {
    ImageData result = {nullptr, 0, 0, 0};
    
    // Load the image using stb_image
    unsigned char* img = stbi_load(filename, &result.width, &result.height, &result.channels, 3);  // Force 3 channels (RGB)
    
    if (img == nullptr) {
        std::cerr << "Error loading image: " << filename << std::endl;
        return result;
    }
    
    // Allocate new array for the int data
    int totalPixels = result.width * result.height * 3;
    result.data = new int[totalPixels];
    
    // Convert unsigned char (0-255) to int
    for (int i = 0; i < totalPixels; i++) {
        result.data[i] = static_cast<int>(img[i]);
    }
    
    // Free the original image data
    stbi_image_free(img);
    
    return result;
}

// Don't forget to free the memory when done!
void freeImageData(ImageData& imageData) {
    delete[] imageData.data;
    imageData.data = nullptr;
}

