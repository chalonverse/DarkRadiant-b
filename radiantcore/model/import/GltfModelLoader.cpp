#include "GltfModelLoader.h"

#include <istream>
//#include "AseModel.h"

#include "os/path.h"
#include "string/case_conv.h"
#include "stream/ScopedArchiveBuffer.h"

#include "../StaticModel.h"
#include "parser/ParseException.h"
#include "../picomodel/PicoModelLoader.h"

//https://github.com/pfirsich/gltf
#include "gltf.h"
#include "../StaticModelSurface.h"

namespace model
{

    GltfModelLoader::GltfModelLoader() :
        ModelImporterBase("GLB")
    {}

    IModelPtr GltfModelLoader::loadModelFromPath(const std::string& path)
    {
        // Open an ArchiveFile to load
        auto file = path_is_absolute(path.c_str()) ?
            GlobalFileSystem().openFileInAbsolutePath(path) :
            GlobalFileSystem().openFile(path);
        
        std::string fileName = path_is_absolute(path.c_str()) ?
            path : GlobalFileSystem().findFile(path) + path;

        const auto gltfFileOpt = gltf::load(fileName.c_str());
        if (!gltfFileOpt)
        {
            //Failed to load
            rError() << "Failed to parse GLB file " << path << std::endl;
            return IModelPtr();
        }
        
        const auto& gltfFile = *gltfFileOpt;
        if (gltfFile.scenes.size() > 1)
        {
            //Too many scenes
            rError() << "GLB file has too many scenes " << path << std::endl;
            return IModelPtr();
        }

        //gltfFile.materials.size()        
        
        //Note: "mesh" = how many individual blender objects there are.
        //so if the model consists of 3 objects, then: you have 3 meshes.

        if (gltfFile.meshes.size() <= 0)
        {
            //No mesh
            rError() << "GLB file has no meshes " << path << std::endl;
            return IModelPtr();
        }

        std::vector<StaticModelSurfacePtr> staticSurfaces;

        for (size_t i = 0; i < gltfFile.meshes.size(); i++)
        {
            if (gltfFile.meshes[i].primitives.size() <= 0)
            {
                continue;
            }

            for (size_t j = 0; j < gltfFile.meshes[i].primitives.size(); j++)
            {
                auto& primitive = gltfFile.meshes[i].primitives[j];
                size_t numVerts = 0;
                size_t numIndices = 0;
                gltf::Accessor::ComponentType indexType = gltf::Accessor::ComponentType::UnsignedShort;
                std::pair<const uint8_t*, size_t> positions;
                std::pair<const uint8_t*, size_t> normals;
                std::pair<const uint8_t*, size_t> texcoords;
                std::pair<const uint8_t*, size_t> indicesPtr;

                for (size_t k = 0; k < primitive.attributes.size(); k++)
                {
                    auto& attribute = primitive.attributes[k];
                    if (attribute.id == "POSITION")
                    {
                        if (gltfFile.accessors[attribute.accessor].componentType != gltf::Accessor::ComponentType::Float)
                        {
                            rError() << "GLB file has a position attribute that doesn't use floats " << path << std::endl;
                            return IModelPtr();
                        }
                        if (gltfFile.accessors[attribute.accessor].type != gltf::Accessor::Type::Vec3)
                        {
                            rError() << "GLB file has a position attribute that isn't a Vec3 " << path << std::endl;
                            return IModelPtr();
                        }

                        numVerts = gltfFile.accessors[attribute.accessor].count;
                        positions = gltfFile.getAccessorData(attribute.accessor);
                        
                    }
                    else if (attribute.id == "NORMAL")
                    {
                        if (gltfFile.accessors[attribute.accessor].componentType != gltf::Accessor::ComponentType::Float)
                        {
                            rError() << "GLB file has a normal attribute that doesn't use floats " << path << std::endl;
                            return IModelPtr();
                        }
                        if (gltfFile.accessors[attribute.accessor].type != gltf::Accessor::Type::Vec3)
                        {
                            rError() << "GLB file has a normal attribute that isn't a Vec3 " << path << std::endl;
                            return IModelPtr();
                        }

                        normals = gltfFile.getAccessorData(attribute.accessor);
                    }
                    else if (attribute.id == "TEXCOORD_0")
                    {
                        if (gltfFile.accessors[attribute.accessor].componentType != gltf::Accessor::ComponentType::Float)
                        {
                            rError() << "GLB file has a texcoord_0 attribute that doesn't use floats " << path << std::endl;
                            return IModelPtr();
                        }
                        if (gltfFile.accessors[attribute.accessor].type != gltf::Accessor::Type::Vec2)
                        {
                            rError() << "GLB file has a texcoord_0 attribute that isn't a Vec2 " << path << std::endl;
                            return IModelPtr();
                        }

                        texcoords = gltfFile.getAccessorData(attribute.accessor);
                    }
                }

                if (primitive.indices)
                {
                    indicesPtr = gltfFile.getAccessorData(*primitive.indices);
                    numIndices = gltfFile.accessors[*primitive.indices].count;
                    indexType = gltfFile.accessors[*primitive.indices].componentType;

                    std::vector<MeshVertex> vertices;
                    std::vector<unsigned int> indices;
                    indices.resize(numIndices);

                    if (indexType == gltf::Accessor::ComponentType::UnsignedByte)
                    {
                        for (size_t index = 0; index < numIndices; index++)
                        {
                            indices[index] = indicesPtr.first[index];
                        }
                    }
                    else if (indexType == gltf::Accessor::ComponentType::UnsignedShort)
                    {
                        const uint16_t* ptrAs16 = reinterpret_cast<const uint16_t*>(indicesPtr.first);
                        for (size_t index = 0; index < numIndices; index++)
                        {
                            indices[index] = ptrAs16[index];
                        }
                    }
                    else if (indexType == gltf::Accessor::ComponentType::UnsignedInt)
                    {
                        const uint32_t* ptrAs32 = reinterpret_cast<const uint32_t*>(indicesPtr.first);
                        for (size_t index = 0; index < numIndices; index++)
                        {
                            indices[index] = ptrAs32[index];
                        }
                    }
                    else
                    {
                        rError() << "GLB file doesn't indices of an unsigned type " << path << std::endl;
                        return IModelPtr();
                    }

                    // GLTF is CCW winding order, but dark radiant wants CW
                    size_t numTris = numIndices / 3;
                    for (size_t tri = 0; tri < numTris; tri++)
                    {
                        std::swap(indices[tri * 3], indices[tri * 3 + 1]);
                    }

                    vertices.reserve(numVerts);
                    const gltf::vec3* posVec3 = reinterpret_cast<const gltf::vec3*>(positions.first);
                    const gltf::vec3* normalsVec3 = reinterpret_cast<const gltf::vec3*>(normals.first);
                    const std::array<float, 2>* texcoordsVec2 = reinterpret_cast<const std::array<float, 2>*>(texcoords.first);
                    for (size_t vertex = 0; vertex < numVerts; vertex++)
                    {
                        // NOTE: Normals are negated due to winding order swap from CCW to CW
                        vertices.emplace_back(posVec3 ? Vertex3(posVec3[vertex][0], posVec3[vertex][1], posVec3[vertex][2]) : Vertex3(),
                            normalsVec3 ? Normal3(-normalsVec3[vertex][0], -normalsVec3[vertex][1], -normalsVec3[vertex][2]) : Normal3(),
                            texcoordsVec2 ? TexCoord2f(texcoordsVec2[vertex][0], texcoordsVec2[vertex][1]) : TexCoord2f());
                    }

                    auto& staticSurface = staticSurfaces.emplace_back(std::make_shared<StaticModelSurface>(std::move(vertices), std::move(indices)));
                    if (primitive.material && gltfFile.materials[*primitive.material].name)
                    {
                        staticSurface->setDefaultMaterial(*gltfFile.materials[*primitive.material].name);
                        staticSurface->setActiveMaterial(staticSurface->getActiveMaterial());
                    }
                }
                else
                {
                    rError() << "GLB file doesn't seem to have indices " << path << std::endl;
                    return IModelPtr();
                }
            }
        }

        auto staticModel = std::make_shared<StaticModel>(staticSurfaces);

        // Set the filename
        staticModel->setFilename(os::getFilename(fileName));
        staticModel->setModelPath(path);

        return staticModel; //Return the model.
    }

}
