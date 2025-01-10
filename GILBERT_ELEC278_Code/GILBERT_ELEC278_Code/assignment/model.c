#include "model.h"
#include "interface.h"
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define CIRCULAR_DEPENDENCY 1
#define INVALID_FUNCTION 2

//Multiple options for each cell so that the program can check before doing operations
typedef enum {TEXT, NUMBER, FORMULA} CellType;

//This is a linked list used for holding dependencies between cells. Each cell will hold the row
// and column of the cells that dep on it. Therefore, when a new dependency is added to a cell, the list is made longer
typedef struct{
    int row;
    int col;
    struct DeptNode *next;
}DepNode;

//Each cell will consist of its type, a string holding its textual value, its value in a double variable
// and its dependencies
typedef struct{
    CellType type;
    char *text;
    double number;
    DepNode *dependTo;
}Cell;


// Spreadsheet will be organised into a 2d array of cells
typedef struct {
    Cell **cells;
}Spreadsheet;
// Global variable to store the spreadsheet
Spreadsheet *mySpreadsheet = NULL;
//Function that initialises each cell and its memory
void model_init() {
    mySpreadsheet = malloc(sizeof(Spreadsheet)); //Memory allocation
    mySpreadsheet->cells = malloc(NUM_ROWS * sizeof(Cell *)); //Allocating memory for each row
    for (int i = 0; i < NUM_ROWS; ++i) { //Allocating memory for each cell in those rows
        mySpreadsheet->cells[i] = malloc(NUM_COLS * sizeof(Cell));
        for (int j = 0; j < NUM_COLS; ++j) { //Allocating for all the rest of the cells in other columns
            mySpreadsheet->cells[i][j].type = TEXT; // Setting initial state of all cells to text
            mySpreadsheet->cells[i][j].dependTo = NULL; //Setting all cells to have no initial dependencies
            mySpreadsheet->cells[i][j].text = NULL; // Setting all cells to have NULL initial value
        }
    }
}
//This function is used to add a dependency to cell. This is done through taking in the row and column value of
// the cell which is being edited. Then it also takes in the cell that it is going to depend on.
void add_dependency(Cell *cell, int depRow, int depCol) {
    DepNode *newNode = malloc(sizeof(DepNode));
    if (newNode == NULL) {
        // Handle memory allocation failure
        return;
    }
    newNode->row = depRow; //adding value of row and column of cell currently being edited into the node
    newNode->col = depCol;
    newNode->next = cell->dependTo; //pointing the list to the next node in the cell being depended on
    cell->dependTo = newNode;  //Setting this node to the cell being depended on
}

//This function clears a certain dependency which a cell relies on in case of an equation being changed. It
//Takes in the same parameters that add dependency function uses.
void clear_dependencies(Cell *cell, ROW row, COL col) {
    DepNode **current = &(cell->dependTo);

    while (*current != NULL) { //Runs through the list of dependencies inside the cell being depended on
        if ((*current)->row == row && (*current)->col == col) { //Code to find when the right node is found.
            DepNode *temp = *current;
            *current = (*current)->next;
            free(temp); //Frees the node and tells the list to point to the next node
        } else {
            current = &((*current)->next);
        }
    }
}

//This function finds all cells that depend on it and updates them
void update_dependent_cells(ROW row, COL col) {
    Cell *cell = &mySpreadsheet->cells[row][col];
    DepNode *current = cell->dependTo;

    if(current == NULL){
        return;
    }
    while (current != NULL) { //Goes through the list until its end
        int row1 = current->row;
        int col1 = current->col;
        set_cell_value(row1, col1, mySpreadsheet->cells[row1][col1].text); //goes and sets value for certain cell
        update_dependent_cells(row1, col1); //Recursive in the case of a line of cells depending on each other
        current = current->next;
    }

}

//This function checks to see if a certain cell is already being depended on which would cause circular dep
void isCircularDependency(int row, int col, int row2, int col2) {
    Cell *cell = &mySpreadsheet->cells[row2][col2];
    DepNode *current1 = cell->dependTo;

    while (current1 != NULL) { //loops until end of list
        if (current1->row == row && current1->col == col) { //in case where values are matching program exits
            exit(CIRCULAR_DEPENDENCY);
        } else {
            current1 = current1->next;
        }
    }
}

//This is a function that gives each letter a numeric value representing its column
int letterCase(char *str){
    char letter = toupper(str[0]); // must get first character which will be the letter
    int nbrCol;
    switch(letter){
        case 'A':
            nbrCol = 0;
            break;
        case 'B':
            nbrCol = 1;
            break;
        case 'C':
            nbrCol = 2;
            break;
        case 'D':
            nbrCol = 3;
            break;
        case 'E':
            nbrCol = 4;
            break;
        case 'F':
            nbrCol = 5;
            break;
        case 'G':
            nbrCol = 6;
            break;
    }
    return nbrCol;
}

//This is a parsing function that is used to split up the equation given and equate it
double functionEq(char *str, int curRow, int curCol) {
    char *operator = "+";
    int maxCh = 12; //This is max size of cell capacity + NULL char therefore should be max of an equation being entered
    char *portion[maxCh]; //this is the array of strings that will hold each part of the equation
    int Count = 0; //Increment variable
    int parts = 0; //Counts how many parts are in the equation
    if (str[0] == '=') { // to take out the equals sign
        str++;
    }
    char *token = strtok(str, operator); //Splits up string into array of its string portions
    while (token != NULL && Count < maxCh) {
        portion[Count] = token;
        parts++;
        Count++;
        token = strtok(NULL, operator); //each portion will be split through operands
    }
    Count = 0; //increment reset
    double sum = 0; //will be total sum of equation
    double number = 0; //variable that temp hold the value of a number in equation
    while (Count < parts) { //this loop traverses through the array, adding each portion to the sum
        char *ptr1;
        number = strtod(portion[Count], &ptr1); //splits up string into double which takes in any numbers
        // until a value other than a number is found and the str is updated, taking out any numbers which were chosen
        if (*ptr1 == '\0') { //in the case of a number the only portion left in the string will be null value
            sum += number;
        } else {
            char *cellRef = strdup(portion[Count]); //holds the current part of equation
            if (cellRef[2] != '\0'){
                exit(INVALID_FUNCTION);
            }
            int column = letterCase(cellRef); //switches column letter into number
            int row = cellRef[1] - '0' - 1; //second char of part should hold row
            free(cellRef);
            if (column >= 0 && column < NUM_COLS && row >= 0 && row < NUM_ROWS) { //insures valid information given
                if (mySpreadsheet->cells[row][column].type == NUMBER ||
                    mySpreadsheet->cells[row][column].type == FORMULA) { //makes sure the function will dep on a valid cell
                    if(column == curCol && row == curRow){ //Error case user inputs cells own value into equation
                        exit(INVALID_FUNCTION);
                    }
                    (isCircularDependency(row, column, curRow, curCol)); //Checks circular dep

                    sum += mySpreadsheet->cells[row][column].number; //adds value of cell to sum
                    add_dependency(&mySpreadsheet->cells[row][column], curRow, curCol); //adds dep
                }
                else{ //Error case when user adds a cell to the formula which does not contain a number
                    exit(INVALID_FUNCTION);
                }
            }
        }
        Count++;
        number = 0;
    }
    return sum;
}

//This is a parsing function that is used to split up the equation given clear the dependencies of cells
// of any cells mentioned. Takes in the text, row and col of a past function added. Therefore, fewer checks are
// required as it has already been checked to be valid
void clearDep(char *str, int curRow, int curCol) {
    char *operator = "+";
    int maxCh = 12;
    char *portion[maxCh];
    int Count = 0;
    int parts = 0;
    if (str[0] == '=') {
        str++;
    }
    char *token = strtok(str, operator);
    while (token != NULL && Count < maxCh) {
        portion[Count] = token;
        parts++;
        Count++;
        token = strtok(NULL, operator);
    }

    Count = 0;
    double number = 0;
    while (Count < parts) { //This loop traverses through the array
        char *ptr1;
        number = strtod(portion[Count], &ptr1);
        if (*ptr1 == '\0'); //in case of number in an equation there is no dependencies so can be ignored
        else {
            char *cellRef = strdup(portion[Count]);
            int column = letterCase(cellRef);
            int row = cellRef[1] - '0' - 1;
            if (column >= 0 && column < NUM_COLS && row >= 0 && row < NUM_ROWS) { //as this is a past function added
                if (mySpreadsheet->cells[row][column].type == NUMBER ||           //it is known to be valid so does not
                    mySpreadsheet->cells[row][column].type == FORMULA) {          //need to be checked
                    clear_dependencies(&mySpreadsheet->cells[row][column], curRow, curCol);
                    //clears dependencies of the certain cell
                }
            }
        free(cellRef);
        }
        Count++;
        number = 0;
    }
}

void set_cell_value(ROW row, COL col, char *text) {
    if (mySpreadsheet->cells[row][col].type == FORMULA) { //when a cell is changed must check if it was formula before
        char *preEqn = mySpreadsheet->cells[row][col].text; //In that case it will clear its past dependencies
        clearDep(strdup(preEqn), row, col);
    }

    // Free any existing string
    if (mySpreadsheet->cells[row][col].type == TEXT || mySpreadsheet->cells[row][col].type == FORMULA) {
        free(mySpreadsheet->cells[row][col].text);
        mySpreadsheet->cells[row][col].text = NULL;
    }
    if (text[0] == '=') { //in the case it is an equation
        mySpreadsheet->cells[row][col].type = FORMULA; //set type to formula
        char *equation = strdup(text);
        mySpreadsheet->cells[row][col].text = equation; //Sets its text to its textual(raw, not calculated) value

        double number = functionEq(strdup(equation), row, col); //finds the sum of the equation

        mySpreadsheet->cells[row][col].number = number; //Sets the equations numerical value to the sum
        char *displayValue = malloc(32 * sizeof(char)); //Switches numerical value to string so that it can be displayed
        snprintf(displayValue, 32, "%0.2f", number);
        update_dependent_cells(row, col); //Updates any cells that are dependent on this cell
        update_cell_display(row, col, displayValue);
    }
    else {
        char *ptr1;
        strtod(text, &ptr1); //uses same method as in functionEq to find if it is number
        if (*ptr1 == '\0') {
            mySpreadsheet->cells[row][col].type = NUMBER; //sets type to number
            mySpreadsheet->cells[row][col].number = strtod(text, NULL); //sets it to number given
            update_dependent_cells(row, col); //updates any cells dependent on this cell
        } else {
            mySpreadsheet->cells[row][col].type = TEXT; //sets type
            mySpreadsheet->cells[row][col].text = strdup(text);
            if (mySpreadsheet->cells[row][col].dependTo != NULL){ //checks to makes sure there are no cells depending on it
                exit(INVALID_FUNCTION);
            }
        }
        update_cell_display(row, col, text); //updates cell display
    }
}

//clears a cell
void clear_cell(ROW row, COL col) {
    if (mySpreadsheet->cells[row][col].type == FORMULA) { //when a cell is changed must check if it was formula before
        char *preEqn = mySpreadsheet->cells[row][col].text; //In that case it will clear its past dependencies
        clearDep(strdup(preEqn), row, col);
    }
    if (mySpreadsheet->cells[row][col].type == TEXT || mySpreadsheet->cells[row][col].type == FORMULA) { //in case of formula or text memory must be freed
        free(mySpreadsheet->cells[row][col].text);
        mySpreadsheet->cells[row][col].text = NULL; //set back to null
    }
    mySpreadsheet->cells[row][col].number = 0;
    mySpreadsheet->cells[row][col].type = TEXT; //rest cell back to initial state (text)
    update_cell_display(row, col, "");
}

//This function puts str into top bar which shows what is inside a cell
char *get_textual_value(ROW row, COL col) {
    char *displayValue = NULL;
    if (mySpreadsheet -> cells[row][col].type == TEXT || mySpreadsheet->cells[row][col].type == FORMULA){
        displayValue = strdup(mySpreadsheet->cells[row][col].text); //in the case of formula or text type
    }
    else if(mySpreadsheet -> cells[row][col].type == NUMBER){
        double number = mySpreadsheet -> cells[row][col].number;
        displayValue = malloc(32 * sizeof(char)); // Allocate memory
        if (displayValue != NULL) {
            snprintf(displayValue, 32, "%0.2f", number);  //transfer number into string
        }
    }
    return displayValue;
}



