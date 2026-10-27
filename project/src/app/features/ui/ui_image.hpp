#pragma once

#include <string>
#include <vector>

struct DecodedImage final
{
	int width{};
	int height{};
	std::vector<unsigned char> rgba{};
};

//png, jpg, bmp or tga to 8 bit rgba, error is filled in on failure
bool decodeImageFile(const std::string &path, DecodedImage &out, std::string &error);
