//                    SPIKE ENGINE
//Copyright 2021 - SpikeTechnologies - All Rights Reserved
#include "spkpch.h"
#include "OpenGLConstantBuffer.h"
#include <glad/glad.h>

namespace Spike
{
    static GLenum SpikeDataUsageToOpenGLDataUsage(DataUsage usage)
    {
        switch (usage)
        {
            case DataUsage::DEFAULT: return GL_STATIC_DRAW;
            case DataUsage::DYNAMIC: return GL_DYNAMIC_DRAW;
        }
        SPK_CORE_LOG_ERROR("Unknown Spike DataUsage! Returning GL_DYNAMIC_DRAW...");
        return GL_DYNAMIC_DRAW;
    }

    OpenGLConstantBuffer::OpenGLConstantBuffer(const Ref<Shader>& shader, const String& name, void* data, const uint32_t size, const uint32_t bindSlot, ShaderDomain shaderDomain, DataUsage usage)
        :m_Name(name), m_Data(data), m_Size(size), m_BindSlot(bindSlot), m_ShaderDomain(shaderDomain), m_DataUsage(usage)
    {
        uint32_t index = glGetUniformBlockIndex((GLuint)shader->GetRendererID(), name.c_str());
        glUniformBlockBinding((GLuint)shader->GetRendererID(), index, bindSlot);
        uint32_t rendererID;

        glGenBuffers(1, &rendererID);
        glBindBuffer(GL_UNIFORM_BUFFER, rendererID);
        glBufferData(GL_UNIFORM_BUFFER, size, data, SpikeDataUsageToOpenGLDataUsage(usage));
        glBindBuffer(GL_UNIFORM_BUFFER, rendererID);
        glBindBufferBase(GL_UNIFORM_BUFFER, m_BindSlot, rendererID);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        m_RendererID = (RendererID)rendererID;
    }

    void OpenGLConstantBuffer::Bind()
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, m_BindSlot, (uint32_t)m_RendererID);
    }

    void OpenGLConstantBuffer::SetData(void* data)
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, m_BindSlot, (uint32_t)m_RendererID);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, m_Size, data);
    }

    OpenGLConstantBuffer::~OpenGLConstantBuffer()
    {
        uint32_t rendererID = reinterpret_cast<uint32_t>(m_RendererID);
        glDeleteBuffers(1, &rendererID);
    }
}