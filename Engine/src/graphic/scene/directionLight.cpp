#include "pch.h"
#include "directionLight.h"

KENSHIN_BEGIN

DirectionLight::DirectionLight(const glm::vec4& direction, const glm::vec4& color)
	: mDirection(direction), mColor(color)
{
	mDirection = glm::normalize(mDirection);
}

void DirectionLight::setDirection(const glm::vec4& direction)
{
	mDirection = glm::normalize(direction);
}

void DirectionLight::setColor(const glm::vec4& color)
{
	mColor = color;
}

const glm::vec4& DirectionLight::getDirection() const
{
	return mDirection;
}

const glm::vec4& DirectionLight::getColor() const
{
	return mColor;
}

KENSHIN_END
