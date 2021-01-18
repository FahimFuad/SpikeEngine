/*
                               SPIKE ENGINE
               This file is licensed under the SPIKE LICENSE
                          --Spike Source Code--

         Copyright 2021 - SpikeTechnologies - All Rights Reserved

File Name      : EditorLayer
File Type      : cpp
File created on: 2021/01/09
File created by: Fahim Fuad
Other editors  : None
Github repository : https://github.com/FahimFuad/Spike
 
0.This software is provided 'AS-IS', without any express or implied warranty.
  In no event will the authors or contributors be held liable for any damages
  arising from the use of this software.

1.The origin of this software must not be misrepresented; you must not claim/
  that you wrote the original software.
 
2.You MUST NOT change or alter this file. This excludes the contributions done
  by people. Changing this file is PERFECTLY LEGAL if you are contributing.

3. THIS NOTICE MAY NOT BE REMOVED OR ALTERED FROM ANY SOURCE DISTRIBUTION.
*/
#include "EditorLayer.h"
#include "Panels/ConsolePanel.h"
#include "Spike/Scene/SceneSerializer.h"
#include "Spike/Utility/PlatformUtils.h"
#include "Spike/Math/Math.h"
#include "FontAwesome.h"

#include <imgui/imgui.h>
#include <ImGuizmo.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#pragma warning(push)
#pragma warning(disable : 4244) //Disable ugly 'C4244' "type conversion" warning!

namespace Spike
{
    EditorLayer::EditorLayer()
        : Layer("EditorLayer")
    {
    }

    void EditorLayer::OnAttach()
    {
        LE_PROFILE_FUNCTION();
        m_CheckerboardTexture = Texture2D::Create("Spike/assets/textures/Checkerboard.png");

        FramebufferSpecification fbSpec;
        fbSpec.Width = 1280;
        fbSpec.Height = 720;
        m_Framebuffer = Framebuffer::Create(fbSpec);
        m_IDFramebuffer = Framebuffer::Create(fbSpec);

        m_EditorScene = Ref<Scene>::Create();
        m_EditorCamera = EditorCamera(45.0f, 1.778f, 0.1f, 1000.0f);
        m_SceneHierarchyPanel.SetContext(m_EditorScene);
    }

    void EditorLayer::OnDetach()
    {
        LE_PROFILE_FUNCTION();
    }


    void EditorLayer::OnScenePlay()
    {
        m_SceneHierarchyPanel.ClearSelectedEntity();

        m_SceneState = SceneState::Play;

        m_RuntimeScene = Ref<Scene>::Create();
        m_EditorScene->CopySceneTo(m_RuntimeScene);

        m_RuntimeScene->OnRuntimeStart();
        m_SceneHierarchyPanel.SetContext(m_RuntimeScene);
    }

    void EditorLayer::OnSceneStop()
    {
        m_RuntimeScene->OnRuntimeStop();
        m_SceneState = SceneState::Edit;

        // Unload runtime scene
        m_RuntimeScene = nullptr;

        m_SceneHierarchyPanel.ClearSelectedEntity();
        m_SceneHierarchyPanel.SetContext(m_EditorScene);
    }

    void EditorLayer::OnUpdate(Timestep ts)
    {
        LE_PROFILE_FUNCTION();
        m_FrameTime = ts;
        // Resize
        if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();
            m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
            (spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
        {
            m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            m_IDFramebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
            m_EditorScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
        }


        // Render
        Renderer2D::ResetStats();
        m_Framebuffer->Bind();
        RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
        RenderCommand::Clear();
        m_Framebuffer->Bind();

        switch (m_SceneState)
        {
        case EditorLayer::SceneState::Edit:
            {
                m_EditorCamera.OnUpdate(ts);
                m_EditorScene->OnUpdateEditor(ts, m_EditorCamera);
                break;
            }
        case EditorLayer::SceneState::Play:
            {
                if (m_ViewportFocused)
                    m_EditorCamera.OnUpdate(ts);

                m_RuntimeScene->OnUpdate(ts);
                m_RuntimeScene->OnUpdateRuntime(ts);
                break;
            }
        case EditorLayer::SceneState::Pause:
            {
                if (m_ViewportFocused)
                    m_EditorCamera.OnUpdate(ts);

                m_RuntimeScene->OnUpdateRuntime(ts);
                break;
            }
        }

        auto [mx, my] = ImGui::GetMousePos();
        mx -= m_ViewportBounds[0].x;
        my -= m_ViewportBounds[0].y;
        auto viewportWidth = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
        auto viewportHeight = m_ViewportBounds[1].y - m_ViewportBounds[0].y;
        my = viewportHeight - my;
        
        int mouseX = (int)mx;
        int mouseY = (int)my;
        
        if (mouseX >= 0 && mouseY >= 0 && mouseX < viewportWidth && mouseY < viewportHeight)
        {
            int pixel = m_EditorScene->Pixel(mx, my);
            m_HoveredEntity = pixel == -1 ? Entity() : Entity((entt::entity)pixel, m_EditorScene.Raw());
        }
        m_Framebuffer->Unbind();
    }

    void EditorLayer::OnImGuiRender()
    {
        LE_PROFILE_FUNCTION();

        static bool dockspaceOpen = true;
        static bool opt_fullscreen_persistant = true;
        bool opt_fullscreen = opt_fullscreen_persistant;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // DockSpace
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 270.0f;
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        style.WindowMinSize.x = minWinSizeX;

        Console::Get()->OnImGuiRender();

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New", "Ctrl+N"))
                    NewScene();

                if (ImGui::MenuItem("Open...", "Ctrl+O"))
                    OpenScene();

                if (ImGui::MenuItem("Save", "Ctrl+S"))
                    SaveScene();

                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                    SaveSceneAs();

                if (ImGui::MenuItem("Exit"))
                    Application::Get().Close();
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        m_SceneHierarchyPanel.OnImGuiRender();

        ImGui::Begin("Stats");
        auto stats = Renderer2D::GetStats();
        //ImGui::Text("Renderer2D Stats:");
        //ImGui::Text("Draw Calls: %d", stats.DrawCalls);
        //ImGui::Text("Quads: %d", stats.QuadCount);
        //ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
        //ImGui::Text("Indices: %d", stats.GetTotalIndexCount());
        static float frameTimeRefreshTimer = 0.0f;
        static float ft = 0.0f;
        static float frameRate = 0.0f;
        frameTimeRefreshTimer += m_FrameTime;
        if (frameTimeRefreshTimer >= 0.25f)
        {
            ft = m_FrameTime;
            frameRate = 1.0f / m_FrameTime;
            frameTimeRefreshTimer = 0.0f;
        }
        auto& caps = RendererAPI::GetCapabilities();

        ImGui::Text("Vendor: %s", caps.Vendor.c_str());
        ImGui::Text("Renderer: %s", caps.Renderer.c_str());

        ImGui::Text("FrameTime: %.3f ms", ft);
        ImGui::Text("FPS: %d", (int)frameRate);
        ImGui::End();

        bool show = true;
        ImGui::Begin("ToolBar", &show, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2);
        if (m_SceneState == SceneState::Edit)
        {
            if (ImGui::Button("Play"))
            {
                OnScenePlay();
            }
        }
        else if (m_SceneState == SceneState::Play)
        {
            if (ImGui::Button("Stop"))
            {
                OnSceneStop();
            }
        }
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin(ICON_FK_GAMEPAD" Viewport");

        auto viewportOffset = ImGui::GetCursorPos();

        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();
        Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused || !m_ViewportHovered);

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

        uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
        ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

        auto windowSize = ImGui::GetWindowSize();
        ImVec2 minBound = ImGui::GetWindowPos();
        minBound.x += viewportOffset.x;
        minBound.y += viewportOffset.y;

        ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y+ windowSize.y };
        m_ViewportBounds[0] = { minBound.x, minBound.y };
        m_ViewportBounds[1] = { maxBound.x, maxBound.y };

        // Gizmos
        Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
        if (selectedEntity && m_GizmoType != -1)
        {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            float windowWidth = (float)ImGui::GetWindowWidth();
            float windowHeight = (float)ImGui::GetWindowHeight();
            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

            // Camera
            // auto cameraEntity = m_ActiveScene->GetPrimaryCameraEntity();
            // const auto& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
            // const glm::mat4& cameraProjection = camera.GetProjection();
            // glm::mat4 cameraView = glm::inverse(cameraEntity.GetComponent<TransformComponent>().GetTransform());

            // Editor camera
            const glm::mat4& cameraProjection = m_EditorCamera.GetProjection();
            glm::mat4 cameraView = m_EditorCamera.GetViewMatrix();

            // Entity transform
            auto& tc = selectedEntity.GetComponent<TransformComponent>();
            glm::mat4 transform = tc.GetTransform();

            // Snapping
            bool snap = Input::IsKeyPressed(Key::LeftControl);
            float snapValue = 0.5f; // Snap to 0.5m for translation/scale
            // Snap to 45 degrees for rotation
            if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
                snapValue = 45.0f;

            float snapValues[3] = { snapValue, snapValue, snapValue };

            ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                (ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
                nullptr, snap ? snapValues : nullptr);

            if (ImGuizmo::IsUsing())
            {
                m_GizmoInUse = true;
                glm::vec3 translation, rotation, scale;
                Math::DecomposeTransform(transform, translation, rotation, scale);

                glm::vec3 deltaRotation = rotation - tc.Rotation;
                tc.Translation = translation;
                tc.Rotation += deltaRotation;
                tc.Scale = scale;
            }
            else
            {
                m_GizmoInUse = false;
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::End();

    }

    void EditorLayer::OnEvent(Event& e)
    {
        m_SceneHierarchyPanel.OnEvent(e);
        m_EditorCamera.OnEvent(e);
        m_EditorScene->OnEvent(e);
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>(SPK_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonPressedEvent>(SPK_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
    {
        // Shortcuts
        if (e.GetRepeatCount() > 0)
            return false;

        bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
        bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
        switch (e.GetKeyCode())
        {
            case Key::N:
            {
                if (control)
                    NewScene();

                break;
            }
            case Key::O:
            {
                if (control)
                    OpenScene();

                break;
            }
            case Key::S:
            {
                if (control && shift)
                    SaveSceneAs();
                if (control)
                    SaveScene();
                break;
            }

            // Gizmos
            case Key::Q:
                if (!m_GizmoInUse)
                {
                    m_GizmoType = -1;
                }
                break;
            case Key::W:
                if (!m_GizmoInUse)
                {
                    m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
                }
                break;
            case Key::E:
                if (!m_GizmoInUse)
                {
                    m_GizmoType = ImGuizmo::OPERATION::ROTATE;
                }
                break;
            case Key::R:
                if (!m_GizmoInUse)
                {
                    m_GizmoType = ImGuizmo::OPERATION::SCALE;
                }
                break;
        }
        return false;
    }

    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
    {
        if (e.GetMouseButton() == Mouse::ButtonLeft && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))
        {
            m_SceneHierarchyPanel.SetSelectedEntity(m_HoveredEntity);
        }
        return false;
    }

    void EditorLayer::NewScene()
    {
        m_EditorScene = Ref<Scene>::Create();
        m_EditorScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
        m_SceneHierarchyPanel.SetContext(m_EditorScene);
        m_FirstTimeSave = true;
        Console::Get()->Print("Successfully created new scene!", Console::LogLevel::LVL_INFO);
    }

    void EditorLayer::OpenScene()
    {
        std::string filepath = FileDialogs::OpenFile("Spike Scene (*.spike)\0*.spike\0");
        if (!filepath.empty())
        {
            m_FirstTimeSave = false;
            m_ActiveFilepath = filepath;
            m_EditorScene = Ref<Scene>::Create();
            m_EditorScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            m_SceneHierarchyPanel.SetContext(m_EditorScene);

            SceneSerializer serializer(m_EditorScene);
            serializer.Deserialize(filepath);
            Console::Get()->Print("Succesfully deserialized scene!", Console::LogLevel::LVL_INFO);
        }
    }

    void EditorLayer::SaveSceneAs()
    {
        std::string filepath = FileDialogs::SaveFile("Spike Scene (*.spike)\0*.spike\0");
        if (!filepath.empty())
        {
            m_FirstTimeSave = false;
            SceneSerializer serializer(m_EditorScene);
            serializer.Serialize(filepath);
            Console::Get()->Print("Scene serialized succesfully!", Console::LogLevel::LVL_INFO);
        }
    }

    void EditorLayer::SaveScene()
    {
        if (m_FirstTimeSave)
        {
            SaveSceneAs();
        }
        else
        {
            SceneSerializer serializer(m_EditorScene);
            serializer.Serialize(m_ActiveFilepath);
            Console::Get()->Print("Scene Saved!", Console::LogLevel::LVL_INFO);
        }
    }
}
#pragma warning (pop) // Pop the warning
