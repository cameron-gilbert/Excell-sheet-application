#ifndef ASSIGNMENT_MODEL_H
#define ASSIGNMENT_MODEL_H

#include "defs.h"

// Initializes the data structure.
//
// This is called once, at program start.
void model_init();

// Sets the value of a cell based on user input.
//
// The string referred to by 'text' is now owned by this function and/or the
// cell contents data structure; it is its responsibility to ensure it is freed
// once it is no longer needed.
void set_cell_value(ROW row, COL col, char *text);

// Clears the value of a cell.
void clear_cell(ROW row, COL col);

// Gets a textual representation of the value of a cell, for editing.
//
// The returned string must have been allocated using 'malloc' and is now owned
// by the interface. The cell contents data structure must not modify it or
// retain any reference to it after the function returns.
char *get_textual_value(ROW row, COL col);

#endif //ASSIGNMENT_MODEL_H
