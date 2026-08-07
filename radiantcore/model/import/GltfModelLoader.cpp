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
        
        if (!file)
        {
            rError() << "Failed to load model " << path << std::endl;
            return IModelPtr();
        }

        
        //TODO: find a way to append the full absolute file path to the relative path.

        const auto gltfFileOpt = gltf::load("D:\\games\\monstergame\\base\\models\\radio.glb");
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

        for (int i = 0; i < gltfFile.meshes.size(); i++)
        {
            if (gltfFile.meshes[i].primitives.size() <= 0)
            {
                continue;
            }

            //TODO: 
            //parse the gltf model into DarkRadiant's StaticModel
            //refer to AseModelLoader.cpp FbxModelLoader.cpp for reference
            
        }

        
        

        

        std::vector<StaticModelSurfacePtr> staticSurfaces;

        auto staticModel = std::make_shared<StaticModel>(staticSurfaces);

        // Set the filename
        staticModel->setFilename(os::getFilename(file->getName()));
        staticModel->setModelPath(path);

        return staticModel; //Return the model.
    }

}
