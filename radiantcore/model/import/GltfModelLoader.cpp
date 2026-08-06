#include "GltfModelLoader.h"

#include <istream>
//#include "AseModel.h"

#include "os/path.h"
#include "string/case_conv.h"
#include "stream/ScopedArchiveBuffer.h"

#include "../StaticModel.h"
#include "parser/ParseException.h"
#include "../picomodel/PicoModelLoader.h"

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

        // Load the model data from the given stream
        archive::ScopedArchiveBuffer data(*file);

        
    }

}
