#if INTERFACE
#include <Engine/Includes/Standard.h>
class Shader {
public:

};
#endif

#include <Engine/Rendering/Shader.h>
#include <Engine/Diagnostics/Memory.h>

PUBLIC STATIC Shader* Shader::New() {
    Shader* texture = (Shader*)Memory::TrackedCalloc("Shader::Shader", 1, sizeof(Shader));
    return texture;
}
