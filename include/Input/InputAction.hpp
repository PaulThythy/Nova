#ifndef INPUT_ACTION_HPP
#define INPUT_ACTION_HPP

//TODO complete rework !
namespace Nova::Input {

    enum class InputAction {
        //mouse actions
        CameraMoveForward,          //mouse wheel
        CameraMoveBackward,         //mouse wheel
        CameraRotate,               //right click hold

        Camera2DTranslate,          //left MAJ pressed + left click hold
        CameraRotateLeft,           //A key
        CameraRotateRight,          //E key
    };

}

#endif //INPUT_ACTION_HPP