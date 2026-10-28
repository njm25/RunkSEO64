#include "ui_image.hpp"

#include <fstream>
#include <iterator>

#pragma warning (push, 0)

//files are read here and handed over as memory, so stb never touches fopen
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#define STBI_ONLY_TGA
#define STBI_MAX_DIMENSIONS 8192
#include "../../../shared/libs/stb/stb_image.hpp"

#pragma warning (pop)

bool decodeImageFile(const std::string &path, DecodedImage &out, std::string &error)
{
	std::ifstream file{ path, std::ios::binary };

	if (!file.is_open()) {
		error = "file not found";
		return false;
	}

	const std::vector<unsigned char> data{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };

	if (data.empty()) {
		error = "file is empty";
		return false;
	}

	int w{}, h{}, channels{};

	unsigned char *const pixels{ stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &w, &h, &channels, 4) };

	if (!pixels) {
		error = stbi_failure_reason() ? stbi_failure_reason() : "not an image";
		return false;
	}

	//dx9 cards can't be counted on past 4096
	if (w > 4096 || h > 4096)
	{
		stbi_image_free(pixels);
		error = "image is bigger than 4096x4096";

		return false;
	}

	out.width = w;
	out.height = h;
	out.rgba.assign(pixels, pixels + static_cast<size_t>(w) * h * 4);

	stbi_image_free(pixels);

	return true;
}
