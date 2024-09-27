#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

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

void DrawPath(Path path, Point position, Bounds screen_bounds) {
    Bounds path_bounds = (Bounds){
        path.bounds.l + position.x,
        path.bounds.t + position.y,
        path.bounds.r + position.x,
        path.bounds.b + position.y,
    };

    if (bounds_intersect(path_bounds, screen_bounds)) {
#ifdef DEBUG
        DrawBounds(path_bounds, BLACK);
#endif

        Vector2 points[path.length];

        for (int i = 0; i < path.length; i++) {
            points[i] = (Vector2){
                path.points[i].x + position.x,
                path.points[i].y + position.y,
            };
        }

        DrawSplineBasis(points, path.length, 1.25, BLACK);
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

        DrawSplineBasis(points, current_path.length, 1.25, BLACK);
    }
}

typedef struct Canvas {
    Path *paths;
    int capacity;
    int length;
    int point_count;
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
    canvas->point_count += path.length;
    canvas->length++;
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
} State;

int main(void) {
    printf("Drawing!\n");

    Canvas canvas = {
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

        // RENDER
        BeginDrawing();

        ClearBackground(RAYWHITE);

        for (int i = 0; i < canvas.length; i++) {
            DrawPath(canvas.paths[i], state.position, state.screen_bounds);
        }

        DrawCurrentPath(current_path, state.position);

#ifdef DEBUG
        DrawText(TextFormat("fps: %d", state.fps), 10, 10, 20, BLACK);
        DrawText(TextFormat("paths: %d", canvas.length), 10, 30, 20, BLACK);
        DrawText(TextFormat("points: %d", canvas.point_count), 10, 50, 20,
                 BLACK);
#endif

        EndDrawing();
    }

    return 0;
}
