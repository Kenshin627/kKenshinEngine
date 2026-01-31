#pragma once
#include "platform.h"
#include <glm/glm.hpp>

KENSHIN_BEGIN

class Camera
{
public:
	Camera() = default;
	~Camera() = default;
	void setPosition(const glm::vec4& pos);
	void setViewMatrix(const glm::mat4& view);
	void setProjectionMatrix(const glm::mat4& projection);
	const glm::vec4& getPosition() const;
	const glm::mat4& getViewMatrix() const;
	const glm::mat4& getProjectionMatrix() const;
	const glm::mat4& getViewProjectionMatrix() const;
private:
	glm::vec4 mPosition;
	glm::mat4 mViewMatrix				{ glm::mat4(1.0) };
	glm::mat4 mProjectionMatrix			{ glm::mat4(1.0) };
	glm::mat4 mViewProjectionMatrix		{ glm::mat4(1.0) };
};

KENSHIN_END