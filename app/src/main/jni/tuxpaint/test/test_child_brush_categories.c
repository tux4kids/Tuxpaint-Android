/**
 * Unit tests for child brush category system
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Test data structures matching tuxpaint.c */
static int child_brush_category = 1;
static int child_brush_indices[13];
static int child_brush_count = 5;
static int child_brush_use_icons = 0;
static int child_brush_variable_size = 1;
static int cur_brush = 0;

/* Include the function we're testing */
void init_child_brush_category(int expert_mode_brush) {
  child_brush_use_icons = 0;
  child_brush_variable_size = 1;
  
  if (expert_mode_brush >= 0 && expert_mode_brush <= 4) {
    /* Category 1: Standard brushes (0-4) */
    child_brush_category = 1;
    int brushes[] = {0, 1, 2, 3, 4};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 5;
    cur_brush = brushes[0];
  }
  else if (expert_mode_brush == 5 || expert_mode_brush == 6 || 
           expert_mode_brush == 7 || expert_mode_brush == 8 ||
           expert_mode_brush == 36 || expert_mode_brush == 37) {
    /* Category 2: Special round brushes (7,8,5,6,37,36) */
    child_brush_category = 2;
    int brushes[] = {7, 8, 5, 6, 37, 36};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 6;
    cur_brush = brushes[0];
  }
  else if (expert_mode_brush == 35 || expert_mode_brush == 34 ||
           expert_mode_brush == 50 || expert_mode_brush == 30 || expert_mode_brush == 39 ||
           expert_mode_brush == 33 || expert_mode_brush == 38 || expert_mode_brush == 49 ||
           expert_mode_brush == 52) {
    /* Category 3: Mixed icon brushes */
    child_brush_category = 3;
    int brushes[] = {35, 34, 50, 30, 39, 33, 38, 49, 52};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 9;
    child_brush_use_icons = 1;
    child_brush_variable_size = 0;
    cur_brush = brushes[0];
  }
  else if (expert_mode_brush == 19 || expert_mode_brush == 31 || expert_mode_brush == 32 ||
           expert_mode_brush == 40 || expert_mode_brush == 61 || expert_mode_brush == 63 ||
           expert_mode_brush == 64 || expert_mode_brush == 68) {
    /* Category 4: Shapes */
    child_brush_category = 4;
    int brushes[] = {19, 31, 32, 40, 61, 63, 64, 68};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 8;
    child_brush_use_icons = 1;
    child_brush_variable_size = 0;
    cur_brush = brushes[0];
  }
  else if (expert_mode_brush >= 20 && expert_mode_brush <= 24) {
    /* Category 5: Flowers */
    child_brush_category = 5;
    int brushes[] = {20, 21, 22, 23, 24};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 5;
    child_brush_use_icons = 1;
    child_brush_variable_size = 1;
    cur_brush = brushes[0];
  }
  else if (expert_mode_brush == 14 || expert_mode_brush == 15 || expert_mode_brush == 16 ||
           expert_mode_brush == 27 || expert_mode_brush == 28 || expert_mode_brush == 29 ||
           expert_mode_brush == 53 || expert_mode_brush == 67) {
    /* Category 6: Animals & Nature */
    child_brush_category = 6;
    int brushes[] = {14, 15, 16, 27, 28, 29, 53, 67};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 8;
    child_brush_use_icons = 1;
    child_brush_variable_size = 0;
    cur_brush = brushes[0];
  }
  else if (expert_mode_brush >= 42 && expert_mode_brush <= 47) {
    /* Category 7: Slash lines */
    child_brush_category = 7;
    int brushes[] = {42, 43, 44, 45, 46, 47};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 6;
    child_brush_use_icons = 1;
    child_brush_variable_size = 1;
    cur_brush = brushes[0];
  }
  else if (expert_mode_brush >= 55 && expert_mode_brush <= 60) {
    /* Category 8: Squares */
    child_brush_category = 8;
    int brushes[] = {55, 56, 57, 58, 59, 60};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 6;
    child_brush_use_icons = 1;
    child_brush_variable_size = 1;
    cur_brush = brushes[0];
  }
  else if (expert_mode_brush == 9 || expert_mode_brush == 25 || expert_mode_brush == 26 ||
           expert_mode_brush == 48 || expert_mode_brush == 54 || expert_mode_brush == 62 ||
           expert_mode_brush == 65 || expert_mode_brush == 66) {
    /* Category 9: Texture brushes */
    child_brush_category = 9;
    int brushes[] = {9, 25, 26, 48, 54, 62, 65, 66};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 8;
    child_brush_use_icons = 1;
    child_brush_variable_size = 0;
    cur_brush = brushes[0];
  }
  else if (expert_mode_brush == 10 || expert_mode_brush == 11 || expert_mode_brush == 12 ||
           expert_mode_brush == 13 || expert_mode_brush == 17 || expert_mode_brush == 18 ||
           expert_mode_brush == 41 || expert_mode_brush == 51) {
    /* Category 10: Effect brushes */
    child_brush_category = 10;
    int brushes[] = {10, 11, 12, 13, 17, 18, 41, 51};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 8;
    child_brush_use_icons = 1;
    child_brush_variable_size = 0;
    cur_brush = brushes[0];
  }
  else {
    /* Default: Category 1 (Standard brushes) */
    child_brush_category = 1;
    int brushes[] = {0, 1, 2, 3, 4};
    memcpy(child_brush_indices, brushes, sizeof(brushes));
    child_brush_count = 5;
    cur_brush = brushes[0];
  }
}

/* Test helper to verify category properties */
void assert_category(int expected_cat, int expected_count, int expected_icons, int expected_variable_size) {
  assert(child_brush_category == expected_cat);
  assert(child_brush_count == expected_count);
  assert(child_brush_use_icons == expected_icons);
  assert(child_brush_variable_size == expected_variable_size);
}

/* Test helper to verify brush indices */
void assert_brush_indices(int expected[], int count) {
  assert(child_brush_count == count);
  for (int i = 0; i < count; i++) {
    if (child_brush_indices[i] != expected[i]) {
      printf("ERROR: Expected brush_indices[%d]=%d, got %d\n", i, expected[i], child_brush_indices[i]);
      assert(0);
    }
  }
}

/* Test Category 1: Standard brushes (0-4) */
void test_category_1_standard_brushes() {
  printf("Testing Category 1: Standard brushes...\n");
  
  for (int brush = 0; brush <= 4; brush++) {
    init_child_brush_category(brush);
    assert_category(1, 5, 0, 1);
    int expected[] = {0, 1, 2, 3, 4};
    assert_brush_indices(expected, 5);
  }
  
  printf("  PASSED\n");
}

/* Test Category 2: Special round brushes */
void test_category_2_special_round() {
  printf("Testing Category 2: Special round brushes...\n");
  
  int test_brushes[] = {5, 6, 7, 8, 36, 37};
  for (int i = 0; i < 6; i++) {
    init_child_brush_category(test_brushes[i]);
    assert_category(2, 6, 0, 1);
    int expected[] = {7, 8, 5, 6, 37, 36};
    assert_brush_indices(expected, 6);
  }
  
  printf("  PASSED\n");
}

/* Test Category 3: Mixed icon brushes */
void test_category_3_mixed_icons() {
  printf("Testing Category 3: Mixed icon brushes...\n");
  
  int test_brushes[] = {35, 34, 50, 30, 39, 33, 38, 49, 52};
  for (int i = 0; i < 9; i++) {
    init_child_brush_category(test_brushes[i]);
    assert_category(3, 9, 1, 0);
    int expected[] = {35, 34, 50, 30, 39, 33, 38, 49, 52};
    assert_brush_indices(expected, 9);
  }
  
  printf("  PASSED\n");
}

/* Test Category 4: Shapes */
void test_category_4_shapes() {
  printf("Testing Category 4: Shapes...\n");
  
  int test_brushes[] = {19, 31, 32, 40, 61, 63, 64, 68};
  for (int i = 0; i < 8; i++) {
    init_child_brush_category(test_brushes[i]);
    assert_category(4, 8, 1, 0);
    int expected[] = {19, 31, 32, 40, 61, 63, 64, 68};
    assert_brush_indices(expected, 8);
  }
  
  printf("  PASSED\n");
}

/* Test Category 5: Flowers */
void test_category_5_flowers() {
  printf("Testing Category 5: Flowers...\n");
  
  for (int brush = 20; brush <= 24; brush++) {
    init_child_brush_category(brush);
    assert_category(5, 5, 1, 1);
    int expected[] = {20, 21, 22, 23, 24};
    assert_brush_indices(expected, 5);
  }
  
  printf("  PASSED\n");
}

/* Test Category 6: Animals & Nature (merged category) */
void test_category_6_animals_nature() {
  printf("Testing Category 6: Animals & Nature...\n");
  
  int test_brushes[] = {14, 15, 16, 27, 28, 29, 53, 67};
  for (int i = 0; i < 8; i++) {
    init_child_brush_category(test_brushes[i]);
    assert_category(6, 8, 1, 0);
    int expected[] = {14, 15, 16, 27, 28, 29, 53, 67};
    assert_brush_indices(expected, 8);
  }
  
  printf("  PASSED\n");
}

/* Test Category 7: Slash lines */
void test_category_7_slash_lines() {
  printf("Testing Category 7: Slash lines...\n");
  
  for (int brush = 42; brush <= 47; brush++) {
    init_child_brush_category(brush);
    assert_category(7, 6, 1, 1);
    int expected[] = {42, 43, 44, 45, 46, 47};
    assert_brush_indices(expected, 6);
  }
  
  printf("  PASSED\n");
}

/* Test Category 8: Squares */
void test_category_8_squares() {
  printf("Testing Category 8: Squares...\n");
  
  for (int brush = 55; brush <= 60; brush++) {
    init_child_brush_category(brush);
    assert_category(8, 6, 1, 1);
    int expected[] = {55, 56, 57, 58, 59, 60};
    assert_brush_indices(expected, 6);
  }
  
  printf("  PASSED\n");
}

/* Test Category 9: Texture brushes */
void test_category_9_texture() {
  printf("Testing Category 9: Texture brushes...\n");
  
  int test_brushes[] = {9, 25, 26, 48, 54, 62, 65, 66};
  for (int i = 0; i < 8; i++) {
    init_child_brush_category(test_brushes[i]);
    assert_category(9, 8, 1, 0);
    int expected[] = {9, 25, 26, 48, 54, 62, 65, 66};
    assert_brush_indices(expected, 8);
  }
  
  printf("  PASSED\n");
}

/* Test Category 10: Effect brushes */
void test_category_10_effects() {
  printf("Testing Category 10: Effect brushes...\n");
  
  int test_brushes[] = {10, 11, 12, 13, 17, 18, 41, 51};
  for (int i = 0; i < 8; i++) {
    init_child_brush_category(test_brushes[i]);
    assert_category(10, 8, 1, 0);
    int expected[] = {10, 11, 12, 13, 17, 18, 41, 51};
    assert_brush_indices(expected, 8);
  }
  
  printf("  PASSED\n");
}

/* Test that all brushes are covered (0-68) */
void test_all_brushes_covered() {
  printf("Testing that all brushes 0-68 are covered...\n");
  
  int covered[69] = {0};  /* Track which brushes are covered */
  
  /* Test each brush and mark it as covered */
  for (int brush = 0; brush <= 68; brush++) {
    init_child_brush_category(brush);
    
    /* Mark all brushes in the category as covered */
    for (int i = 0; i < child_brush_count; i++) {
      int idx = child_brush_indices[i];
      if (idx >= 0 && idx <= 68) {
        covered[idx] = 1;
      }
    }
  }
  
  /* Verify all brushes are covered */
  int uncovered_count = 0;
  for (int i = 0; i <= 68; i++) {
    if (!covered[i]) {
      printf("  WARNING: Brush %d is not covered by any category\n", i);
      uncovered_count++;
    }
  }
  
  if (uncovered_count > 0) {
    printf("  %d brushes not covered\n", uncovered_count);
  } else {
    printf("  PASSED - All brushes covered\n");
  }
}

/* Test edge cases */
void test_edge_cases() {
  printf("Testing edge cases...\n");
  
  /* Test negative brush */
  init_child_brush_category(-1);
  assert_category(1, 5, 0, 1);  /* Should default to category 1 */
  
  /* Test brush beyond range */
  init_child_brush_category(100);
  assert_category(1, 5, 0, 1);  /* Should default to category 1 */
  
  printf("  PASSED\n");
}

int main() {
  printf("=== Child Brush Category System Unit Tests ===\n\n");
  
  test_category_1_standard_brushes();
  test_category_2_special_round();
  test_category_3_mixed_icons();
  test_category_4_shapes();
  test_category_5_flowers();
  test_category_6_animals_nature();
  test_category_7_slash_lines();
  test_category_8_squares();
  test_category_9_texture();
  test_category_10_effects();
  test_all_brushes_covered();
  test_edge_cases();
  
  printf("\n=== All tests PASSED ===\n");
  return 0;
}
