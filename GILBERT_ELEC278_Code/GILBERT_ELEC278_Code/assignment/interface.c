#include "interface.h"
#include "model.h"

#include <ctype.h>
#include <errno.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_EDIT_SIZE 128

// Current cur_row and column.
static ROW cur_row = ROW_1;
static COL cur_col = COL_A;

// Column to return to when pressing <enter>.
static COL return_col = COL_A;

// Current editable text.
static char *edit_text = NULL;
static size_t edit_text_capacity = 0;
static size_t edit_text_length = 0;
static size_t edit_position = 0;
static size_t edit_display_offset = 0;

static void set_cell_attr(attr_t attr) {
    mvchgat(2 * ((int) cur_row + 2) + 1, (CELL_DISPLAY_WIDTH + 1) * (cur_col + 1) + 1, CELL_DISPLAY_WIDTH, attr, 0,
            NULL);
}

static void ensure_edit_text_capacity(size_t capacity) {
    if (capacity <= edit_text_capacity)
        return;
    if (capacity < DEFAULT_EDIT_SIZE)
        capacity = DEFAULT_EDIT_SIZE;
    if (capacity < edit_text_capacity * 2)
        capacity = edit_text_capacity * 2;
    if (edit_text == NULL) {
        edit_text = malloc(capacity);
        edit_text[0] = 0;
    } else
        edit_text = realloc(edit_text, capacity);
    if (edit_text == NULL) {
        endwin();
        exit(ENOMEM);
    }
    edit_text_capacity = capacity;
}

int main() {
    /* INITIALIZATION */

    // Initialize NCURSES.
    initscr();

    // Enable raw characters for control sequences.
    raw();

    // Disable automatic echo of typed characters.
    noecho();

    // Enable input of function keys.
    keypad(stdscr, true);

    /* DRAW BORDERS */

    // Include extra column to the left for cur_row numbers.
    const size_t total_width = (NUM_COLS + 1) * (CELL_DISPLAY_WIDTH + 1) + 1;
    // Include two extra rows on top for edit field and cur_row numbers.
    const size_t total_height = (NUM_ROWS + 2) * 2 + 1;

    // Resize window.
    resizeterm(total_height + 1, total_width);

    // Draw the top line.
    addch(ACS_ULCORNER);
    for (size_t i = 0; i < total_width - 2; i++)
        addch(ACS_HLINE);
    addch(ACS_URCORNER);

    // Draw the left/right and interior lines.
    for (size_t i = 0; i < NUM_ROWS + 2; i++) {
        if (i > 0) {
            mvaddch(2 * i, 0, ACS_LTEE);
            for (size_t j = 0; j < NUM_COLS + 1; j++) {
                if (j > 0)
                    addch(i == 1 ? ACS_TTEE : ACS_PLUS);
                for (size_t k = 0; k < CELL_DISPLAY_WIDTH; k++)
                    addch(ACS_HLINE);
            }
            addch(ACS_RTEE);
        }
        mvaddch(2 * i + 1, 0, ACS_VLINE);
        if (i > 0)
            for (size_t j = 1; j < NUM_COLS + 1; j++)
                mvaddch(2 * i + 1, (CELL_DISPLAY_WIDTH + 1) * j, ACS_VLINE);
        mvaddch(2 * i + 1, total_width - 1, ACS_VLINE);
    }

    // Draw the bottom line.
    mvaddch(total_height - 1, 0, ACS_LLCORNER);
    for (size_t i = 0; i < NUM_COLS + 1; i++) {
        if (i > 0)
            addch(ACS_BTEE);
        for (size_t j = 0; j < CELL_DISPLAY_WIDTH; j++)
            addch(ACS_HLINE);
    }
    addch(ACS_LRCORNER);

    // Draw exit instructions.
    mvaddstr(total_height, 0, "Press Ctrl+C to exit.");

    /* HEADERS */

    // Print the column headers.
    for (COL col = COL_A; col < NUM_COLS; col++)
        mvaddch(3, (CELL_DISPLAY_WIDTH + 1) * (col + 1) + CELL_DISPLAY_WIDTH / 2 + 1, col + 'A');

    // Generate the format specifier for the cur_row headers.
    char format_buffer[8];
    snprintf(format_buffer, sizeof(format_buffer), "%%%dd", CELL_DISPLAY_WIDTH);

    // Print the cur_row headers.
    for (ROW row = ROW_1; row < NUM_ROWS; row++)
        mvprintw(2 * ((int) row + 2) + 1, 1, format_buffer, row + 1);

    /* MAIN LOOP */

    // Initialize data structure.
    model_init();

    // String of blanks used by main loop.
    char blanks[total_width + 1];
    for (size_t i = 0; i < total_width; i++)
        blanks[i] = ' ';
    blanks[total_width] = 0;

    while (true) {
        // Print the current cell coordinates in top-left corner.
        mvaddnstr(3, 1, blanks, CELL_DISPLAY_WIDTH);
        mvprintw(3, CELL_DISPLAY_WIDTH / 2, "%c%d", cur_col + 'A', cur_row + 1);

        // Show the textual representation of the current cell in the edit field.
        if (edit_text != NULL)
            free(edit_text);
        edit_text = get_textual_value(cur_row, cur_col);
        edit_text_capacity = edit_text == NULL ? 0 : strlen(edit_text);
        edit_text_length = edit_text_capacity;
        mvaddnstr(1, 1, blanks, total_width - 2);
        if (edit_text != NULL)
            mvaddnstr(1, 1, edit_text, total_width - 2);

        // Highlight the current cell.
        set_cell_attr(A_REVERSE);
        refresh();

        // Read next key.
        int c = getch();
        set_cell_attr(A_NORMAL);

        // Handle key.
        handle_key:
        switch (c) {
            case 3: // Ctrl+C
                endwin();
                return 0;
            case KEY_UP:
                if (cur_row > ROW_1)
                    cur_row--;
                continue;
            case KEY_DOWN:
                if (cur_row < NUM_ROWS - 1)
                    cur_row++;
                continue;
            case KEY_LEFT:
                if (cur_col > COL_A)
                    cur_col--;
                return_col = cur_col;
                continue;
            case KEY_RIGHT:
                if (cur_col < NUM_COLS - 1)
                    cur_col++;
                return_col = cur_col;
                continue;
            case KEY_PPAGE:
                cur_row = ROW_1;
                continue;
            case KEY_NPAGE:
                cur_row = NUM_ROWS - 1;
                continue;
            case KEY_HOME:
                cur_col = COL_A;
                return_col = COL_A;
                continue;
            case KEY_END:
                cur_col = NUM_COLS - 1;
                return_col = NUM_COLS - 1;
                continue;
            case '\t':
                if (cur_col < NUM_COLS - 1)
                    cur_col++;
                continue;
            case KEY_DC:
                clear_cell(cur_row, cur_col);
                continue;
            case '\n':
                if (cur_row < NUM_ROWS - 1) {
                    cur_row++;
                    cur_col = return_col;
                }
                continue;
            case ' ':
                // Edit the current cell without deleting anything.
                ensure_edit_text_capacity(1);
                edit_position = edit_text_length;
                break;
            default:
                if (isgraph(c)) {
                    // Clear the edit text and start typing a new value.
                    ensure_edit_text_capacity(1);
                    edit_text[0] = (char) c;
                    edit_text_length = 1;
                    edit_position = 1;
                }
                break;
        }

        // Editing cell value.
        while (true) {
            // Ensure relevant part of edit text is visible.
            if (edit_position < edit_display_offset)
                edit_display_offset = 0;
            if (edit_position - edit_display_offset >= total_width - 1)
                edit_display_offset = edit_position - total_width + 2;

            // Display edit text.
            mvaddnstr(1, 1, blanks, total_width - 2);
            size_t display_amount = edit_text_length - edit_display_offset;
            if (display_amount > total_width - 2)
                display_amount = total_width - 2;
            mvaddnstr(1, 1, edit_text + edit_display_offset, display_amount);
            move(1, edit_position - edit_display_offset + 1);

            // Read next key of input.
            c = getch();

            switch (c) {
                case 3: // Ctrl+C
                    endwin();
                    return 0;
                case KEY_LEFT:
                    if (edit_position > 0)
                        edit_position--;
                    continue;
                case KEY_RIGHT:
                    if (edit_position < edit_text_length)
                        edit_position++;
                    continue;
                case KEY_HOME:
                    edit_position = 0;
                    continue;
                case KEY_END:
                    edit_position = edit_text_length;
                    continue;
                case KEY_UP:
                case KEY_DOWN:
                case KEY_PPAGE:
                case KEY_NPAGE:
                case 0033: // Escape key.
                    // Cancel edit and navigate as usual.
                    free(edit_text);
                    edit_text = NULL;
                    edit_text_capacity = 0;
                    edit_text_length = 0;
                    goto handle_key;
                case KEY_BACKSPACE:
                case 0010: // ASCII backspace.
                    if (edit_position > 0) {
                        edit_position--;
                        edit_text_length--;
                        memmove(edit_text + edit_position, edit_text + edit_position + 1,
                                edit_text_length - edit_position);
                    }
                    continue;
                case KEY_DC:
                    if (edit_position < edit_text_length) {
                        edit_text_length--;
                        memmove(edit_text + edit_position, edit_text + edit_position + 1,
                                edit_text_length - edit_position);
                    }
                    continue;
                case '\t':
                case '\n':
                    // Apply edit and navigate as usual.
                    ensure_edit_text_capacity(edit_text_length + 1);
                    edit_text[edit_text_length] = 0;
                    set_cell_value(cur_row, cur_col, edit_text);
                    edit_text = NULL;
                    edit_text_capacity = 0;
                    edit_text_length = 0;
                    goto handle_key;
                default:
                    if (isgraph(c)) {
                        ensure_edit_text_capacity(edit_text_length + 1);
                        memmove(edit_text + edit_position + 1, edit_text + edit_position,
                                edit_text_length - edit_position);
                        edit_text[edit_position++] = (char) c;
                        edit_text_length++;
                    }
                    continue;
            }
        }
    }
}

void update_cell_display(ROW row, COL col, const char *text) {
    int console_row = 2 * ((int) row + 2) + 1;
    int console_col = (CELL_DISPLAY_WIDTH + 1) * (col + 1) + 1;
    char blanks[CELL_DISPLAY_WIDTH + 1];
    for (size_t i = 0; i < CELL_DISPLAY_WIDTH; i++)
        blanks[i] = ' ';
    blanks[CELL_DISPLAY_WIDTH] = '\0';
    mvaddstr(console_row, console_col, blanks);
    mvaddnstr(console_row, console_col, text, CELL_DISPLAY_WIDTH);
}
