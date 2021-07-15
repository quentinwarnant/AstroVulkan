#pragma once

#include <optional>

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	std::optional<uint32_t> computeFamily;


	bool IsComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value();
	}
};