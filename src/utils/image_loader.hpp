#include <iostream>
#include "lodepng.h"

#pragma once



struct ImageLoader
{

    std::vector<unsigned char> loaded_image;
    unsigned w,h;

    void loadImage(const char* filename) {

        std::vector<unsigned char> image; //the raw pixels
        unsigned int width, height;
        std::vector<unsigned char> png;
        

        //load and decode
        unsigned error = lodepng::load_file(png, filename);
        if(!error) error = lodepng::decode(image, width, height, png);

        //if there's an error, display it
        if(error) {
            std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
            throw "Skibidi TOilet";
        }

        loaded_image = image;

        w = width;
        h = height;
        std::cout << "Width:" << w << "\tHeight:" << h;
        std::cout << "\nImage Size:" << loaded_image.size();


        //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
        
    }

    bool isIn(double x, double y){
        int integer_x = x * this->w + this->w/2;
        int integer_y = y * this->h + this->h/2;

        if(integer_x < 0 || integer_x > w){return false;}
        if(integer_y < 0 || integer_y > h){return false;}

        unsigned int pixel_num = integer_x + integer_y*this->w;

        unsigned int abs_pixel = pixel_num * 4+3;

        if(abs_pixel >= loaded_image.size()){return false;}

       // std::cout << (int)loaded_image[absolute_pixel_num] << std::endl;
        double a = loaded_image[abs_pixel];

        return a;




    }
    sf::Color getValue(double x, double y){
        int integer_x = x * this->w + this->w/2;
        int integer_y = y * this->h + this->h/2;
        if(integer_x < 0 || integer_x > w){return {0,0,0};}
        if(integer_y < 0 || integer_y > h){return {0,0,0};}




        unsigned int pixel_num = integer_x + integer_y*this->w;

        unsigned int abs_pixel = pixel_num * 4;

        if(abs_pixel >= loaded_image.size()){return {0,0,0};}

       // std::cout << (int)loaded_image[absolute_pixel_num] << std::endl;
        double a = loaded_image[abs_pixel+3] / 255.0f;
        return sf::Color{loaded_image[abs_pixel],loaded_image[abs_pixel+1],loaded_image[abs_pixel+2]};



    }
	
};
