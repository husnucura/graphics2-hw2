This project demonstrates HDR rendering with cubemap environment lighting, featuring:

    Deferred shading pipeline

    Physically-based lighting

    HDR tone mapping (Reinhard operator)

    Motion blur effect

    Gamma correction

    Multiple rendering modes for debugging

Controls
Camera Controls

    Middle Mouse Button: Hold to rotate the camera view

    Mouse Movement: While middle mouse is held, move to look around

Rendering Controls

    Q: Quit the application

    V: Toggle VSync on/off

    B: Toggle motion blur effect

    G: Toggle gamma correction

    Arrow Up/Down: Increase/decrease exposure (2x steps)

    Arrow Left/Right: Decrease/increase tone mapping key value
    
    Space: Toggle fullscreen/windowed

Rendering Mode Selection

    0: Tonemapped final output (default)

    1: Cubemap only view

    2: World positions visualization

    3: Normals visualization

    4: Deferred lighting pass

    5: Composite view (cubemap + model)

    6: Composite with motion blur

Features

    HDR environment lighting using cubemap

    Deferred rendering pipeline

    Physically-based lighting calculations

    Reinhard tone mapping operator

    Camera-space motion blur effect

    Gamma correction

    Multiple debug visualization modes

    Real-time FPS counter and status display

Requirements

    OpenGL 3.3+ compatible GPU

    GLFW3

    GLEW

    GLM

    FreeType (for text rendering)

    stb_image (for HDR image loading)

Building

    Install required dependencies

    Compile with a C++11 compatible compiler

    Link against GLFW, GLEW, OpenGL, and FreeType libraries

Assets

The program expects to find:

    armadillo.obj model file

    HDR cubemap textures in cubemap/ directory (px.hdr, nx.hdr, py.hdr, ny.hdr, pz.hdr, nz.hdr)

Performance Notes

    VSync can be toggled with V key

    Motion blur effect is automatically triggered by camera movement

    Higher exposure values may require tone mapping adjustment