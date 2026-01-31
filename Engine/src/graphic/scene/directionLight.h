#pragma once
#include "platform.h"
#include <glm/glm.hpp>

KENSHIN_BEGIN

class DirectionLight
{
public:
	DirectionLight(const glm::vec4& direction, const glm::vec4& color);
	DirectionLight() = default;
	~DirectionLight() = default;
	void setDirection(const glm::vec4& direction);
	void setColor(const glm::vec4& color);
	const glm::vec4& getDirection() const;
	const glm::vec4& getColor() const;	
private:
	glm::vec4 mDirection	{ 0.5f, 0.5f, 0.5f, 0.0f };
	glm::vec4 mColor		{ 1.0f, 1.0f, 1.0f, 1.0f };
};

KENSHIN_END