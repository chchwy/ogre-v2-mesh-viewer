#include "stdafx.h"
#include "gltfexporter.h"

#include <filesystem>
#include <map>

#include <QBuffer>
#include <QImage>

#include "OgreHlmsPbsDatablock.h"
#include "OgreImage2.h"
#include "OgreTextureBox.h"

#include "tiny_gltf.h"
#include "texturebutton.h"
#include "textureutil.h"


namespace
{

class Builder
{
public:
    tinygltf::Model model;

    Builder()
    {
        model.asset.version = "2.0";
        model.asset.generator = "ogre-v2-mesh-viewer";
        model.buffers.emplace_back();

        tinygltf::Scene scene;
        model.scenes.push_back(scene);
        model.defaultScene = 0;
    }

    void addObject(const MeshData::Object& object)
    {
        tinygltf::Mesh mesh;
        mesh.name = object.name;

        for (const MeshData::SubMesh& subMesh : object.subMeshes)
        {
            if (subMesh.vertices.empty() || subMesh.indices.empty())
                continue;
            mesh.primitives.push_back(makePrimitive(subMesh));
        }

        if (mesh.primitives.empty())
            return;

        model.meshes.push_back(mesh);

        tinygltf::Node node;
        node.name = object.name;
        node.mesh = int(model.meshes.size()) - 1;
        if (object.worldTransform != Ogre::Matrix4::IDENTITY)
        {
            // Written as translation/rotation/scale: more importers handle that than a raw matrix
            Ogre::Vector3 position, scale;
            Ogre::Quaternion orientation;
            object.worldTransform.decomposition(position, scale, orientation);

            node.translation = { position.x, position.y, position.z };
            node.rotation = { orientation.x, orientation.y, orientation.z, orientation.w };
            node.scale = { scale.x, scale.y, scale.z };
        }
        model.nodes.push_back(node);
        model.scenes[0].nodes.push_back(int(model.nodes.size()) - 1);
    }

private:
    std::map<Ogre::HlmsDatablock*, int> mMaterials;
    std::map<std::pair<Ogre::TextureGpu*, Ogre::TextureGpu*>, int> mTextures;

    int addBufferView(const void* data, size_t size, int target)
    {
        std::vector<unsigned char>& bytes = model.buffers[0].data;
        while (bytes.size() % 4u)
            bytes.push_back(0);

        tinygltf::BufferView view;
        view.buffer = 0;
        view.byteOffset = bytes.size();
        view.byteLength = size;
        view.target = target;

        const unsigned char* src = static_cast<const unsigned char*>(data);
        bytes.insert(bytes.end(), src, src + size);

        model.bufferViews.push_back(view);
        return int(model.bufferViews.size()) - 1;
    }

    int addFloatAccessor(const std::vector<float>& values, int components, bool withBounds)
    {
        tinygltf::Accessor accessor;
        accessor.bufferView = addBufferView(values.data(), values.size() * sizeof(float), TINYGLTF_TARGET_ARRAY_BUFFER);
        accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        accessor.count = values.size() / size_t(components);
        accessor.type = (components == 4) ? TINYGLTF_TYPE_VEC4 : (components == 3) ? TINYGLTF_TYPE_VEC3 : TINYGLTF_TYPE_VEC2;

        if (withBounds)
        {
            accessor.minValues.assign(size_t(components), std::numeric_limits<double>::max());
            accessor.maxValues.assign(size_t(components), std::numeric_limits<double>::lowest());
            for (size_t i = 0; i < values.size(); ++i)
            {
                const size_t c = i % size_t(components);
                accessor.minValues[c] = std::min(accessor.minValues[c], double(values[i]));
                accessor.maxValues[c] = std::max(accessor.maxValues[c], double(values[i]));
            }
        }

        model.accessors.push_back(accessor);
        return int(model.accessors.size()) - 1;
    }

    tinygltf::Primitive makePrimitive(const MeshData::SubMesh& subMesh)
    {
        std::vector<float> positions, normals, texcoords, tangents;
        positions.reserve(subMesh.vertices.size() * 3u);
        for (const MeshData::Vertex& v : subMesh.vertices)
        {
            positions.insert(positions.end(), v.position, v.position + 3);

            if (subMesh.hasNormals)
            {
                Ogre::Vector3 n(v.normal[0], v.normal[1], v.normal[2]);
                if (n.normalise() == 0.0f || n.isNaN())
                    n = Ogre::Vector3::UNIT_Y;
                normals.push_back(n.x);
                normals.push_back(n.y);
                normals.push_back(n.z);
            }
            if (subMesh.hasTexcoords)
                texcoords.insert(texcoords.end(), v.texcoord, v.texcoord + 2);
            if (subMesh.hasTangents)
            {
                Ogre::Vector3 t(v.tangent[0], v.tangent[1], v.tangent[2]);
                if (t.normalise() == 0.0f || t.isNaN())
                    t = Ogre::Vector3::UNIT_X;
                tangents.push_back(t.x);
                tangents.push_back(t.y);
                tangents.push_back(t.z);
                tangents.push_back(v.tangent[3] < 0.0f ? -1.0f : 1.0f);
            }
        }

        tinygltf::Primitive primitive;
        primitive.mode = TINYGLTF_MODE_TRIANGLES;
        primitive.attributes["POSITION"] = addFloatAccessor(positions, 3, true);
        if (subMesh.hasNormals)
            primitive.attributes["NORMAL"] = addFloatAccessor(normals, 3, false);
        if (subMesh.hasTexcoords)
            primitive.attributes["TEXCOORD_0"] = addFloatAccessor(texcoords, 2, false);
        if (subMesh.hasTangents)
            primitive.attributes["TANGENT"] = addFloatAccessor(tangents, 4, false);

        tinygltf::Accessor indices;
        indices.bufferView = addBufferView(subMesh.indices.data(), subMesh.indices.size() * sizeof(uint32_t),
                                           TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER);
        indices.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
        indices.count = subMesh.indices.size();
        indices.type = TINYGLTF_TYPE_SCALAR;
        model.accessors.push_back(indices);
        primitive.indices = int(model.accessors.size()) - 1;

        primitive.material = addMaterial(subMesh);
        return primitive;
    }

    static QImage readTexture(Ogre::TextureGpu* texture)
    {
        if (texture == nullptr)
            return QImage();

        Ogre::Image2 image;
        if (!TextureUtil::download(texture, image))
            return QImage();

        if (image.getPixelFormat() == Ogre::PFG_RG8_SNORM)
        {
            // Ogre keeps normal maps as signed XY; glTF wants the usual unsigned RGB encoding
            const Ogre::TextureBox box = image.getData(0);
            QImage result(int(box.width), int(box.height), QImage::Format_RGB32);
            for (Ogre::uint32 y = 0; y < box.height; ++y)
            {
                const signed char* src = static_cast<const signed char*>(box.at(0, y, 0));
                QRgb* dst = reinterpret_cast<QRgb*>(result.scanLine(int(y)));
                for (Ogre::uint32 x = 0; x < box.width; ++x)
                {
                    const float nx = std::max(src[x * 2 + 0] / 127.0f, -1.0f);
                    const float ny = std::max(src[x * 2 + 1] / 127.0f, -1.0f);
                    const float nz = std::sqrt(std::max(0.0f, 1.0f - nx * nx - ny * ny));
                    dst[x] = qRgb(int(nx * 127.5f + 127.5f), int(ny * 127.5f + 127.5f), int(nz * 127.5f + 127.5f));
                }
            }
            return result;
        }
        return TextureButton::toQtImage(image);
    }

    /// Embeds the image as PNG and returns the glTF texture index, or -1.
    int addImageTexture(const QImage& image, const std::string& name)
    {
        if (image.isNull())
            return -1;

        QByteArray png;
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        if (!image.save(&buffer, "PNG"))
            return -1;

        tinygltf::Image gltfImage;
        gltfImage.name = name;
        gltfImage.mimeType = "image/png";
        gltfImage.bufferView = addBufferView(png.constData(), size_t(png.size()), 0);
        model.images.push_back(gltfImage);

        if (model.samplers.empty())
            model.samplers.emplace_back();

        tinygltf::Texture texture;
        texture.source = int(model.images.size()) - 1;
        texture.sampler = 0;
        model.textures.push_back(texture);
        return int(model.textures.size()) - 1;
    }

    int addTexture(Ogre::TextureGpu* texture)
    {
        if (texture == nullptr)
            return -1;

        const auto key = std::make_pair(texture, static_cast<Ogre::TextureGpu*>(nullptr));
        const auto found = mTextures.find(key);
        if (found != mTextures.end())
            return found->second;

        const int index = addImageTexture(readTexture(texture), texture->getNameStr());
        mTextures[key] = index;
        return index;
    }

    /// glTF packs roughness in G and metalness in B of one texture; Ogre keeps two greyscale ones.
    int addMetalRoughTexture(Ogre::TextureGpu* roughness, Ogre::TextureGpu* metallic)
    {
        if (roughness == nullptr && metallic == nullptr)
            return -1;

        const auto key = std::make_pair(roughness, metallic);
        const auto found = mTextures.find(key);
        if (found != mTextures.end())
            return found->second;

        QImage roughImage = readTexture(roughness).convertToFormat(QImage::Format_Grayscale8);
        QImage metalImage = readTexture(metallic).convertToFormat(QImage::Format_Grayscale8);

        int index = -1;
        const QSize size = !roughImage.isNull() ? roughImage.size() : metalImage.size();
        if (!size.isEmpty())
        {
            if (!roughImage.isNull() && roughImage.size() != size)
                roughImage = roughImage.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            if (!metalImage.isNull() && metalImage.size() != size)
                metalImage = metalImage.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

            // A missing map is left at 1 so that the material's factor alone decides the value
            QImage packed(size, QImage::Format_RGB32);
            for (int y = 0; y < size.height(); ++y)
            {
                const uchar* rough = roughImage.isNull() ? nullptr : roughImage.constScanLine(y);
                const uchar* metal = metalImage.isNull() ? nullptr : metalImage.constScanLine(y);
                QRgb* dst = reinterpret_cast<QRgb*>(packed.scanLine(y));
                for (int x = 0; x < size.width(); ++x)
                    dst[x] = qRgb(255, rough ? rough[x] : 255, metal ? metal[x] : 255);
            }

            const std::string name = (roughness ? roughness : metallic)->getNameStr() + "_metalRough";
            index = addImageTexture(packed, name);
        }

        mTextures[key] = index;
        return index;
    }

    int addMaterial(const MeshData::SubMesh& subMesh)
    {
        const auto found = mMaterials.find(subMesh.datablock);
        if (found != mMaterials.end())
            return found->second;

        tinygltf::Material material;
        material.name = subMesh.materialName;

        const Ogre::HlmsPbsDatablock* pbs = dynamic_cast<Ogre::HlmsPbsDatablock*>(subMesh.datablock);
        if (pbs)
        {
            const Ogre::Vector3 diffuse = pbs->getDiffuse();
            const bool transparent = (pbs->getTransparencyMode() != Ogre::HlmsPbsDatablock::None);
            const float alpha = transparent ? pbs->getTransparency() : 1.0f;

            tinygltf::PbrMetallicRoughness& pbr = material.pbrMetallicRoughness;
            pbr.baseColorFactor = { diffuse.x, diffuse.y, diffuse.z, alpha };
            pbr.roughnessFactor = pbs->getRoughness();
            pbr.metallicFactor = (pbs->getWorkflow() == Ogre::HlmsPbsDatablock::MetallicWorkflow) ? pbs->getMetalness() : 0.0;
            pbr.baseColorTexture.index = addTexture(pbs->getTexture(Ogre::PBSM_DIFFUSE));
            pbr.metallicRoughnessTexture.index = addMetalRoughTexture(pbs->getTexture(Ogre::PBSM_ROUGHNESS),
                                                                      pbs->getTexture(Ogre::PBSM_METALLIC));

            material.normalTexture.index = addTexture(pbs->getTexture(Ogre::PBSM_NORMAL));
            material.normalTexture.scale = pbs->getNormalMapWeight();

            material.emissiveTexture.index = addTexture(pbs->getTexture(Ogre::PBSM_EMISSIVE));
            const Ogre::Vector3 emissive = pbs->getEmissive();
            material.emissiveFactor = { std::min(emissive.x, 1.0f), std::min(emissive.y, 1.0f), std::min(emissive.z, 1.0f) };

            material.doubleSided = pbs->getTwoSidedLighting();

            if (transparent)
            {
                material.alphaMode = "BLEND";
            }
            else if (pbs->getAlphaTest() != Ogre::CMPF_ALWAYS_PASS)
            {
                material.alphaMode = "MASK";
                material.alphaCutoff = pbs->getAlphaTestThreshold();
            }
        }

        model.materials.push_back(material);
        const int index = int(model.materials.size()) - 1;
        mMaterials[subMesh.datablock] = index;
        return index;
    }
};

}

bool GltfExporter::writeToFile(const std::vector<MeshData::Object>& objects, const QString& sOutFile)
{
    Builder builder;
    for (const MeshData::Object& object : objects)
        builder.addObject(object);

    if (builder.model.nodes.empty())
        return false;

    builder.model.buffers[0].name = "buffer";

    // A stream opened from a wide path keeps non-ASCII folder names working on Windows
    std::ofstream stream(std::filesystem::path(sOutFile.toStdWString()), std::ios::binary);
    if (!stream.is_open())
    {
        qDebug() << "Error: unable to write to file " << sOutFile;
        return false;
    }

    const bool binary = sOutFile.endsWith(".glb", Qt::CaseInsensitive);
    tinygltf::TinyGLTF writer;
    return writer.WriteGltfSceneToStream(&builder.model, stream, !binary, binary);
}
