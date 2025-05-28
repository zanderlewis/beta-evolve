#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// BETA EVOLVE START
// Initial basic sorting implementation
// Make it a really fast and efficient sorting algorithm
void sort(int arr[], int n) {
    // Simple bubble sort as starting point
    for (int i = 0; i < n-1; i++) {
        for (int j = 0; j < n-i-1; j++) {
            if (arr[j] > arr[j+1]) {
                int temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            }
        }
    }
}
// BETA EVOLVE END

// BETA EVOLVE START
// Testing and validation functions
int validate_sorting(int arr[], int n) {
    for (int i = 0; i < n-1; i++) {
        if (arr[i] > arr[i+1]) {
            return 0; // Failed
        }
    }
    return 1; // Success
}
// BETA EVOLVE END

int main() {
    // Test the sorting algorithm
    int test_arr[] = {64, 34, 25, 12, 22, 11, 90};
    int n = sizeof(test_arr)/sizeof(test_arr[0]);
    
    printf("Original array: ");
    for (int i = 0; i < n; i++) {
        printf("%d ", test_arr[i]);
    }
    printf("\n");
    
    sort(test_arr, n);
    
    printf("Sorted array: ");
    for (int i = 0; i < n; i++) {
        printf("%d ", test_arr[i]);
    }
    printf("\n");
    
    // Validate sorting correctness
    if (validate_sorting(test_arr, n)) {
        printf("SORTING SUCCESS!\n");
        return 0;
    } else {
        printf("SORTING FAILED!\n");
        return 1;
    }
}
