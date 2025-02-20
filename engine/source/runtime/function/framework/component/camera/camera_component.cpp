#include "runtime/function/framework/component/camera/camera_component.h"

#include "runtime/core/base/macro.h"
#include "runtime/core/math/math_headers.h"

#include "runtime/engine.h"
#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/input/input_system.h"
#include "runtime/function/render/include/render/glm_wrapper.h"
#include "runtime/function/scene/scene_manager.h"

namespace Pilot
{
    CameraComponent::CameraComponent(const CameraComponentRes& camera_param, GObject* parent_object) :
        Component {parent_object}, m_camera_param {camera_param}
    {
        const std::string& camera_type_name = camera_param.m_parameter.getTypeName();
        if (camera_type_name.compare("FirstPersonCameraParameter") == 0)
        {
            m_camera_mode              = CameraMode::first_person;
            m_camera_param.m_parameter = PILOT_REFLECTION_NEW(FirstPersonCameraParameter);
            *static_cast<FirstPersonCameraParameter*>(m_camera_param.m_parameter) =
                *static_cast<FirstPersonCameraParameter*>(camera_param.m_parameter.operator->());
        }
        else if (camera_type_name.compare("ThirdPersonCameraParameter") == 0)
        {
            m_camera_mode              = CameraMode::third_person;
            m_camera_param.m_parameter = PILOT_REFLECTION_NEW(ThirdPersonCameraParameter);
            *static_cast<ThirdPersonCameraParameter*>(m_camera_param.m_parameter) =
                *static_cast<ThirdPersonCameraParameter*>(camera_param.m_parameter.operator->());
        }
        else if (camera_type_name.compare("FreeCameraParameter") == 0)
        {
            m_camera_mode              = CameraMode::free;
            m_camera_param.m_parameter = PILOT_REFLECTION_NEW(FreeCameraParameter);
            *static_cast<FreeCameraParameter*>(m_camera_param.m_parameter) =
                *static_cast<FreeCameraParameter*>(camera_param.m_parameter.operator->());
        }
        else
        {
            LOG_ERROR("invalid camera type");
        }

        SceneManager::getInstance().setFOV(camera_param.m_parameter->m_fov);
    }

    void CameraComponent::tick(float delta_time)
    {
        if (g_is_editor_mode)
            return;

        switch (m_camera_mode)
        {
            case CameraMode::first_person:
                tickFirstPersonCamera(delta_time);
                break;
            case CameraMode::third_person:
                tickThirdPersonCamera(delta_time);
                break;
            case CameraMode::free:
                break;
            default:
                break;
        }
    }

    void CameraComponent::tickFirstPersonCamera(float delta_time)
    {
        Quaternion q_yaw, q_pitch;

        q_yaw.fromAngleAxis(InputSystem::getInstance().m_cursor_delta_yaw, Vector3::UNIT_Z);
        q_pitch.fromAngleAxis(InputSystem::getInstance().m_cursor_delta_pitch, m_left);

        TransformComponent* parent_transform = m_parent_object->tryGetComponent(TransformComponent);
        const float offset  = static_cast<FirstPersonCameraParameter*>(m_camera_param.m_parameter)->m_vertical_offset;
        Vector3     eye_pos = parent_transform->getPosition() + offset * Vector3::UNIT_Z;

        m_foward = q_yaw * q_pitch * m_foward;
        m_left   = q_yaw * q_pitch * m_left;
        m_up     = m_foward.crossProduct(m_left);

        // glm::vec3 eye = GLMUtil::fromVec3(eye_pos);
        // glm::vec3 forwad = GLMUtil::fromVec3(m_foward);
        // glm::vec3 up = GLMUtil::fromVec3(m_up);

        // Matrix4x4 desired_mat = GLMUtil::toMat4x4(glm::lookAtRH(eye, eye + forwad, up));

        Matrix4x4 desired_mat = Math::makeLookAtMatrix(eye_pos, m_foward, m_up);
        SceneManager::getInstance().setMainViewMatrix(desired_mat, PCurrentCameraType::Motor);

        Vector3    object_facing = m_foward - m_foward.dotProduct(Vector3::UNIT_Z) * Vector3::UNIT_Z;
        Vector3    object_left   = Vector3::UNIT_Z.crossProduct(object_facing);
        Quaternion object_rotation;
        object_rotation.fromAxes(object_left, -object_facing, Vector3::UNIT_Z);
        parent_transform->setRotation(object_rotation);
    }

    void CameraComponent::tickThirdPersonCamera(float delta_time)
    {
        ThirdPersonCameraParameter* para = static_cast<ThirdPersonCameraParameter*>(m_camera_param.m_parameter);

        Quaternion q_yaw, q_pitch;

        q_yaw.fromAngleAxis(InputSystem::getInstance().m_cursor_delta_yaw, Vector3::UNIT_Z);
        q_pitch.fromAngleAxis(InputSystem::getInstance().m_cursor_delta_pitch, m_left);

        para->m_cursor_pitch = para->m_cursor_pitch * q_pitch;

        TransformComponent* parent_transform  = m_parent_object->tryGetComponent(TransformComponent);
        const float         vertical_offset   = para->m_vertical_offset;
        const float         horizontal_offset = para->m_horizontal_offset;
        Vector3             offset            = Vector3(0, horizontal_offset, vertical_offset);

        parent_transform->setRotation(q_yaw * parent_transform->getRotation());

        Vector3 center_pos = parent_transform->getPosition() + Vector3::UNIT_Z * vertical_offset - 0.5f;
        Vector3 camera_pos =
            parent_transform->getRotation() * para->m_cursor_pitch * offset + parent_transform->getPosition();
        Vector3 camera_forward = center_pos - camera_pos;
        Vector3 camera_up      = parent_transform->getRotation() * para->m_cursor_pitch * Vector3::UNIT_Z;

        // glm::vec3 pos = camera_pos.toGLMVec3();
        // glm::vec3 forwad = camera_forward.toGLMVec3();
        // glm::vec3 up = camera_up.toGLMVec3();

        // auto desired_mat = glm::lookAtRH(pos, pos + forwad, up);
        Matrix4x4 desired_mat = Math::makeLookAtMatrix(camera_pos, camera_pos + camera_forward, camera_up);
        SceneManager::getInstance().setMainViewMatrix(desired_mat, PCurrentCameraType::Motor);
    }

} // namespace Pilot
