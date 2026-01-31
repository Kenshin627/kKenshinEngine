#include "pch.h"
#include "camera.h"

KENSHIN_BEGIN

void Camera::setPosition(const glm::vec4& pos)
{
	mPosition = pos;
}

void Camera::setViewMatrix(const glm::mat4& view)
{
	mViewMatrix = view;
	mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
}

void Camera::setProjectionMatrix(const glm::mat4& projection)
{
	mProjectionMatrix = projection;
	mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
}

const glm::vec4& Camera::getPosition() const
{
	return mPosition;
}

const glm::mat4& Camera::getViewMatrix() const
{
	return mViewMatrix;
}

const glm::mat4& Camera::getProjectionMatrix() const
{
	return mProjectionMatrix;
}

const glm::mat4& Camera::getViewProjectionMatrix() const
{
	return mViewProjectionMatrix;
}

KENSHIN_END
