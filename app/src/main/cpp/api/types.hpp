#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <algorithm>

namespace sense {

// ============================================================================
// 1. Math & Geometry Primitives
// ============================================================================
struct Point {
    int x = 0;
    int y = 0;
    Point() = default;
    Point(int _x, int _y) : x(_x), y(_y) {}
};

struct PointF {
    float x = 0.0f;
    float y = 0.0f;
    PointF() = default;
    PointF(float _x, float _y) : x(_x), y(_y) {}
};

struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    Rect() = default;
    Rect(int _x, int _y, int _w, int _h) : x(_x), y(_y), w(_w), h(_h) {}
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

struct RectF {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    RectF() = default;
    RectF(float _x, float _y, float _w, float _h) : x(_x), y(_y), w(_w), h(_h) {}
};

// ============================================================================
// 2. Color Type & Helpers
// ============================================================================
struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    Color() = default;
    Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255)
        : r(_r), g(_g), b(_b), a(_a) {}

    static Color White()       { return Color(255, 255, 255, 255); }
    static Color Black()       { return Color(0, 0, 0, 255); }
    static Color Transparent() { return Color(0, 0, 0, 0); }
    static Color Red()         { return Color(255, 50, 50, 255); }
    static Color Green()       { return Color(50, 255, 50, 255); }
    static Color Blue()        { return Color(50, 150, 255, 255); }
    static Color Yellow()      { return Color(255, 230, 50, 255); }
    static Color Cyan()        { return Color(50, 230, 255, 255); }
    static Color Magenta()     { return Color(255, 50, 230, 255); }
    static Color Orange()      { return Color(255, 140, 0, 255); }
    static Color Gray(uint8_t v = 128, uint8_t alpha = 255) { return Color(v, v, v, alpha); }
};

// ============================================================================
// 3. Game Checkpoints
// ============================================================================
enum class CheckPoint : int {
    IDLE = 0,
    BEGIN = 1,
    A_START = 500,
    A_STOP = 1000,
    B_START = 1500,
    B_STOP = 2000,
    C_START = 2500,
    C_STOP = 3000,
    D_START = 3500,
    D_STOP = 4000,
    E_START = 4500,
    E_STOP = 5000,
    F_START = 5500,
    F_STOP = 6000,
    G_START = 6500,
    G_STOP = 7000,
    H_START = 7500,
    H_STOP = 8000,
    I_START = 8500,
    I_STOP = 9000,
    J_START = 9500,
    J_STOP = 10000,
    K_START = 10500,
    K_STOP = 11000,
    L_START = 11500,
    L_STOP = 12000,
    M_START = 12500,
    M_STOP = 13000,
    N_START = 13500,
    N_STOP = 14000,
    O_START = 14500,
    O_STOP = 15000,
    P_START = 15500,
    P_STOP = 16000,
    Q_START = 16500,
    Q_STOP = 17000,
    R_START = 17500,
    R_STOP = 18000,
    S_START = 18500,
    S_STOP = 19000,
    T_START = 19500,
    T_STOP = 20000,
    FINAL_START = 25000,
    FINAL_STOP = 25100
};

// ============================================================================
// 4. Player Movement Enum
// ============================================================================
enum class PlayerMove : int {
    Left = 0,
    Right = 1,
    Undefined = 2
};

// ============================================================================
// 5. ABI-Compatible SDL2 Types
// ============================================================================
struct SDL_Point {
    int x;
    int y;
};

struct SDL_Rect {
    int x, y;
    int w, h;
};

struct SDL_Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct SDL_Vertex {
    PointF position;
    SDL_Color color;
    PointF tex_coord;
};

struct SDL_Keysym {
    int32_t scancode;
    int32_t sym;
    uint16_t mod;
    uint32_t unused;
};

struct SDL_KeyboardEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint8_t state;
    uint8_t repeat;
    uint8_t padding2;
    uint8_t padding3;
    SDL_Keysym keysym;
};

struct SDL_MouseButtonEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint32_t which;
    uint8_t button;
    uint8_t state;
    uint8_t clicks;
    uint8_t padding1;
    int32_t x;
    int32_t y;
};

struct SDL_MouseMotionEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint32_t which;
    uint32_t state;
    int32_t x;
    int32_t y;
    int32_t xrel;
    int32_t yrel;
};

struct SDL_WindowEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint8_t event;
    uint8_t padding1;
    uint8_t padding2;
    uint8_t padding3;
    int32_t data1;
    int32_t data2;
};

struct SDL_ControllerButtonEvent {
    uint32_t type;
    uint32_t timestamp;
    int32_t which;
    uint8_t button;
    uint8_t state;
    uint8_t padding1;
    uint8_t padding2;
};

struct SDL_MouseWheelEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint32_t which;
    int32_t x;
    int32_t y;
    uint32_t direction;
    float preciseX;
    float preciseY;
    int32_t mouseX;
    int32_t mouseY;
};

union SDL_Event {
    uint32_t type;
    SDL_KeyboardEvent key;
    SDL_MouseButtonEvent button;
    SDL_MouseMotionEvent motion;
    SDL_MouseWheelEvent wheel;
    SDL_WindowEvent window;
    SDL_ControllerButtonEvent cbutton;
    uint8_t padding[56];
};

struct SDL_Renderer;
struct SDL_Window;
struct SDL_Texture;

// Keyboard Key Codes
enum KeyCode : int32_t {
    Key_Unknown = 0,
    Key_Return = '\r',
    Key_Escape = '\033',
    Key_Backspace = '\b',
    Key_Tab = '\t',
    Key_Space = ' ',
    Key_A = 'a',
    Key_B = 'b',
    Key_C = 'c',
    Key_D = 'd',
    Key_E = 'e',
    Key_F = 'f',
    Key_G = 'g',
    Key_H = 'h',
    Key_I = 'i',
    Key_J = 'j',
    Key_K = 'k',
    Key_L = 'l',
    Key_M = 'm',
    Key_N = 'n',
    Key_O = 'o',
    Key_P = 'p',
    Key_Q = 'q',
    Key_R = 'r',
    Key_S = 's',
    Key_T = 't',
    Key_U = 'u',
    Key_V = 'v',
    Key_W = 'w',
    Key_X = 'x',
    Key_Y = 'y',
    Key_Z = 'z',
    Key_0 = '0',
    Key_1 = '1',
    Key_2 = '2',
    Key_3 = '3',
    Key_4 = '4',
    Key_5 = '5',
    Key_6 = '6',
    Key_7 = '7',
    Key_8 = '8',
    Key_9 = '9',
    Key_F1 = (1 << 30) | 58,
    Key_F2 = (1 << 30) | 59,
    Key_F3 = (1 << 30) | 60,
    Key_F4 = (1 << 30) | 61,
    Key_F5 = (1 << 30) | 62,
    Key_F6 = (1 << 30) | 63,
    Key_F7 = (1 << 30) | 64,
    Key_F8 = (1 << 30) | 65,
    Key_F9 = (1 << 30) | 66,
    Key_F10 = (1 << 30) | 67,
    Key_F11 = (1 << 30) | 68,
    Key_F12 = (1 << 30) | 69,
    Key_Right = (1 << 30) | 79,
    Key_Left = (1 << 30) | 80,
    Key_Down = (1 << 30) | 81,
    Key_Up = (1 << 30) | 82,
    Key_LShift = (1 << 30) | 225,
    Key_LCtrl = (1 << 30) | 224,
    Key_LAlt = (1 << 30) | 226
};

// Aliases for intuitive key referencing
namespace Key {
    constexpr KeyCode Return = Key_Return;
    constexpr KeyCode Escape = Key_Escape;
    constexpr KeyCode Backspace = Key_Backspace;
    constexpr KeyCode Tab = Key_Tab;
    constexpr KeyCode Space = Key_Space;
    constexpr KeyCode A = Key_A;
    constexpr KeyCode B = Key_B;
    constexpr KeyCode C = Key_C;
    constexpr KeyCode D = Key_D;
    constexpr KeyCode E = Key_E;
    constexpr KeyCode F = Key_F;
    constexpr KeyCode G = Key_G;
    constexpr KeyCode H = Key_H;
    constexpr KeyCode I = Key_I;
    constexpr KeyCode J = Key_J;
    constexpr KeyCode K = Key_K;
    constexpr KeyCode L = Key_L;
    constexpr KeyCode M = Key_M;
    constexpr KeyCode N = Key_N;
    constexpr KeyCode O = Key_O;
    constexpr KeyCode P = Key_P;
    constexpr KeyCode Q = Key_Q;
    constexpr KeyCode R = Key_R;
    constexpr KeyCode S = Key_S;
    constexpr KeyCode T = Key_T;
    constexpr KeyCode U = Key_U;
    constexpr KeyCode V = Key_V;
    constexpr KeyCode W = Key_W;
    constexpr KeyCode X = Key_X;
    constexpr KeyCode Y = Key_Y;
    constexpr KeyCode Z = Key_Z;
    constexpr KeyCode F1 = Key_F1;
    constexpr KeyCode F2 = Key_F2;
    constexpr KeyCode F3 = Key_F3;
    constexpr KeyCode F4 = Key_F4;
    constexpr KeyCode F5 = Key_F5;
    constexpr KeyCode F6 = Key_F6;
    constexpr KeyCode F7 = Key_F7;
    constexpr KeyCode F8 = Key_F8;
    constexpr KeyCode F9 = Key_F9;
    constexpr KeyCode F10 = Key_F10;
    constexpr KeyCode F11 = Key_F11;
    constexpr KeyCode F12 = Key_F12;
    constexpr KeyCode Left = Key_Left;
    constexpr KeyCode Right = Key_Right;
    constexpr KeyCode Up = Key_Up;
    constexpr KeyCode Down = Key_Down;
    constexpr KeyCode Shift = Key_LShift;
    constexpr KeyCode Ctrl = Key_LCtrl;
}

// ============================================================================
// 6. Network & HTTP Types
// ============================================================================
struct HttpResponse {
    int statusCode = 0;
    std::string body;
    std::map<std::string, std::string> headers;
    bool success = false;
    std::string errorMessage;
};

// ============================================================================
// 7. Camera & Perspective Types
// ============================================================================
enum class ColorBlendMode : int {
    None = 0,
    Blend = 1,
    Add = 2,
    Mod = 3
};

struct CameraTransform {
    float zoom = 1.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float angle = 0.0f;
    float tiltX = 0.0f;
    float tiltY = 0.0f;
    bool flipH = false;
    bool flipV = false;

    // Screen Shake
    float shakeTrauma = 0.0f;
    float shakeDecay = 1.0f;
    float currentShakeX = 0.0f;
    float currentShakeY = 0.0f;

    // Color Tint / Filter
    bool filterEnabled = false;
    Color filterColor = Color::Transparent();
    ColorBlendMode filterBlend = ColorBlendMode::Blend;

    // Flash Effect
    float flashTimer = 0.0f;
    float flashDuration = 0.0f;
    Color flashColor = Color::White();
};

} // namespace sense

// Make SDL types accessible in global namespace
using SDL_Renderer = sense::SDL_Renderer;
using SDL_Window = sense::SDL_Window;
using SDL_Texture = sense::SDL_Texture;
using SDL_Event = sense::SDL_Event;
using SDL_Point = sense::SDL_Point;
using SDL_Rect = sense::SDL_Rect;
using SDL_Color = sense::SDL_Color;
using SDL_Vertex = sense::SDL_Vertex;

