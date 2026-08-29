#ifndef RENDERDEBUGMODE_H
#define RENDERDEBUGMODE_H

namespace Nova::App {

    enum class RenderDebugMode : int {
        Lit = 0,
        Wireframe,
        Normals,
        Positions,
        VertexColor,
        Depth,
        Count
    };

} // namespace Nova::App

#endif // RENDERDEBUGMODE_H