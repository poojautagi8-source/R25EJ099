#include <stdio.h>
#include <stdlib.h>

#define ROWS 20
#define COLS 60
#define MAX_SHAPES 100

// ==========================================
// Structure definitions & Enums
// ==========================================

typedef enum {
    SHAPE_LINE,
    SHAPE_RECTANGLE,
    SHAPE_CIRCLE,
    SHAPE_TRIANGLE
} ShapeType;

typedef struct {
    int x1, y1;
    int x2, y2;
} LineParams;

typedef struct {
    int x1, y1;
    int x2, y2;
} RectParams;

typedef struct {
    int xc, yc;
    int r;
} CircleParams;

typedef struct {
    int x1, y1;
    int x2, y2;
    int x3, y3;
} TriangleParams;

typedef struct {
    int id;
    ShapeType type;
    union {
        LineParams line;
        RectParams rect;
        CircleParams circle;
        TriangleParams triangle;
    } data;
} Shape;

// ==========================================
// Global state
// ==========================================

static char canvas[ROWS][COLS];
static Shape shapes[MAX_SHAPES];
static int shape_count = 0;
static int next_id = 1;

// ==========================================
// Drawing algorithms
// ==========================================

static void plot(char canvas[ROWS][COLS], int x, int y) {
    if (x >= 0 && x < COLS && y >= 0 && y < ROWS) {
        canvas[y][x] = '*';
    }
}

void draw_line(char canvas[ROWS][COLS], int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        plot(canvas, x1, y1);
        if (x1 == x2 && y1 == y2) {
            break;
        }
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void draw_rect(char canvas[ROWS][COLS], int x1, int y1, int x2, int y2) {
    draw_line(canvas, x1, y1, x2, y1);
    draw_line(canvas, x1, y2, x2, y2);
    draw_line(canvas, x1, y1, x1, y2);
    draw_line(canvas, x2, y1, x2, y2);
}

static void plot_circle_points(char canvas[ROWS][COLS], int xc, int yc, int x, int y) {
    plot(canvas, xc + x, yc + y);
    plot(canvas, xc - x, yc + y);
    plot(canvas, xc + x, yc - y);
    plot(canvas, xc - x, yc - y);
    plot(canvas, xc + y, yc + x);
    plot(canvas, xc - y, yc + x);
    plot(canvas, xc + y, yc - x);
    plot(canvas, xc - y, yc - x);
}

void draw_circle(char canvas[ROWS][COLS], int xc, int yc, int r) {
    if (r < 0) return;
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;
    
    plot_circle_points(canvas, xc, yc, x, y);
    while (y >= x) {
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
        plot_circle_points(canvas, xc, yc, x, y);
    }
}

void draw_triangle(char canvas[ROWS][COLS], int x1, int y1, int x2, int y2, int x3, int y3) {
    draw_line(canvas, x1, y1, x2, y2);
    draw_line(canvas, x2, y2, x3, y3);
    draw_line(canvas, x3, y3, x1, y1);
}

// ==========================================
// Canvas controls
// ==========================================

void clear_canvas(char canvas[ROWS][COLS]) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            canvas[r][c] = '_';
        }
    }
}

void display_canvas(char canvas[ROWS][COLS]) {
    // Print column coordinates headers
    printf("\n   ");
    for (int c = 0; c < COLS; c++) {
        if (c % 10 == 0) {
            printf("%d", c / 10);
        } else {
            printf(" ");
        }
    }
    printf("\n   ");
    for (int c = 0; c < COLS; c++) {
        printf("%d", c % 10);
    }
    printf("\n");

    // Top border
    printf("  +");
    for (int c = 0; c < COLS; c++) printf("-");
    printf("+\n");

    // Canvas values
    for (int r = 0; r < ROWS; r++) {
        printf("%2d|", r);
        for (int c = 0; c < COLS; c++) {
            if (canvas[r][c] == '*') {
                // Bright yellow color for shapes (standard ANSI sequence)
                printf("\033[1;33m*\033[0m");
            } else {
                // Dim background color for '_'
                printf("\033[90m_\033[0m");
            }
        }
        printf("|\n");
    }

    // Bottom border
    printf("  +");
    for (int c = 0; c < COLS; c++) printf("-");
    printf("+\n");
}

static void redraw_all() {
    clear_canvas(canvas);
    for (int i = 0; i < shape_count; i++) {
        Shape s = shapes[i];
        switch (s.type) {
            case SHAPE_LINE:
                draw_line(canvas, s.data.line.x1, s.data.line.y1, s.data.line.x2, s.data.line.y2);
                break;
            case SHAPE_RECTANGLE:
                draw_rect(canvas, s.data.rect.x1, s.data.rect.y1, s.data.rect.x2, s.data.rect.y2);
                break;
            case SHAPE_CIRCLE:
                draw_circle(canvas, s.data.circle.xc, s.data.circle.yc, s.data.circle.r);
                break;
            case SHAPE_TRIANGLE:
                draw_triangle(canvas, s.data.triangle.x1, s.data.triangle.y1,
                              s.data.triangle.x2, s.data.triangle.y2,
                              s.data.triangle.x3, s.data.triangle.y3);
                break;
        }
    }
}

// ==========================================
// Inputs & Menu functions
// ==========================================

static void flush_input() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static int get_int(const char *prompt, int min, int max) {
    int val;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &val) == 1) {
            if (val >= min && val <= max) {
                flush_input();
                return val;
            }
            printf("Error: Value must be in range [%d, %d].\n", min, max);
        } else {
            printf("Error: Invalid number. Try again.\n");
            flush_input();
        }
    }
}

static int get_x(const char *prompt) {
    return get_int(prompt, 0, COLS - 1);
}

static int get_y(const char *prompt) {
    return get_int(prompt, 0, ROWS - 1);
}

static void list_shapes() {
    printf("\n=== Active Shape Objects ===\n");
    if (shape_count == 0) {
        printf("(No shapes in the list)\n");
        printf("============================\n");
        return;
    }
    for (int i = 0; i < shape_count; i++) {
        Shape s = shapes[i];
        printf("[%d] ", s.id);
        switch (s.type) {
            case SHAPE_LINE:
                printf("Line: start(%d, %d) to end(%d, %d)\n", 
                       s.data.line.x1, s.data.line.y1, s.data.line.x2, s.data.line.y2);
                break;
            case SHAPE_RECTANGLE:
                printf("Rectangle: top-left(%d, %d) to bottom-right(%d, %d)\n", 
                       s.data.rect.x1, s.data.rect.y1, s.data.rect.x2, s.data.rect.y2);
                break;
            case SHAPE_CIRCLE:
                printf("Circle: center(%d, %d), radius %d\n", 
                       s.data.circle.xc, s.data.circle.yc, s.data.circle.r);
                break;
            case SHAPE_TRIANGLE:
                printf("Triangle: vertices (%d, %d), (%d, %d), (%d, %d)\n", 
                       s.data.triangle.x1, s.data.triangle.y1,
                       s.data.triangle.x2, s.data.triangle.y2,
                       s.data.triangle.x3, s.data.triangle.y3);
                break;
        }
    }
    printf("============================\n");
}

static void add_shape_menu() {
    if (shape_count >= MAX_SHAPES) {
        printf("\n\033[1;31mError: Maximum shape count limit reached.\033[0m\n");
        return;
    }

    printf("\n--- Add Shape Menu ---\n");
    printf("1. Line\n");
    printf("2. Rectangle\n");
    printf("3. Circle\n");
    printf("4. Triangle\n");
    printf("5. Back\n");
    
    int choice = get_int("Choose shape type (1-5): ", 1, 5);
    if (choice == 5) return;

    Shape s;
    s.id = next_id++;
    
    switch (choice) {
        case 1:
            s.type = SHAPE_LINE;
            printf("\nAdding Line:\n");
            s.data.line.x1 = get_x("Enter start X (0-59): ");
            s.data.line.y1 = get_y("Enter start Y (0-19): ");
            s.data.line.x2 = get_x("Enter end X (0-59): ");
            s.data.line.y2 = get_y("Enter end Y (0-19): ");
            break;
        case 2:
            s.type = SHAPE_RECTANGLE;
            printf("\nAdding Rectangle:\n");
            s.data.rect.x1 = get_x("Enter top-left X (0-59): ");
            s.data.rect.y1 = get_y("Enter top-left Y (0-19): ");
            s.data.rect.x2 = get_x("Enter bottom-right X (0-59): ");
            s.data.rect.y2 = get_y("Enter bottom-right Y (0-19): ");
            break;
        case 3:
            s.type = SHAPE_CIRCLE;
            printf("\nAdding Circle:\n");
            s.data.circle.xc = get_x("Enter center X (0-59): ");
            s.data.circle.yc = get_y("Enter center Y (0-19): ");
            s.data.circle.r = get_int("Enter radius (>= 0): ", 0, 100);
            break;
        case 4:
            s.type = SHAPE_TRIANGLE;
            printf("\nAdding Triangle:\n");
            s.data.triangle.x1 = get_x("Enter vertex 1 X (0-59): ");
            s.data.triangle.y1 = get_y("Enter vertex 1 Y (0-19): ");
            s.data.triangle.x2 = get_x("Enter vertex 2 X (0-59): ");
            s.data.triangle.y2 = get_y("Enter vertex 2 Y (0-19): ");
            s.data.triangle.x3 = get_x("Enter vertex 3 X (0-59): ");
            s.data.triangle.y3 = get_y("Enter vertex 3 Y (0-19): ");
            break;
    }

    shapes[shape_count++] = s;
    printf("\n\033[1;32mShape successfully added with ID %d!\033[0m\n", s.id);
    
    redraw_all();
    display_canvas(canvas);
}

static void delete_shape_menu() {
    if (shape_count == 0) {
        printf("\nNo active shapes to delete.\n");
        return;
    }

    list_shapes();
    int id = get_int("Enter Shape ID to delete: ", 1, 9999);
    
    int index = -1;
    for (int i = 0; i < shape_count; i++) {
        if (shapes[i].id == id) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        printf("\n\033[1;31mError: Shape ID %d not found.\033[0m\n", id);
        return;
    }

    for (int i = index; i < shape_count - 1; i++) {
        shapes[i] = shapes[i + 1];
    }
    shape_count--;

    printf("\n\033[1;32mShape ID %d successfully deleted!\033[0m\n", id);
    redraw_all();
    display_canvas(canvas);
}

static void modify_shape_menu() {
    if (shape_count == 0) {
        printf("\nNo active shapes to modify.\n");
        return;
    }

    list_shapes();
    int id = get_int("Enter Shape ID to modify: ", 1, 9999);

    int index = -1;
    for (int i = 0; i < shape_count; i++) {
        if (shapes[i].id == id) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        printf("\n\033[1;31mError: Shape ID %d not found.\033[0m\n", id);
        return;
    }

    Shape *s = &shapes[index];
    printf("\nModifying Shape ID %d:\n", s->id);
    switch (s->type) {
        case SHAPE_LINE:
            s->data.line.x1 = get_x("Enter new start X (0-59): ");
            s->data.line.y1 = get_y("Enter new start Y (0-19): ");
            s->data.line.x2 = get_x("Enter new end X (0-59): ");
            s->data.line.y2 = get_y("Enter new end Y (0-19): ");
            break;
        case SHAPE_RECTANGLE:
            s->data.rect.x1 = get_x("Enter new top-left X (0-59): ");
            s->data.rect.y1 = get_y("Enter new top-left Y (0-19): ");
            s->data.rect.x2 = get_x("Enter new bottom-right X (0-59): ");
            s->data.rect.y2 = get_y("Enter new bottom-right Y (0-19): ");
            break;
        case SHAPE_CIRCLE:
            s->data.circle.xc = get_x("Enter new center X (0-59): ");
            s->data.circle.yc = get_y("Enter new center Y (0-19): ");
            s->data.circle.r = get_int("Enter new radius: ", 0, 100);
            break;
        case SHAPE_TRIANGLE:
            s->data.triangle.x1 = get_x("Enter new vertex 1 X (0-59): ");
            s->data.triangle.y1 = get_y("Enter new vertex 1 Y (0-19): ");
            s->data.triangle.x2 = get_x("Enter new vertex 2 X (0-59): ");
            s->data.triangle.y2 = get_y("Enter new vertex 2 Y (0-19): ");
            s->data.triangle.x3 = get_x("Enter new vertex 3 X (0-59): ");
            s->data.triangle.y3 = get_y("Enter new vertex 3 Y (0-19): ");
            break;
    }

    printf("\n\033[1;32mShape ID %d successfully modified!\033[0m\n", s->id);
    redraw_all();
    display_canvas(canvas);
}

static void show_help() {
    printf("\n=== Help & Coordinate Guides ===\n");
    printf("Canvas size is 20 rows by 60 columns.\n");
    printf("Horizontal coordinates: X goes from 0 (left) to 59 (right).\n");
    printf("Vertical coordinates: Y goes from 0 (top) to 19 (bottom).\n");
    printf("Background coordinates use grey '_', shape coordinates use gold '*'.\n");
    printf("================================\n");
}

int main() {
    clear_canvas(canvas);

    while (1) {
        printf("\n\033[1;36m=== 2D ASCII GRAPHICS EDITOR ===\033[0m\n");
        printf("1. Add Shape\n");
        printf("2. Delete Shape\n");
        printf("3. Modify Shape\n");
        printf("4. Display Picture\n");
        printf("5. List Active Shapes\n");
        printf("6. Help & Instructions\n");
        printf("7. Exit\n");
        printf("\033[1;36m================================\033[0m\n");
        
        int choice = get_int("Enter option (1-7): ", 1, 7);
        switch (choice) {
            case 1:
                add_shape_menu();
                break;
            case 2:
                delete_shape_menu();
                break;
            case 3:
                modify_shape_menu();
                break;
            case 4:
                redraw_all();
                display_canvas(canvas);
                break;
            case 5:
                list_shapes();
                break;
            case 6:
                show_help();
                break;
            case 7:
                printf("\nExiting. Goodbye!\n");
                return 0;
        }
    }
}
