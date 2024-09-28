#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DARK_BACKGROUND                                                        \
    (Color) { 42, 42, 42, 255 }

#define DARK_FOREGROUND                                                        \
    (Color) { 219, 219, 219, 128 }

#define DARK_DEBUG                                                             \
    (Color) { 219, 219, 219, 16 }

#define LIGHT_BACKGROUND                                                       \
    (Color) { 232, 232, 232, 255 }

#define LIGHT_FOREGROUND                                                       \
    (Color) { 51, 51, 51, 128 }

#define LIGHT_DEBUG                                                            \
    (Color) { 51, 51, 51, 16 }

typedef struct Config {
    Color background;
    Color foreground;
    Color debug_color;
} Config;

Config config;

void config_load(int argc, char *argv[]) {
    config = (Config){DARK_BACKGROUND, DARK_FOREGROUND, DARK_DEBUG};

    for (int i = 0; i < argc; i++) {
        char *arg = argv[i];

        if (strcmp(arg, "--light") == 0 || strcmp(arg, "-l") == 0) {
            config = (Config){LIGHT_BACKGROUND, LIGHT_FOREGROUND, LIGHT_DEBUG};
        }
    }
};

void DrawDebugText(const char *text, int value, int y) {
    DrawText(TextFormat(text, value), 10, (y * 20) + 10, 20,
             config.debug_color);
}

typedef struct Point {
    int x;
    int y;
} Point;

int point_distance(Point a, Point b) {
    return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

typedef struct Bounds {
    int l;
    int t;
    int r;
    int b;
} Bounds;

bool bounds_intersect(Bounds a, Bounds b) {
    return a.l < b.r && a.r > b.l && a.t < b.b && a.b > b.t;
}

void DrawBounds(Bounds bounds, Color color) {
    DrawLine(bounds.l, bounds.t, bounds.r, bounds.t, color);
    DrawLine(bounds.r, bounds.t, bounds.r, bounds.b, color);
    DrawLine(bounds.r, bounds.b, bounds.l, bounds.b, color);
    DrawLine(bounds.l, bounds.b, bounds.l, bounds.t, color);
}

typedef struct Path {
    Point *points;
    int length;
    Bounds bounds;
} Path;

#define SPLINE_SEGMENT_DIVISIONS 16 // Spline segments subdivisions

void DrawPathSplineBasis(Path path, Point position, float thick, Color color) {
    if (path.length < 4) {
        return;
    }

    float a[4] = {0};
    float b[4] = {0};
    float dy = 0.0f;
    float dx = 0.0f;
    float size = 0.0f;

    Vector2 currentPoint = {0};
    Vector2 nextPoint = {0};
    Vector2 vertices[2 * SPLINE_SEGMENT_DIVISIONS + 2] = {0};

    for (int i = 0; i < (path.length - 3); i++) {
        float t = 0.0f;

        Vector2 p1 = (Vector2){
            path.points[i].x + position.x,
            path.points[i].y + position.y,
        };
        Vector2 p2 = (Vector2){
            path.points[i + 1].x + position.x,
            path.points[i + 1].y + position.y,
        };
        Vector2 p3 = (Vector2){
            path.points[i + 2].x + position.x,
            path.points[i + 2].y + position.y,
        };
        Vector2 p4 = (Vector2){
            path.points[i + 3].x + position.x,
            path.points[i + 3].y + position.y,
        };

        a[0] = (-p1.x + 3.0f * p2.x - 3.0f * p3.x + p4.x) / 6.0f;
        a[1] = (3.0f * p1.x - 6.0f * p2.x + 3.0f * p3.x) / 6.0f;
        a[2] = (-3.0f * p1.x + 3.0f * p3.x) / 6.0f;
        a[3] = (p1.x + 4.0f * p2.x + p3.x) / 6.0f;

        b[0] = (-p1.y + 3.0f * p2.y - 3.0f * p3.y + p4.y) / 6.0f;
        b[1] = (3.0f * p1.y - 6.0f * p2.y + 3.0f * p3.y) / 6.0f;
        b[2] = (-3.0f * p1.y + 3.0f * p3.y) / 6.0f;
        b[3] = (p1.y + 4.0f * p2.y + p3.y) / 6.0f;

        currentPoint.x = a[3];
        currentPoint.y = b[3];

        if (i == 0) {
            DrawCircleV(currentPoint, thick / 2.0f,
                        color); // Draw init line circle-cap
        }

        if (i > 0) {
            vertices[0].x = currentPoint.x + dy * size;
            vertices[0].y = currentPoint.y - dx * size;
            vertices[1].x = currentPoint.x - dy * size;
            vertices[1].y = currentPoint.y + dx * size;
        }

        for (int j = 1; j <= SPLINE_SEGMENT_DIVISIONS; j++) {
            t = ((float)j) / ((float)SPLINE_SEGMENT_DIVISIONS);

            nextPoint.x = a[3] + t * (a[2] + t * (a[1] + t * a[0]));
            nextPoint.y = b[3] + t * (b[2] + t * (b[1] + t * b[0]));

            dy = nextPoint.y - currentPoint.y;
            dx = nextPoint.x - currentPoint.x;
            size = 0.5f * thick / sqrtf(dx * dx + dy * dy);

            if ((i == 0) && (j == 1)) {
                vertices[0].x = currentPoint.x + dy * size;
                vertices[0].y = currentPoint.y - dx * size;
                vertices[1].x = currentPoint.x - dy * size;
                vertices[1].y = currentPoint.y + dx * size;
            }

            vertices[2 * j + 1].x = nextPoint.x - dy * size;
            vertices[2 * j + 1].y = nextPoint.y + dx * size;
            vertices[2 * j].x = nextPoint.x + dy * size;
            vertices[2 * j].y = nextPoint.y - dx * size;

            currentPoint = nextPoint;
        }

        DrawTriangleStrip(vertices, 2 * SPLINE_SEGMENT_DIVISIONS + 2, color);
    }

    DrawCircleV(currentPoint, thick / 2.0f, color); // Draw end line circle-cap
}

void DrawPath(Path path, Point position, Bounds screen_bounds) {
    Bounds path_bounds = (Bounds){
        path.bounds.l + position.x,
        path.bounds.t + position.y,
        path.bounds.r + position.x,
        path.bounds.b + position.y,
    };

    if (bounds_intersect(path_bounds, screen_bounds)) {
#ifdef DEBUG
        DrawBounds(path_bounds, config.debug_color);
#endif
        DrawPathSplineBasis(path, position, 1.25, config.foreground);
    }
}

typedef struct CurrentPath {
    Point *points;
    int capacity;
    int length;
} CurrentPath;

void current_path_push(CurrentPath *curr_path, Point point) {
    if (curr_path->length > 0) {
        Point last = curr_path->points[curr_path->length - 1];

        int target_distance = curr_path->length > 4 ? 4 : 1;

        if (point_distance(last, point) < target_distance) {
            return;
        }
    }

    if (curr_path->length == curr_path->capacity) {
        curr_path->capacity *= 2;
        curr_path->points =
            realloc(curr_path->points, sizeof(Point) * curr_path->capacity);
    }
    curr_path->points[curr_path->length] = point;
    curr_path->length++;
}

Path current_path_finalize(CurrentPath *curr_path) {
    Point points[curr_path->length];
    int length = curr_path->length;
    Bounds bounds = {curr_path->points[0].x, curr_path->points[0].y,
                     curr_path->points[0].x, curr_path->points[0].y};

    for (int i = 0; i < curr_path->length; i++) {
        Point point = curr_path->points[i];

        if (point.x < bounds.l) {
            bounds.l = point.x;
        }
        if (point.x > bounds.r) {
            bounds.r = point.x;
        }
        if (point.y < bounds.t) {
            bounds.t = point.y;
        }
        if (point.y > bounds.b) {
            bounds.b = point.y;
        }

        points[i] = point;
    }

    Path path = {
        malloc(sizeof(Point) * length),
        length,
        bounds,
    };

    for (int i = 0; i < length; i++) {
        path.points[i] = points[i];
    }

    curr_path->length = 0;

    return path;
}

void DrawCurrentPath(CurrentPath current_path, Point position) {
    if (current_path.length > 0) {
        Vector2 points[current_path.length];

        for (int i = 0; i < current_path.length; i++) {
            points[i] = (Vector2){
                current_path.points[i].x + position.x,
                current_path.points[i].y + position.y,
            };
        }

        DrawSplineBasis(points, current_path.length, 1.25, config.foreground);
    }
}

typedef struct Canvas {
    Path *paths;
    int capacity;
    int length;
#ifdef DEBUG
    int point_count;
#endif
} Canvas;

void canvas_push(Canvas *canvas, CurrentPath *curr_path) {
    if (curr_path->length == 0) {
        return;
    }

    if (canvas->length == canvas->capacity) {
        canvas->capacity *= 2;
        canvas->paths = realloc(canvas->paths, sizeof(Path) * canvas->capacity);
    }

    Path path = current_path_finalize(curr_path);

    canvas->paths[canvas->length] = path;
    canvas->length++;
#ifdef DEBUG
    canvas->point_count += path.length;
#endif
}

void canvas_clear(Canvas *canvas) {
    if (canvas->length > 0) {
        for (int i = 0; i < canvas->length; i++) {
            free(canvas->paths[i].points);
        }
        canvas->length = 0;
    }
}

void canvas_move_last(Canvas *from, Canvas *to) {
    if (from->length == 0) {
        return;
    }

    Path path = from->paths[from->length - 1];

    if (to->length == to->capacity) {
        to->capacity *= 2;
        to->paths = realloc(to->paths, sizeof(Path) * to->capacity);
    }

    to->paths[to->length] = path;
    to->length++;
#ifdef DEBUG
    to->point_count += path.length;
#endif

    from->length--;
#ifdef DEBUG
    from->point_count -= path.length;
#endif
}

const float INPUT_RATE = 1.f / 30.f;

typedef struct State {
    Point mouse;
    Point mouse_prev;
    Point screen;
    Bounds screen_bounds;
    Point position;
    float frame_time;
    float input_time;
    int fps;
    bool is_input_frame;
    bool is_drawing;
    bool is_drawing_released;
    bool is_dragging;
    bool is_dragging_released;
    bool history_back;
    bool history_forward;
} State;

int main(int argc, char *argv[]) {
    printf("Drawing!\n");

    config_load(argc, argv);

    Canvas canvas = {
        malloc(sizeof(Path) * 4),
        4,
        0,
        0,
    };

    Canvas clipboard = {
        malloc(sizeof(Path) * 4),
        4,
        0,
        0,
    };

    CurrentPath current_path = {
        malloc(sizeof(Point) * 64),
        64,
        0,
    };

    State state = {0};

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(960, 640, "canvas");

    while (!WindowShouldClose()) {
        // READ
        state.is_drawing_released = false;
        state.mouse_prev = state.mouse;
        state.mouse = (Point){GetMouseX(), GetMouseY()};
        state.screen = (Point){GetScreenWidth(), GetScreenHeight()};
        state.screen_bounds = (Bounds){0, 0, state.screen.x, state.screen.y};
        state.fps = GetFPS();
        state.frame_time = GetFrameTime();
        state.input_time += state.frame_time;
        state.is_input_frame = state.input_time > INPUT_RATE;
        state.history_back = IsKeyPressed(KEY_Z) &&
                             IsKeyDown(KEY_LEFT_CONTROL) &&
                             !IsKeyDown(KEY_LEFT_SHIFT);
        state.history_forward = IsKeyPressed(KEY_Z) &&
                                IsKeyDown(KEY_LEFT_CONTROL) &&
                                IsKeyDown(KEY_LEFT_SHIFT);

        if (state.is_input_frame) {
            state.input_time = 0;
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            state.is_drawing = true;
        } else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            state.is_drawing = false;
            state.is_drawing_released = true;
        }

        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            state.is_dragging = true;
        } else if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
            state.is_dragging = false;
            state.is_dragging_released = true;
        }

        // UPDATE
        if (state.is_drawing) {
            canvas_clear(&clipboard);

            if (state.is_input_frame) {
                current_path_push(&current_path,
                                  (Point){state.mouse.x - state.position.x,
                                          state.mouse.y - state.position.y});
            }
        } else if (state.is_drawing_released) {
            canvas_push(&canvas, &current_path);
        }

        if (state.is_dragging) {
            state.position.x += state.mouse.x - state.mouse_prev.x;
            state.position.y += state.mouse.y - state.mouse_prev.y;
        }

        if (state.history_back) {
            canvas_move_last(&canvas, &clipboard);
        } else if (state.history_forward) {
            canvas_move_last(&clipboard, &canvas);
        }

        // RENDER
        BeginDrawing();

        ClearBackground(config.background);

        for (int i = 0; i < canvas.length; i++) {
            DrawPath(canvas.paths[i], state.position, state.screen_bounds);
        }

        DrawCurrentPath(current_path, state.position);

#ifdef DEBUG
        DrawDebugText("fps: %d", state.fps, 0);
        DrawDebugText("paths: %d", canvas.length, 1);
        DrawDebugText("points: %d", canvas.point_count, 2);
#endif

        EndDrawing();
    }

    return 0;
}
