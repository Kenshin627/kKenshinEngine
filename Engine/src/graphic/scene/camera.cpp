#include "pch.h"
#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

KENSHIN_BEGIN

void Camera::setPosition(const glm::vec4& pos)
{
	mPosition = pos;
	mViewMatrixDirty = true;
	mViewProjectionwMatrixDirty = true;
}

void Camera::setAspectRatio(float aspectRatio)
{
	mAspectRatio = aspectRatio;
	mProjectionMatrixDirty = true;
	mViewProjectionwMatrixDirty = true;
}

void Camera::setViewMatrix()
{
	mViewMatrix = glm::lookAt(glm::vec3(mPosition), mCenter, mUp);
	mViewMatrixDirty = false;
}

void Camera::setProjectionMatrix()
{
	mProjectionMatrix = glm::perspective(mFovY, mAspectRatio, mNear, mFar)	;
	mProjectionMatrixDirty = false;
}

const glm::vec4& Camera::getPosition() const
{
	return mPosition;
}

const glm::mat4& Camera::getViewMatrix()
{
	if (mViewMatrixDirty)
	{
		setViewMatrix();
	}
	return mViewMatrix;
}

const glm::mat4& Camera::getProjectionMatrix()
{
	if (mProjectionMatrixDirty)
	{
		setProjectionMatrix();
	}
	return mProjectionMatrix;
}

const glm::mat4& Camera::getViewProjectionMatrix()
{
	if (mViewProjectionwMatrixDirty)
	{
		mViewProjectionMatrix = getProjectionMatrix() * getViewMatrix();
		mViewProjectionwMatrixDirty = false;
	}
	return mViewProjectionMatrix;
}

void Camera::setCenter(const glm::vec3& center)
{
	mCenter = center;
	mViewMatrixDirty = true;
	mViewProjectionwMatrixDirty = true;
}

void Camera::setUp(const glm::vec3& up)
{
	mUp = glm::normalize(up);
	mViewMatrixDirty = true;
	mViewProjectionwMatrixDirty = true;
}

void Camera::setFovY(float fovY)
{
	mFovY = glm::radians(fovY);
	mProjectionMatrixDirty = true;
	mViewProjectionwMatrixDirty = true;
}

void Camera::setNear(float nearPlane)
{
	mNear = nearPlane;
	mProjectionMatrixDirty = true;
	mViewProjectionwMatrixDirty = true;
}

void Camera::setFar(float farPlane)
{
	mFar = farPlane;
	mProjectionMatrixDirty = true;
	mViewProjectionwMatrixDirty = true;
}

KENSHIN_END
