#define NOMINMAX
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <string>
#include <random>

constexpr float WIDTH = 1280.0f, HEIGHT = 720.0f;
constexpr float PI = 3.14159265359f;
constexpr double STOP_TIME = 10.0, BUS_SPEED = 110.0;
struct Point { float x, y; };
struct Mesh { GLuint vao = 0, vbo = 0; GLsizei count = 0; };
struct Segment {
    std::vector<Point> points;
    std::vector<double> lengths;
    double length = 0;
};
struct PassengerInput {
    const bool* stopped = nullptr;
    int passengers = 0;
    bool inspector = false;
    unsigned long long fines = 0;
    std::mt19937 rng{ std::random_device{}() };
};
void errorCallback(int code, const char* text) {
    std::cerr << "GLFW " << code << ": " << text << '\n';
}
void resizeCallback(GLFWwindow*, int w, int h) {
    if (w <= 0 || h <= 0) return;
    int vw = w, vh = h;
    if (float(w) / h > WIDTH / HEIGHT) vw = int(h * WIDTH / HEIGHT);
    else vh = int(w * HEIGHT / WIDTH);
    glViewport((w - vw) / 2, (h - vh) / 2, vw, vh);
}
void mouseButtonCallback(GLFWwindow* window, int button, int action, int) {
    if (action != GLFW_PRESS) return;
    auto* s = static_cast<PassengerInput*>(glfwGetWindowUserPointer(window));
    if (!s || !s->stopped || !*s->stopped) return;
    if (button == GLFW_MOUSE_BUTTON_LEFT && s->passengers < 50) ++s->passengers;
    // Kontrolor ne izlazi desnim klikom: ceka narednu stanicu.
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && s->passengers > (s->inspector ? 1 : 0))
        --s->passengers;
}
void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }
    auto* s = static_cast<PassengerInput*>(glfwGetWindowUserPointer(window));
    if (!s || key != GLFW_KEY_K || !s->stopped || !*s->stopped) return;
    if (!s->inspector && s->passengers < 50) {
        s->inspector = true;
        ++s->passengers;
        std::cout << "Kontrolor je usao.\n";
    }
}
void inspectorLeaves(PassengerInput& s) {
    if (!s.inspector) return;
    // Pre izlaska: passengers ukljucuje jednog kontrolora.
    int ordinaryPassengers = s.passengers - 1;
    std::uniform_int_distribution<int> distribution(0, ordinaryPassengers);
    int newFines = distribution(s.rng);
    s.fines += static_cast<unsigned long long>(newFines);
    --s.passengers;
    s.inspector = false;
    std::cout << "Kontrolor je izasao. Nove kazne: " << newFines
        << "; ukupno: " << s.fines << '\n';
}
GLuint compileShader(GLenum type, const char* text) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &text, nullptr); glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048]; glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cerr << log << '\n'; glDeleteShader(s); return 0;
    }
    return s;
}
GLuint createProgram() {
    const char* vs = R"(
#version 330 core
layout(location=0) in vec2 aPosition;
uniform vec2 uOffset;
uniform vec2 uScale;
out vec2 vUV;
void main() {
    vec2 p=aPosition*uScale+uOffset;
    gl_Position=vec4(p.x/1280.0*2.0-1.0,1.0-p.y/720.0*2.0,0.0,1.0);
    vUV=aPosition;
}
)";
    const char* fs = R"(
#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform vec4 uColor;
uniform bool uTextured;
uniform sampler2D uTexture;
void main() {
    FragColor=uColor;
    if(uTextured) FragColor*=texture(uTexture,vUV);
}
)";
    GLuint v = compileShader(GL_VERTEX_SHADER, vs), f = compileShader(GL_FRAGMENT_SHADER, fs);
    if (!v || !f) { glDeleteShader(v); glDeleteShader(f); return 0; }
    GLuint program = glCreateProgram();
    glAttachShader(program, v); glAttachShader(program, f); glLinkProgram(program);
    glDeleteShader(v); glDeleteShader(f);
    GLint ok = 0; glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048]; glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << log << '\n'; glDeleteProgram(program); return 0;
    }
    return program;
}
Mesh createMesh(const std::vector<Point>& points) {
    Mesh m; m.count = static_cast<GLsizei>(points.size());
    glGenVertexArrays(1, &m.vao); glGenBuffers(1, &m.vbo);
    glBindVertexArray(m.vao); glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Point), points.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), nullptr);
    glEnableVertexAttribArray(0); glBindVertexArray(0);
    return m;
}
void deleteMesh(const Mesh& m) {
    glDeleteVertexArrays(1, &m.vao); glDeleteBuffers(1, &m.vbo);
}
GLuint createTexture(int w, int h, const std::vector<unsigned char>& pixels) {
    GLuint t; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    return t;
}
GLuint createDigit(int digit) {
    static const char* patterns[] = {
        "111101101101111","010110010010111","111001111100111",
        "111001111001111","101101111001001","111100111001111",
        "111100111101111","111001010010010","111101111101111",
        "111101111001111"
    };
    std::vector<unsigned char> pixels(3 * 5 * 4, 255);
    for (int i = 0; i < 15; ++i) pixels[i * 4 + 3] = patterns[digit][i] == '1' ? 255 : 0;
    return createTexture(3, 5, pixels);
}
GLuint createBusTexture() {
    constexpr int w = 48, h = 24;
    std::vector<unsigned char> p(w * h * 4, 0);
    auto rect = [&](int x, int y, int rw, int rh, int r, int g, int b) {
        for (int j = y; j < y + rh; ++j) for (int i = x; i < x + rw; ++i) {
            int k = (j * w + i) * 4;
            p[k] = static_cast<unsigned char>(r); p[k + 1] = static_cast<unsigned char>(g);
            p[k + 2] = static_cast<unsigned char>(b); p[k + 3] = 255;
        }
        };
    rect(2, 2, 43, 18, 245, 186, 42);
    rect(4, 4, 7, 7, 100, 200, 235); rect(13, 4, 7, 7, 100, 200, 235);
    rect(22, 4, 7, 7, 100, 200, 235); rect(32, 4, 10, 7, 100, 200, 235);
    rect(32, 12, 9, 7, 65, 75, 85); rect(36, 12, 1, 7, 240, 240, 240);
    rect(4, 15, 26, 2, 230, 125, 25);
    rect(8, 18, 7, 6, 24, 28, 34); rect(34, 18, 7, 6, 24, 28, 34);
    rect(10, 20, 3, 2, 170, 175, 180); rect(36, 20, 3, 2, 170, 175, 180);
    rect(43, 13, 3, 3, 255, 255, 210);
    return createTexture(w, h, p);
}
Point catmullRom(Point a, Point b, Point c, Point d, float t) {
    auto f = [t](float a, float b, float c, float d) {
        return 0.5f * (2 * b + (-a + c) * t + (2 * a - 5 * b + 4 * c - d) * t * t + (-a + 3 * b - 3 * c + d) * t * t * t);
        };
    return { f(a.x,b.x,c.x,d.x),f(a.y,b.y,c.y,d.y) };
}
std::array<Segment, 10> createSegments(const std::vector<Point>& stations) {
    std::array<Segment, 10> segments;
    for (int i = 0; i < 10; ++i) {
        auto& s = segments[i];
        for (int j = 0; j <= 200; ++j) {
            Point p = catmullRom(stations[(i + 9) % 10], stations[i],
                stations[(i + 1) % 10], stations[(i + 2) % 10], j / 200.0f);
            if (j == 0) p = stations[i];
            if (j == 200) p = stations[(i + 1) % 10];
            if (!s.points.empty()) {
                Point prev = s.points.back();
                double dx = p.x - prev.x, dy = p.y - prev.y;
                s.length += std::sqrt(dx * dx + dy * dy);
            }
            s.points.push_back(p); s.lengths.push_back(s.length);
        }
    }
    return segments;
}
Point positionAtDistance(const Segment& s, double distance) {
    if (distance <= 0) return s.points.front();
    if (distance >= s.length) return s.points.back();
    auto it = std::upper_bound(s.lengths.begin(), s.lengths.end(), distance);
    size_t i = static_cast<size_t>(it - s.lengths.begin());
    double span = s.lengths[i] - s.lengths[i - 1];
    float t = static_cast<float>((distance - s.lengths[i - 1]) / span);
    return { s.points[i - 1].x + (s.points[i].x - s.points[i - 1].x) * t,
            s.points[i - 1].y + (s.points[i].y - s.points[i - 1].y) * t };
}
int main() {
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Autobus - SV66/2022", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(window); glfwTerminate(); return 1;
    }
    glfwSetFramebufferSizeCallback(window, resizeCallback);
    int w, h; glfwGetFramebufferSize(window, &w, &h); resizeCallback(window, w, h);
    glfwSwapInterval(1);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLuint program = createProgram();
    if (!program) { glfwDestroyWindow(window); glfwTerminate(); return 1; }
    glUseProgram(program);
    GLint offset = glGetUniformLocation(program, "uOffset");
    GLint scale = glGetUniformLocation(program, "uScale");
    GLint color = glGetUniformLocation(program, "uColor");
    GLint textured = glGetUniformLocation(program, "uTextured");
    glUniform1i(glGetUniformLocation(program, "uTexture"), 0);
    glActiveTexture(GL_TEXTURE0);
    const std::vector<Point> stations = {
        {170,220},{350,140},{580,200},{800,130},{1070,230},
        {1110,440},{900,570},{650,500},{420,590},{180,470}
    };
    auto segments = createSegments(stations);
    std::vector<Point> route;
    for (const auto& s : segments) route.insert(route.end(), s.points.begin(), s.points.end());
    Mesh routeMesh = createMesh(route);
    std::vector<Point> circle = { {0,0} };
    for (int i = 0; i <= 64; ++i) circle.push_back({ std::cos(2 * PI * i / 64),std::sin(2 * PI * i / 64) });
    Mesh circleMesh = createMesh(circle);
    Mesh quad = createMesh({ {0,0},{1,0},{1,1},{0,0},{1,1},{0,1} });
    GLuint digits[10];
    for (int i = 0; i < 10; ++i) digits[i] = createDigit(i);
    GLuint busTexture = createBusTexture();
    auto draw = [&](const Mesh& mesh, GLenum mode, float x, float y, float sx, float sy,
        float r, float g, float b, float a, GLuint texture) {
            glUniform2f(offset, x, y); glUniform2f(scale, sx, sy);
            glUniform4f(color, r, g, b, a); glUniform1i(textured, texture != 0);
            glBindTexture(GL_TEXTURE_2D, texture); glBindVertexArray(mesh.vao);
            glDrawArrays(mode, 0, mesh.count);
        };
    auto rect = [&](float x, float y, float sx, float sy, float r, float g, float b) {
        draw(quad, GL_TRIANGLES, x, y, sx, sy, r, g, b, 1, 0);
        };
    auto number = [&](unsigned long long value, float x, float y, float size) {
        for (char c : std::to_string(value)) {
            draw(quad, GL_TRIANGLES, x, y, 3 * size, 5 * size, 1, 1, 1, 1, digits[c - '0']);
            x += 4 * size;
        }
        };
    int currentStation = 0;
    bool stopped = true;
    PassengerInput input;
    input.stopped = &stopped;
    glfwSetWindowUserPointer(window, &input);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);
    double stopRemaining = STOP_TIME, distance = 0;
    Point bus = stations[0];
    double previousTime = glfwGetTime();
    std::cout << "Levi klik: ulazak. Desni: izlazak. K: kontrola. Escape: izlaz.\n";
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime(), remaining = now - previousTime;
        previousTime = now;
        while (remaining > 0) {
            if (stopped) {
                double step = std::min(remaining, stopRemaining);
                stopRemaining -= step; remaining -= step;
                if (stopRemaining <= 0) { stopped = false; distance = 0; }
            }
            else {
                const Segment& s = segments[currentStation];
                double toArrival = (s.length - distance) / BUS_SPEED;
                if (remaining < toArrival) {
                    distance += BUS_SPEED * remaining; remaining = 0;
                    bus = positionAtDistance(s, distance);
                }
                else {
                    remaining -= toArrival;
                    currentStation = (currentStation + 1) % 10;
                    bus = stations[currentStation];
                    stopped = true; stopRemaining = STOP_TIME; distance = 0;
                    inspectorLeaves(input);
                    std::cout << "Stanica " << currentStation << '\n';
                }
            }
        }
        glfwPollEvents();
        if (glfwWindowShouldClose(window)) break;
        std::string title = "Autobus - SV66/2022 | ";
        if (stopped) title += "Stanica " + std::to_string(currentStation) +
            " | Vrata otvorena | Polazak za " + std::to_string(int(std::ceil(stopRemaining))) + " s";
        else title += "Ka stanici " + std::to_string((currentStation + 1) % 10) + " | Vrata zatvorena";
        title += " | Putnici: " + std::to_string(input.passengers) + "/50";
        title += " | Kazne: " + std::to_string(input.fines);
        if (input.inspector) title += " | KONTROLA";
        glfwSetWindowTitle(window, title.c_str());
        glClearColor(0.08f, 0.12f, 0.18f, 1); glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        draw(routeMesh, GL_LINE_STRIP, 0, 0, 1, 1, 0.9f, 0.15f, 0.18f, 1, 0);
        for (int i = 0; i < 10; ++i) {
            draw(circleMesh, GL_TRIANGLE_FAN, stations[i].x, stations[i].y, 23, 23, 0.95f, 0.12f, 0.15f, 1, 0);
            draw(quad, GL_TRIANGLES, stations[i].x - 9, stations[i].y - 15, 18, 30, 1, 1, 1, 1, digits[i]);
        }
        draw(quad, GL_TRIANGLES, bus.x - 36, bus.y - 18, 72, 36, 1, 1, 1, 1, busTexture);
        // Gornji levi panel: osoba + broj putnika, zuti tiket + ukupne kazne.
        rect(20, 20, 510, 80, 0.15f, 0.20f, 0.27f);
        draw(circleMesh, GL_TRIANGLE_FAN, 52, 42, 9, 9, 0.95f, 0.95f, 1, 1, 0);
        rect(42, 55, 20, 22, 0.95f, 0.95f, 1);
        rect(42, 77, 7, 13, 0.95f, 0.95f, 1); rect(55, 77, 7, 13, 0.95f, 0.95f, 1);
        number(input.passengers, 86, 40, 6);
        rect(180, 32, 2, 56, 0.35f, 0.40f, 0.47f);
        rect(202, 40, 32, 30, 0.98f, 0.76f, 0.25f);
        rect(209, 48, 18, 3, 0.25f, 0.20f, 0.12f);
        rect(209, 57, 18, 3, 0.25f, 0.20f, 0.12f);
        number(input.fines, 250, 40, 5);
        // Gornji desni panel: vrata.
        rect(1150, 20, 104, 80, 0.15f, 0.20f, 0.27f);
        rect(1170, 30, 64, 60, 0.03f, 0.05f, 0.08f);
        if (stopped) {
            rect(1170, 30, 12, 60, 0.25f, 0.85f, 0.55f);
            rect(1222, 30, 12, 60, 0.25f, 0.85f, 0.55f);
        }
        else {
            rect(1170, 30, 30, 60, 0.75f, 0.78f, 0.82f);
            rect(1204, 30, 30, 60, 0.75f, 0.78f, 0.82f);
        }
        // Treci ugao: oznaka kontrolora, vidljiva samo dok je u autobusu.
        if (input.inspector) {
            rect(20, 620, 100, 80, 0.15f, 0.20f, 0.27f);
            draw(circleMesh, GL_TRIANGLE_FAN, 70, 648, 11, 11, 1, 0.8f, 0.6f, 1, 0);
            rect(56, 632, 28, 8, 0.2f, 0.55f, 0.95f);
            rect(52, 638, 36, 4, 0.12f, 0.3f, 0.6f);
            rect(54, 662, 32, 28, 0.2f, 0.55f, 0.95f);
            rect(75, 668, 6, 8, 1, 0.85f, 0.2f);
        }
        glBindVertexArray(0); glfwSwapBuffers(window);
    }
    glfwSetMouseButtonCallback(window, nullptr); glfwSetKeyCallback(window, nullptr);
    glfwSetWindowUserPointer(window, nullptr);
    glDeleteTextures(10, digits); glDeleteTextures(1, &busTexture);
    deleteMesh(routeMesh); deleteMesh(circleMesh); deleteMesh(quad);
    glDeleteProgram(program); glfwDestroyWindow(window); glfwTerminate();
    return 0;
}