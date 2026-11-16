#include "stdafx.h"
#include "meshdata.h"

#include "OgreBitwise.h"
#include "OgreHlmsDatablock.h"
#include "OgreLogManager.h"
#include "OgreQuaternion.h"
#include "OgreSubItem.h"
#include "Vao/OgreAsyncTicket.h"


namespace
{

bool hasSemantic(const Ogre::VertexArrayObject* vao, Ogre::VertexElementSemantic semantic)
{
    for (const Ogre::VertexBufferPacked* buffer : vao->getVertexBuffers())
    {
        for (const Ogre::VertexElement2& element : buffer->getVertexElements())
        {
            if (element.mSemantic == semantic)
                return true;
        }
    }
    return false;
}

void readFloats(const char* src, Ogre::VertexElementType type, float* dst, int count)
{
    switch (type)
    {
    case Ogre::VET_FLOAT2:
    case Ogre::VET_FLOAT3:
    case Ogre::VET_FLOAT4:
        memcpy(dst, src, sizeof(float) * count);
        break;
    case Ogre::VET_HALF2:
    case Ogre::VET_HALF4:
    {
        const Ogre::uint16* halves = reinterpret_cast<const Ogre::uint16*>(src);
        for (int i = 0; i < count; ++i)
            dst[i] = Ogre::Bitwise::halfToFloat(halves[i]);
        break;
    }
    default:
        for (int i = 0; i < count; ++i)
            dst[i] = 0.0f;
        break;
    }
}

/// Ogre packs normal + tangent into a quaternion (QTangent): the normal is its X axis,
/// the tangent its Y axis, and the sign of w tells whether the bitangent is mirrored.
void readQTangent(const char* src, float* normal, float* tangent)
{
    const Ogre::int16* q = reinterpret_cast<const Ogre::int16*>(src);
    Ogre::Quaternion qTangent(q[3] / 32767.0f, q[0] / 32767.0f, q[1] / 32767.0f, q[2] / 32767.0f);
    qTangent.normalise();

    const Ogre::Vector3 n = qTangent.xAxis();
    const Ogre::Vector3 t = qTangent.yAxis();
    normal[0] = n.x; normal[1] = n.y; normal[2] = n.z;
    tangent[0] = t.x; tangent[1] = t.y; tangent[2] = t.z;
    tangent[3] = (q[3] < 0) ? -1.0f : 1.0f;
}

void readIndices(Ogre::VertexArrayObject* vao, std::vector<uint32_t>& outIndices, size_t numVertices)
{
    Ogre::IndexBufferPacked* indexBuffer = vao->getIndexBuffer();
    if (!indexBuffer)
    {
        outIndices.resize(numVertices);
        for (size_t i = 0; i < numVertices; ++i)
            outIndices[i] = uint32_t(i);
        return;
    }

    const size_t start = vao->getPrimitiveStart();
    const size_t count = vao->getPrimitiveCount();
    const bool is16bit = (indexBuffer->getIndexType() == Ogre::IndexBufferPacked::IT_16BIT);

    Ogre::AsyncTicketPtr ticket;
    const void* data = indexBuffer->getShadowCopy();
    if (data)
    {
        data = static_cast<const char*>(data) + start * indexBuffer->getBytesPerElement();
    }
    else
    {
        ticket = indexBuffer->readRequest(start, count);
        data = ticket->map();
    }

    outIndices.resize(count);
    for (size_t i = 0; i < count; ++i)
    {
        outIndices[i] = is16bit ? static_cast<const Ogre::uint16*>(data)[i]
                                : static_cast<const Ogre::uint32*>(data)[i];
    }

    if (ticket)
        ticket->unmap();
}

bool readVao(Ogre::VertexArrayObject* vao, MeshData::SubMesh& out)
{
    if (vao->getOperationType() != Ogre::OT_TRIANGLE_LIST || !hasSemantic(vao, Ogre::VES_POSITION))
        return false;

    out.hasNormals = hasSemantic(vao, Ogre::VES_NORMAL);
    out.hasTexcoords = hasSemantic(vao, Ogre::VES_TEXTURE_COORDINATES);

    Ogre::VertexArrayObject::ReadRequestsVec requests;
    requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
    if (out.hasNormals)
        requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_NORMAL));
    if (out.hasTexcoords)
        requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_TEXTURE_COORDINATES));

    const bool hasTangentElement = hasSemantic(vao, Ogre::VES_TANGENT);
    if (hasTangentElement)
        requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_TANGENT));

    vao->readRequests(requests);
    Ogre::VertexArrayObject::mapAsyncTickets(requests);

    const size_t numVertices = requests[0].vertexBuffer->getNumElements();
    out.vertices.resize(numVertices);

    for (size_t i = 0; i < numVertices; ++i)
    {
        MeshData::Vertex& v = out.vertices[i];
        v = MeshData::Vertex{ { 0, 0, 0 }, { 0, 1, 0 }, { 0, 0 }, { 1, 0, 0, 1 } };

        size_t r = 0;
        readFloats(requests[r].data + i * requests[r].vertexBuffer->getBytesPerElement(), requests[r].type, v.position, 3);
        ++r;
        if (out.hasNormals)
        {
            const char* src = requests[r].data + i * requests[r].vertexBuffer->getBytesPerElement();
            if (requests[r].type == Ogre::VET_SHORT4_SNORM)
            {
                readQTangent(src, v.normal, v.tangent);
                out.hasTangents = true;
            }
            else
            {
                readFloats(src, requests[r].type, v.normal, 3);
            }
            ++r;
        }
        if (out.hasTexcoords)
        {
            readFloats(requests[r].data + i * requests[r].vertexBuffer->getBytesPerElement(), requests[r].type, v.texcoord, 2);
            ++r;
        }
        if (hasTangentElement)
        {
            const Ogre::VertexElementType type = requests[r].type;
            if (type == Ogre::VET_FLOAT3 || type == Ogre::VET_FLOAT4)
            {
                readFloats(requests[r].data + i * requests[r].vertexBuffer->getBytesPerElement(), type, v.tangent,
                           type == Ogre::VET_FLOAT4 ? 4 : 3);
                if (type == Ogre::VET_FLOAT3)
                    v.tangent[3] = 1.0f;
                out.hasTangents = true;
            }
        }
    }

    Ogre::VertexArrayObject::unmapAsyncTickets(requests);

    readIndices(vao, out.indices, numVertices);
    out.indices.resize(out.indices.size() - out.indices.size() % 3);
    return true;
}

void collectItems(Ogre::SceneNode* node, std::vector<Ogre::Item*>& items)
{
    for (size_t i = 0; i < node->numAttachedObjects(); ++i)
    {
        Ogre::Item* item = dynamic_cast<Ogre::Item*>(node->getAttachedObject(i));
        if (item)
            items.push_back(item);
    }

    for (size_t i = 0; i < node->numChildren(); ++i)
    {
        Ogre::SceneNode* child = dynamic_cast<Ogre::SceneNode*>(node->getChild(i));
        if (child)
            collectItems(child, items);
    }
}

}

namespace MeshData
{

std::vector<SubMesh> readMesh(Ogre::Mesh* mesh)
{
    std::vector<SubMesh> result;
    for (unsigned i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        Ogre::SubMesh* subMesh = mesh->getSubMesh(i);

        SubMesh data;
        data.materialName = subMesh->getMaterialName();

        if (subMesh->mVao[Ogre::VpNormal].empty() || !readVao(subMesh->mVao[Ogre::VpNormal][0], data))
        {
            Ogre::LogManager::getSingleton().logMessage(
                "Skipping submesh " + std::to_string(i) + " of " + mesh->getName() + ": not a triangle list");
            // Keep the slot so submesh indices still line up with the item's sub items
            data.vertices.clear();
            data.indices.clear();
        }
        result.push_back(std::move(data));
    }
    return result;
}

std::vector<Object> readScene(Ogre::SceneNode* root)
{
    std::vector<Ogre::Item*> items;
    collectItems(root, items);

    std::vector<Object> result;
    for (Ogre::Item* item : items)
    {
        Object object;
        object.name = item->getName();
        if (object.name.empty())
            object.name = item->getMesh()->getName();
        object.worldTransform = item->getParentNode()->_getFullTransformUpdated();
        object.subMeshes = readMesh(item->getMesh().get());

        for (size_t i = 0; i < object.subMeshes.size() && i < item->getNumSubItems(); ++i)
        {
            Ogre::HlmsDatablock* datablock = item->getSubItem(i)->getDatablock();
            object.subMeshes[i].datablock = datablock;
            if (datablock && datablock->getNameStr())
                object.subMeshes[i].materialName = *datablock->getNameStr();
        }
        result.push_back(std::move(object));
    }
    return result;
}

}
