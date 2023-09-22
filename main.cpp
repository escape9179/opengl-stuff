#include <windows.h>
#include <gl/glew.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>
#include "lodepng.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

int getShaderCompileStatus(GLuint shader) {
    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if(isCompiled == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(shader); // Don't leak the shader.
    }
    return isCompiled;
}

char *readFile(char *path) {
    std::ifstream stream(path, std::ios::ate); // Create an input file stream and go to the end of the file.
    auto size = stream.tellg(); // Get the current position of the stream (should be end of file because 'ate' was specified).
    stream.seekg(0); // Go to the beginning of the file.
    auto *buffer = new std::string(size, '\0'); // Create a string to use a buffer.
    stream.read(buffer->data(), size); // Read all the contents of the file into the buffer in one go.
    return buffer->data(); // Return the underlying data of the string a char pointer.
}

struct Vertex {
    /* Position */
    float x;
    float y;
    float z;
    float w;

    /* Color */
    float r;
    float g;
    float b;
    float a;
};

struct Vector4f {
    float x;
    float y;
    float z;
    float w;
};

void GLAPIENTRY debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n", (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

HGLRC hglrc; // Handle to an OpenGL rendering context.
bool running = true; // Used for the game loop.
/* Vertex Data to feed into vertex buffer. */
static const Vertex vertexData[] = {
        {-1, -1, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0},
        {1, -1, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0},
        {0, 1, 0.0, 1.0, 0.5, 1.0, 1.0, 1.0}
};

static const Vertex blockVertexData[] = {
        /* Front face*/
        {-1.0, -1.0, -1.0, 1.0, 0.0, 0.0, 0.0, 1.0},
        {1.0, -1.0, -1.0, 1.0, 1.0, 0.0, 0.0, 1.0},
        {1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 0.0, 1.0},
        {-1.0, -1.0, -1.0, 1.0, 0.0, 0.0, 0.0, 1.0},
        {1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 0.0, 1.0},
        {-1.0, 1.0, -1.0, 1.0, 0.0, 1.0, 0.0, 1.0},

        /* Left face */
        {-1.0, -1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0},
        {-1.0, -1.0, -1.0, 1.0, 1.0, 0.0, 0.0, 1.0},
        {-1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 0.0, 1.0},
        {-1.0, -1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0},
        {-1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 0.0, 1.0},
        {-1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0, 1.0},

        /* Back face */
        {-1.0, -1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0},
        {1.0, -1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0},
        {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 1.0},
        {-1.0, -1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0},
        {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 1.0},
        {-1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0, 1.0},

        /* Right face */
        {-1.0, -1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0},
        {-1.0, -1.0, -1.0, 1.0, 1.0, 0.0, 0.0, 1.0},
        {-1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 0.0, 1.0},
        {-1.0, -1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0},
        {-1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 0.0, 1.0},
        {-1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0, 1.0},
};

float aspect = 1.0f;

int main(int argc, char *argv[]) {
    WinMain(nullptr, nullptr, argv[0], SHOW_OPENWINDOW);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char *args, int iCmdShow) {
    /* Named used for the window class. Any windows that want similar functionality will probably use this class name
     * when creating an instance of the window.*/
    char *appName = "App";
    WNDCLASS wndclass;
    wndclass.style = CS_OWNDC; // Creates a private device context. This prevents us from having to get the DC
                                // and release it whenever we need to do something with the screen. This is useful
                                // because we're going to be drawing to the screen a lot.
    wndclass.lpfnWndProc = WndProc; // Pointer to the window procedure.
    wndclass.cbClsExtra = 0; // Extra amount of bytes to allocate at the end of the window class.
    wndclass.cbWndExtra = 0; // Extra amount of bytes to allocate at the end of the window instance.
    wndclass.hInstance = hInstance;
    wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndclass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = nullptr;
    wndclass.lpszClassName = appName;

    if (!RegisterClass(&wndclass)) {
        MessageBox(nullptr, "Error registering window class.", "Error", MB_ICONERROR | MB_OK);
        return 0;
    }

    HWND hwnd = CreateWindow(
            appName,
            "Window",
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            nullptr,
            nullptr,
            hInstance,
            nullptr
    );

    if (hwnd == nullptr) {
        MessageBox(nullptr, "Error creating window.", "Error", MB_ICONERROR | MB_OK);
        return 0;
    }

    PIXELFORMATDESCRIPTOR pfd; // Pixel format descriptor
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR)); // Zero all the memory occupied by the PFD.
                                                                    // because it might not already be zeroed.
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR); // Set the size of the PFD.
    pfd.nVersion = 1; // Set the version of the PFD.
    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    pfd.iPixelType = PFD_TYPE_RGBA; // Set the desired type of pixels to use.
    pfd.cColorBits = 32; // Set the desired number of color bits.
    pfd.cDepthBits = 32; // Set the desired number of depth bits.

    HDC hdc = GetDC(hwnd); // Get a handle to the device context.
    int pixelFormatIndex = ChoosePixelFormat(hdc,
                                             &pfd); // Attempts to match an appropriate pixel format supported by a device
    // context to a given pixel format specification
    if (pixelFormatIndex == 0) {
        MessageBox(nullptr,
                   "Could not find suitable pixel format descriptor, or descriptor was filled out incorrectly.",
                   "Pixel format descriptor", MB_ICONERROR | MB_OK);
        return 0;
    }

    /* Sets the pixel format of the specified device context to the format specified by the pixel format index. */
    SetPixelFormat(hdc, pixelFormatIndex, &pfd);

    /* Obtains information about the pixel format identified by the pixel format index of the HDC. The PIXELFORMATDESCRIPTOR's
     * members are then set to this vertexData. */
    DescribePixelFormat(hdc, pixelFormatIndex, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

    hglrc = wglCreateContext(hdc); // Get a handle to the OpenGL rendering context.
    wglMakeCurrent(hdc, hglrc); // Make the rendering context the current context.

    if (glewInit() != GLEW_OK) { // Attempt to initialize GLEW. This must happen after a rendering context is made current.
        MessageBox(nullptr, "Error initializing GLEW", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debugMessageCallback, nullptr);

    ShowWindow(hwnd, iCmdShow); // Show the window on the screen.

    char *vertexShaderSource = readFile("vshader.vert");
    char *fragmentShaderSource = readFile("fshader.frag");

    /* Create and compile vertex shader. */
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLint isCompiled = getShaderCompileStatus(vertexShader);
    if (isCompiled == GL_FALSE) {
        MessageBox(nullptr, "Vertex shader failed to compile", "Error", MB_ICONERROR | MB_OK);
        return 0;
    }

    /* Create and compile fragment shader. */
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    isCompiled = getShaderCompileStatus(fragmentShader);
    if (isCompiled == GL_FALSE) {
        MessageBox(nullptr, "Fragment shader failed to compile", "Error", MB_ICONERROR | MB_OK);
        return 0;
    }

    /* Create program, attach shaders to it, and link it. */
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    /* Delete shaders because they are part of the program now. */
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLuint vao; // Object used to supply vertexData to the start of the OpenGL pipeline. Required to draw stuff.
    /* Create vertex arrays. First arg is the number of arrays, second is pointer to an array of VAO, or just one array. */
    glCreateVertexArrays(1, &vao);
    /* Bind the VAO we created to the context so we can use it. */
    glBindVertexArray(vao);

    static const float n = aspect;
    static const float f = 1000.0f;
    static const float l = -1.0f, r = 1.0f;
    static const float t = 1.0f, b = -1.0f;
    static const Vector4f projectionMatrix[] = {
            {(2*n)/(r-l), 0.0, 0.0, 0.0},
            {0.0, (2*n)/(t-b), 0.0, 0.0},
            {(r+l)/(r-l), (t+b)/(t-b), (n+f)/(n-f), -1.0},
            {0.0, 0.0, (2*n*f)/(n-f), 0.0}
    };

    /* A variable to store the name of the bufferNames. */
    GLuint bufferNames[2];

    /* Create buffers.
     * First param is the number of buffers to create.
     * Second is a pointer to a place to store a bufferNames name or array of bufferNames names. */
    glCreateBuffers(2, bufferNames);

    /* Specify the vertexData store parameters for the bufferNames.
     * param1 is the name of the bufferNames
     * param2 is the size of the bufferNames
     * param3 is the initial vertexData to store in the bufferNames
     * param4 are the mapping flags, in this case it's allowing the bufferNames to be mapped for writing purposes. */
//    glNamedBufferStorage(bufferNames[0], sizeof(vertexData), nullptr, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
    glNamedBufferStorage(bufferNames[0], sizeof(blockVertexData), nullptr, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);

    /* Set up bufferNames for storing transformation matrices. */
    glBindBuffer(GL_ARRAY_BUFFER, bufferNames[1]);
    /* There are 3 matrices containing 4 floats each, which explains the multiplication. */
    glBufferStorage(GL_ARRAY_BUFFER, sizeof(Vector4f) * 4, projectionMatrix, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);

    /* Bind a bufferNames to the OGL context using the GL_ARRAY_BUFFER binding point.
     * Using the GL_ARRAY_BUFFER binding point suggests to OGL that we're going to be using
     * the bufferNames to store vertex vertexData. */
    glBindBuffer(GL_ARRAY_BUFFER, bufferNames[0]);

    /* Insert the vertexData into the buffer at offset 0.
     * param3 is the size of the vertexData
     * param4 is a pointer to the vertexData. */
//    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertexData), vertexData);

    /* Map the buffer data store to the clients address space. */
    void *ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
//    memcpy(ptr, vertexData, sizeof(vertexData));
    memcpy(ptr, blockVertexData, sizeof(blockVertexData));

    /* We're done mapping the buffer, so unmap it from the clients address space. */
    glUnmapBuffer(GL_ARRAY_BUFFER);

    /* Bind a bufferNames to one of the vertex bufferNames bindings.
     * 'vaobj' is the vertex array object whose state is being modified.
     * 'bindingindex' is the index of the vertex bufferNames.
     * 'bufferNames' is the name of the bufferNames being bound.
     * 'offset' is where the first vertex's data starts (in bytes).
     * 'stride' is the distance between each vertex (in bytes. Set to 0 for auto calculation.) */
    glVertexArrayVertexBuffer(vao, 0, bufferNames[0], 0, sizeof(Vertex));

    /* Associate a vertex attribute and vertex bufferNames binding for a vertex array object.
     * 'vaobj' is the name of the vertex array object.
     * 'attribindex' is the index of the attribute associated with the vertex bufferNames binding.
     * 'bindingindex' is the index of the vertex bufferNames binding to associate the vertex attribute with. */
    glVertexArrayAttribBinding(vao, 0, 0);

    /* Describe the layout and format of the vertexData.
     * 'vaobj' is the vertex array object whose state is being modified.
     * 'attribindex' is the index of the vertex attribute.
     * 'size' is the number of components stored in the bufferNames for each vertex.
     * 'type' is the type of vertexData.
     * 'normalized' tells OpenGL whether the data in the bufferNames should be normalized.
     * 'relativeoffset' is the offset from the vertex's data where the specific attribute starts. */
    glVertexArrayAttribFormat(vao, 0, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, x));

    glVertexArrayAttribBinding(vao, 2, 0);
    glVertexArrayAttribFormat(vao, 2, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, r));

    /* Enable automatic filling of the attribute.
     * You can also use glEnableVertexAttribArray(0) to use the attribute specified at index 0 without providing 'vao'.
     *
     * 'glDisableVertexAttribArray(GLuint index)' allows you to switch back to using static vertexData via 'glVertexAttrib*'. */
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(2);

    glUseProgram(program); // Tell OpenGL to use the shader program we created.

    /* Returns the index of a named uniform block.
     * 'program' is the shader program.
     * 'uniformBlockName' is the name of the uniform block in the shader program. */
//    GLuint transformBlockIndex = glGetUniformBlockIndex(program, "Transform");
    GLuint transformBlockIndex = 0;

    /* Assign a binding point to a uniform block.
     * 'program' is the shader program where the uniform block is located.
     * 'uniformBlockIndex' is the index of the uniform block getting assigned a binding point.
     * 'uniformBlockBinding' is the index of the uniform block binding point. */
    glUniformBlockBinding(program, transformBlockIndex, 0);

    /* Bind a buffer to a uniform block.
     * 'GL_UNIFORM_BUFFER' says that we're binding a bufferNames to a uniform binding point.
     * 'index' is the index of the binding point.
     * 'bufferNames' the name of the bufferNames object to attach. */
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, bufferNames[1]);

    std::vector<unsigned char> image;
    unsigned width, height;
    unsigned error = lodepng::decode(image, width, height, "dirt.png");
    if (error != 0) {
        MessageBox(nullptr, lodepng_error_text(error), "LodePNG Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    /* GLuint containing texture name. */
    GLuint texture;

    /* Create a texture object and store the name in 'texture'. */
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);

    /* Allocate storage space for the texture.
     * 'texture' is the name of the texture object.
     * '1' is the number of mipmap levels.
     * 'GL_RGBA32F' is the type of data the texture will hold (32-bit floating-point RGBA data).
     * '256' and '256' is the width and height of the texture (256x256 texels). */
    glTextureStorage2D(texture, 1, GL_RGBA32F, width, height);

    /* Fill texture buffer associated with object with texture data.
     * 'texture' is the texture object we created.
     * '0' is the level (mipmap levels I'm assuming).
     * '0' and '0' are the offsets in the texture to apply the data to (width and height I'm assuming).
     * '16' and '16' are the dimensions of the image to replace (in texels).
     * 'GL_RGBA' represents 4 channel data.
     * 'GL_FLOAT' is the type of the data.
     * 'image.data()' is the texture data we're using for the texture. */
    glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);

    /* Bind texture object to the context using GL_TEXTURE_2D binding point. */
    glBindTexture(GL_TEXTURE_2D, texture);

    GLuint sampler;
    /* Create sampler objects.
     * 'n' is the number of sampler objects to create.
     * 'samplers' addresses of variables to store names of samplers. */
    glCreateSamplers(1, &sampler);

    /* Set the filtering mode for the magnification of a texture to choose the nearest texel instead
     * of taking the weighted average of the surrounding texels when determining the color of a fragment.
     * 'sampler' is the sampler to use.
     * 'GL_TEXTURE_MAG_FILTER' specifies that we're setting a parameter for the magnification filter.
     * 'GL_NEAREST' specifies that we want the mag. filter to choose the nearest texel. */
    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    /* Bind a sampler to a texture unit.
     * '0' is the index of the texture unit to bind to.
     * 'sampler' is the sampler we're binding. */
    glBindSampler(0, sampler);

    MSG msg;
    int currentFrame = 0;
    long long previousTime = 0;
    float time = 0;
    while (running) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        using namespace std::chrono;
        auto currentTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        if (previousTime == 0) {
            previousTime = currentTime;
            continue;
        }
        float delta = currentTime - previousTime;
        previousTime = currentTime;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the color and depth buffer bit.
                                                                    // Not clearing the color buffer bit will make it look
                                                                    // like there's smudges on the screen.
        glClearColor(1.0, 0.0, 1.0, 1.0);
        glVertexAttrib1f(1, time); // Set the vertex attribute at location 0 to 'time'
//        glDrawArrays(GL_TRIANGLES, 0, sizeof(vertexData) / sizeof(float));
        glDrawArrays(GL_TRIANGLES, 0, sizeof(blockVertexData) / sizeof(float));

        wglSwapLayerBuffers(hdc, WGL_SWAP_MAIN_PLANE); // Swap front and back buffers because the context is double-buffered

        currentFrame++; // Increment frame count.
        time += delta / 1000; // Increase the time by the delta / 1000 so that the time represents the time that has passed
                            // in seconds.
    }

    glDeleteVertexArrays(1, &vao); // Delete the vertex array (or arrays) we created.
    glDeleteProgram(program); // Delete the shader program we created.
    wglDeleteContext(hglrc); // Delete the OpenGL rendering context we created.

    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    switch (message) {
        case WM_KEYDOWN: {
            static bool vae = true;
            if (wParam == VK_DOWN) {
                if (vae) glDisableVertexAttribArray(2);
                else glEnableVertexAttribArray(2);
                vae = !vae;
            }
            return 0;
        }
        case WM_SIZE: {
            glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            aspect = (float) width / (float) height;
            PostMessage(hwnd, WM_PAINT, 0, 0);
            return 0;
        }
        case WM_PAINT:
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 0;
        case WM_DESTROY:
            running = false;
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}