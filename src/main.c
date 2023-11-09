#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "bitmap.h"

#define MAX_WIDTH 32
#define MAX_HEIGHT 32
#define MINIMUM_IMAGE_BYTES 128

// Function to display a template from the template file.
void displayTemplate(FILE *template_file, int template_index) {
    uint8_t num_components;
    // Read the number of components from the binary file.
    if (fread(&num_components, sizeof(uint8_t), 1, template_file) != 1) {
        printf("Error: Unable to read the number of components from the binary file.\n");
        exit(1);
    }

    // Check if the template index is within a valid range.
    if (template_index < 0 || template_index >= num_components) {
        printf("Template index out of range\n");
        exit(1);
    }

    // Seek to the position of the specified template within the file.
    fseek(template_file, MINIMUM_IMAGE_BYTES * template_index + 1, SEEK_SET);

    int bit[MAX_WIDTH * MAX_HEIGHT];
    int bit_index = 0;
    uint8_t byte;

    printf("Template data:\n");

    // Read and process the binary data of the template.
    while (bit_index < MAX_WIDTH * MAX_HEIGHT && fread(&byte, sizeof(uint8_t), 1, template_file) == 1) {
        for (int i = 7; i >= 0; i--) {
            bit[bit_index++] = (byte >> i) & 1;
        }
    }

    int line_index = 0;
    for (int i = bit_index - MAX_WIDTH; i > -1; i++) {
        if (line_index == MAX_WIDTH * MAX_HEIGHT) {
            printf("\n");
            break;
        }
        printf("%c", bit[i] ? '1' : ' ');
        if ((i + 1) % MAX_WIDTH == 0 && line_index != 0) {
            i -= 2 * MAX_WIDTH;
            printf("\n");
        }
        line_index++;
    }
}

// Function to find components in a bitmap based on templates.
void find_components(FILE* read_file, char* bmp_file) {
    Bmp bmp = read_bmp(bmp_file);
    int pcb_height = bmp.height;
    int pcb_width = bmp.width;

    int bmp_binary[pcb_height][pcb_width];

    // Convert the BMP file to a binary representation.
    for (int y = 0; y < pcb_height; y++) {
        for (int x = 0; x < pcb_width; x++) {
            double averageRGB = (bmp.pixels[y][x][RED] + bmp.pixels[y][x][BLUE] + bmp.pixels[y][x][GREEN]) / 3;
            bmp_binary[y][x] = (averageRGB >= MINIMUM_IMAGE_BYTES) ? 1 : 0;
        }
    }

    fseek(read_file, 0, SEEK_SET);
    uint8_t num_components_byte;
    fread(&num_components_byte, 1, 1, read_file);

    int num_components = num_components_byte;
    fseek(read_file, 1, SEEK_SET);

    int bit_all_components[num_components][MAX_WIDTH * MAX_HEIGHT];

    // Read and store template data for all components.
    for (int component_index = 0; component_index < num_components; component_index++) {
        fseek(read_file, (MINIMUM_IMAGE_BYTES * component_index + 1), SEEK_SET);

        int indexBit = 0;
        uint8_t component_byte;

        while (fread(&component_byte, 1, 1, read_file) == 1 && indexBit < MAX_WIDTH * MAX_HEIGHT) {
            for (int i = 7; i >= 0; i--) {
                bit_all_components[component_index][indexBit] = (component_byte >> i) & 1;
                indexBit++;
            }
        }
    }

    int found_compo_type[200];
    int row_pos[200];
    int col_pos[200];
    int num_found_components = 0;

    // Iterate through the BMP to find matching components.
    for (int row = 0; row <= pcb_height - MAX_HEIGHT; row++) {
        for (int col = 0; col <= pcb_width - MAX_WIDTH; col++) {
            for (int component_index = 0; component_index < num_components; component_index++) {
                bool Component = true;

                for (int i = 0; i < MAX_HEIGHT && Component; i++) {
                    for (int j = 0; j < MAX_WIDTH; j++) {
                        int pcbRow = row + i;
                        int pcbCol = col + j;

                        if (pcbRow < 0 || pcbRow >= pcb_height || pcbCol < 0 || pcbCol >= pcb_width) {
                            if (bit_all_components[component_index][i * MAX_WIDTH + j] == 1) {
                                Component = false;
                                break;
                            }
                        } else if (bmp_binary[pcbRow][pcbCol] != bit_all_components[component_index][i * MAX_WIDTH + j]) {
                            Component = false;
                            break;
                        }
                    }
                }

                if (Component) {
                    found_compo_type[num_found_components] = component_index;
                    row_pos[num_found_components] = row;
                    col_pos[num_found_components] = col;
                    num_found_components++;
                }
            }
        }
    }

    printf("Found %d components:\n", num_found_components);
    for (int i = 0; i < num_found_components; i++) {
        printf("type: %d, row: %d, column: %d\n", found_compo_type[i], row_pos[i], col_pos[i]);
    }

    free_bmp(bmp);
}

// Function to create an empty grid with the given dimensions.
int **create_empty_grid(int pcb_height, int pcb_width) {
    // Allocate memory for an array of pointers to int arrays.
    int **grid = (int **)malloc(pcb_height * sizeof(int *));

    // Iterate through the rows.
    for (int i = 0; i < pcb_height; i++) {
        // Allocate memory for an int array to represent a row in the grid.
        grid[i] = (int *)malloc(pcb_width * sizeof(int));

        // Initialize each element in the row to 0.
        for (int j = 0; j < pcb_width; j++) {
            grid[i][j] = 0;
        }
    }

    // Return a pointer to the created grid.
    return grid;
}

// Function to check if a cell (row, col) is within the bounds of the PCB grid.
bool is_valid_cell(int row, int col, int pcb_height, int pcb_width) {
    return row >= 0 && col >= 0 && row < pcb_height && col < pcb_width;
}

// Recursive function for deep research to check connectivity between cells in a PCB grid.
bool deep_research(int **data, int **visited, int row, int col, int stop_row, int stop_col, int pcb_height, int pcb_width) {
    // Check if the cell is out of bounds or already visited.
    if (!is_valid_cell(row, col, pcb_height, pcb_width) || visited[row][col] == 1) {
        return false;
    }

    // Mark the cell as visited.
    visited[row][col] = 1;

    // If the current cell matches the stop cell, connectivity is established.
    if (row == stop_row && col == stop_col) {
        return true;
    }

    // Define movement directions (up, down, left, right).
    int dr[] = {1, -1, 0, 0};
    int dc[] = {0, 0, 1, -1};

    // Check neighboring cells for connectivity.
    for (int i = 0; i < 4; i++) {
        int new_row = row + dr[i];
        int new_col = col + dc[i];

        // Check if the neighboring cell is within bounds, has the same value, and is not visited.
        if (is_valid_cell(new_row, new_col, pcb_height, pcb_width) &&
            data[new_row][new_col] == data[row][col] &&
            deep_research(data, visited, new_row, new_col, stop_row, stop_col, pcb_height, pcb_width)) {
            return true;
        }
    }

    // If no connectivity is found, return false.
    return false;
}

// Function to mark a rectangular component in the grid with a specified value.
void border_component(int **grid, int row, int col, int numRows, int numCols, int width, int height, int value) {
    // Iterate through the rows within the specified height.
    for (int i = row; i < row + height && i < numRows; i++) {
        // Iterate through the columns within the specified width.
        for (int j = col; j < col + width && j < numCols; j++) {
            // Check if the cell (i, j) is within the grid's bounds.
            if (is_valid_cell(i, j, numRows, numCols)) {
                // Mark the cell with the specified value to represent a component.
                grid[i][j] = value;
            }
        }
    }
}

void check_connection(FILE* read_file, char* bmp_file) {
    Bmp bmp = read_bmp(bmp_file);
    int pcb_height = bmp.height;
    int pcb_width = bmp.width;

    int **bmp_binary = create_empty_grid(pcb_height, pcb_width);

    // Convert the BMP file to a binary representation.
    for (int y = 0; y < pcb_height; y++) {
        for (int x = 0; x < pcb_width; x++) {
            double averageRGB = (bmp.pixels[y][x][RED] + bmp.pixels[y][x][BLUE] + bmp.pixels[y][x][GREEN]) / 3;
            bmp_binary[y][x] = (averageRGB >= MINIMUM_IMAGE_BYTES) ? 1 : 0;
        }
    }

    fseek(read_file, 0, SEEK_SET);
    uint8_t num_components_byte;
    fread(&num_components_byte, 1, 1, read_file);

    int num_components = num_components_byte;
    fseek(read_file, 1, SEEK_SET);

    int bit_all_components[num_components][MAX_WIDTH * MAX_HEIGHT];

    // Read and store template data for all components.
    for (int component_index = 0; component_index < num_components; component_index++) {
        fseek(read_file, (MINIMUM_IMAGE_BYTES * component_index + 1), SEEK_SET);

        int indexBit = 0;
        uint8_t component_byte;

        while (fread(&component_byte, 1, 1, read_file) == 1 && indexBit < MAX_WIDTH * MAX_HEIGHT) {
            for (int i = 7; i >= 0; i--) {
                bit_all_components[component_index][indexBit] = (component_byte >> i) & 1;
                indexBit++;
            }
        }
    }

    int found_compo_type[200];
    int row_pos[200];
    int col_pos[200];
    int found_components = 0;

    // Iterate through the BMP to find matching components.
    for (int row = 0; row <= pcb_height - MAX_HEIGHT; row++) {
        for (int col = 0; col <= pcb_width - MAX_WIDTH; col++) {
            for (int component_index = 0; component_index < num_components; component_index++) {
                bool Component = true;

                for (int i = 0; i < MAX_HEIGHT && Component; i++) {
                    for (int j = 0; j < MAX_WIDTH; j++) {
                        int pcbRow = row + i;
                        int pcbCol = col + j;

                        if (pcbRow < 0 || pcbRow >= pcb_height || pcbCol < 0 || pcbCol >= pcb_width) {
                            if (bit_all_components[component_index][i * MAX_WIDTH + j] == 1) {
                                Component = false;
                                break;
                            }
                        } else if (bmp_binary[pcbRow][pcbCol] != bit_all_components[component_index][i * MAX_WIDTH + j]) {
                            Component = false;
                            break;
                        }
                    }
                }

                if (Component) {
                    found_compo_type[found_components] = component_index;
                    row_pos[found_components] = row;
                    col_pos[found_components] = col;
                    found_components++;
                }
            }
        }
    }

    // Create an empty grid to track connectivity between components.
    create_empty_grid(pcb_height, pcb_width);

// Iterate through the found components to check their connectivity.
    for (int component_check = 0; component_check < found_components; component_check++) {
        int start_row = row_pos[component_check];
        int start_col = col_pos[component_check];
        int num_connect = 0;

        // Initialize an array to store connected components.
        int *connection = (int *)malloc(found_components * sizeof(int));

        // Iterate through other components to check for connectivity.
        for (int component_connected = 0; component_connected < found_components; component_connected++) {
            if (component_connected == component_check) {
                continue;
            }

            // Create an empty grid to track visited cells for connectivity check.
            int **checked = create_empty_grid(pcb_height, pcb_width);

            int stop_row = row_pos[component_connected];
            int stop_col = col_pos[component_connected];

            // Mark other non-relevant components as temporarily disconnected.
            for (int i = 0; i < found_components; i++) {
                if (i != component_check && i != component_connected) {
                    border_component(bmp_binary, row_pos[i], col_pos[i], pcb_height, pcb_width, MAX_WIDTH, MAX_HEIGHT, 2);
                }
            }

            // Check if there is a connection between the two components.
            bool Connected = deep_research(bmp_binary, checked, start_row, start_col, stop_row, stop_col, pcb_height, pcb_width);

            // Restore temporarily disconnected components to their original state.
            for (int i = 0; i < found_components; i++) {
                if (i != component_check && i != component_connected) {
                    border_component(bmp_binary, row_pos[i], col_pos[i], pcb_height, pcb_width, MAX_WIDTH, MAX_HEIGHT, 1);
                }
            }

            // If there is a connection, record the connected component.
            if (Connected) {
                connection[num_connect] = component_connected;
                num_connect++;
            }
        }

        // Print the connectivity information for the current component.
        if (num_connect > 0) {
            printf("Component %d connected to ", component_check);
            for (int i = 0; i < num_connect; i++) {
                printf("%d ", connection[i]);
            }
            printf("\n");
        } else {
            printf("Component %d connected to nothing\n", component_check);
        }

        // Free memory allocated for the connection array.
        free(connection);
    }
    // Free memory
    free_bmp(bmp);
}

int main(int argc, char *argv[]) {
    // Check the number of command-line arguments.
    if (argc != 4) {
        printf("Invalid arguements!\n");
        return 1;
    }

    char mode = argv[1][0];
    // Check the selected mode is or isn't 't', 'l', or 'c'.
    if (mode != 't' && mode != 'l' && mode != 'c') {
        printf("Invalid mode selected!\n");
        return 1;
    }

    char *template_filename = argv[2];
    FILE *template_file = fopen(template_filename, "r");

    // Check if the template file can be opened.
    if (template_file == NULL) {
        printf("Can't load template file\n");
        return 1;
    }

    char *index = argv[3];
    int template_index = atoi(argv[3]);

    if (template_file != NULL) {
        // Call functions based on the selected mode.
        if (mode == 't') {
            displayTemplate(template_file, template_index);
        }

        if (mode == 'l') {
            find_components(template_file, index);
        }

        if (mode == 'c') {
            find_components(template_file, index);
            check_connection(template_file, index);
        }

        // Close the template file.
        fclose(template_file);
    }

    return 0;
}
