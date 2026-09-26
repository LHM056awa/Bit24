#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_COUNT 4
#define OPS_COUNT 9
#define EXPR_SIZE 55 /* worst case: 4*11 digits + 3*2 op chars + 4 parens + NUL */

static const char *ops_str[] = {"+", "-", "*", "/", "&", "|", "^", "<<", ">>"};

/* Check if a float value can be safely converted to unsigned long long (non-negative integer) */
static bool is_integer(float x) {
    return fabsf(x - floorf(x)) < 1e-4f && x >= 0.0f && x <= (float)ULLONG_MAX;
}

static bool apply(float a, float b, int op, float *res) {
    switch (op) {
        case 0:
            *res = a + b;
            return true; /* + */
        case 1:
            *res = a - b;
            return true; /* - */
        case 2:
            *res = a * b;
            return true; /* * */
        case 3:          /* / */
            if (fabsf(b) < 1e-4f) return false;
            *res = a / b;
            return true;
        case 4: /* & */
            if (!is_integer(a) || !is_integer(b)) return false;
            *res = (float)((unsigned long long)a & (unsigned long long)b);
            return true;
        case 5: /* | */
            if (!is_integer(a) || !is_integer(b)) return false;
            *res = (float)((unsigned long long)a | (unsigned long long)b);
            return true;
        case 6: /* ^ */
            if (!is_integer(a) || !is_integer(b)) return false;
            *res = (float)((unsigned long long)a ^ (unsigned long long)b);
            return true;
        case 7: /* << */
            if (!is_integer(a) || !is_integer(b)) return false;
            if (b < 0.0f || b >= 64.0f) return false;
            *res = (float)((unsigned long long)a << (int)b);
            return true;
        case 8: /* >> */
            if (!is_integer(a) || !is_integer(b)) return false;
            if (b < 0.0f || b >= 64.0f) return false;
            *res = (float)((unsigned long long)a >> (int)b);
            return true;
        default:
            return false;
    }
}

static bool eval_shape(const int *nums, int shape, int op1, int op2, int op3, float *result) {
    float a = (float)nums[0], b = (float)nums[1], c = (float)nums[2], d = (float)nums[3];
    float r1, r2;
    if (shape == 0) { /* ((a op1 b) op2 c) op3 d */
        if (!apply(a, b, op1, &r1)) return false;
        if (!apply(r1, c, op2, &r2)) return false;
        return apply(r2, d, op3, result);
    } else if (shape == 1) { /* (a op1 (b op2 c)) op3 d */
        if (!apply(b, c, op2, &r1)) return false;
        if (!apply(a, r1, op1, &r2)) return false;
        return apply(r2, d, op3, result);
    } else if (shape == 2) { /* (a op1 b) op2 (c op3 d) */
        if (!apply(a, b, op1, &r1)) return false;
        if (!apply(c, d, op3, &r2)) return false;
        return apply(r1, r2, op2, result);
    } else if (shape == 3) { /* a op1 ((b op2 c) op3 d) */
        if (!apply(b, c, op2, &r1)) return false;
        if (!apply(r1, d, op3, &r2)) return false;
        return apply(a, r2, op1, result);
    } else { /* shape == 4 : a op1 (b op2 (c op3 d)) */
        if (!apply(c, d, op3, &r1)) return false;
        if (!apply(b, r1, op2, &r2)) return false;
        return apply(a, r2, op1, result);
    }
}

static void format_expr(const int *nums, int shape, int op1, int op2, int op3, char *expr) {
    const int a = nums[0], b = nums[1], c = nums[2], d = nums[3];
    switch (shape) {
        case 0: /* ((a op1 b) op2 c) op3 d */
            snprintf(expr, EXPR_SIZE, "((%d%s%d)%s%d)%s%d", a, ops_str[op1], b, ops_str[op2], c, ops_str[op3], d);
            break;
        case 1: /* (a op1 (b op2 c)) op3 d */
            snprintf(expr, EXPR_SIZE, "(%d%s(%d%s%d))%s%d", a, ops_str[op1], b, ops_str[op2], c, ops_str[op3], d);
            break;
        case 2: /* (a op1 b) op2 (c op3 d) */
            snprintf(expr, EXPR_SIZE, "(%d%s%d)%s(%d%s%d)", a, ops_str[op1], b, ops_str[op2], c, ops_str[op3], d);
            break;
        case 3: /* a op1 ((b op2 c) op3 d) */
            snprintf(expr, EXPR_SIZE, "%d%s((%d%s%d)%s%d)", a, ops_str[op1], b, ops_str[op2], c, ops_str[op3], d);
            break;
        default: /* shape == 4 : a op1 (b op2 (c op3 d)) */
            snprintf(expr, EXPR_SIZE, "%d%s(%d%s(%d%s%d))", a, ops_str[op1], b, ops_str[op2], c, ops_str[op3], d);
            break;
    }
}

static int cmp_int(const void *p, const void *q) {
    int a = *(const int *)p;
    int b = *(const int *)q;
    return (a > b) - (a < b);
}

static bool next_permutation(int *a, int n) {
    int k = n - 2, l, i, j, t;
    while (k >= 0 && a[k] >= a[k + 1]) k--;
    if (k < 0) return false;
    l = n - 1;
    while (a[l] <= a[k]) l--;
    t = a[k];
    a[k] = a[l];
    a[l] = t;
    for (i = k + 1, j = n - 1; i < j; i++, j--) {
        t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
    return true;
}

int main(int argc, char **argv) {
    bool loop_mode = (argc > 1 && strcmp(argv[1], "--loop") == 0);
    int nums[NUM_COUNT];
    int i, op1, op2, op3, shape;
    do {
        for (i = 0; i < NUM_COUNT; ++i) {
            if (scanf("%d", &nums[i]) != 1) {
                if (!loop_mode) {
                    fprintf(stderr, "Failed to read %d integers.\n", NUM_COUNT);
                    return 1;
                }
                return 0;
            }
        }
        qsort(nums, NUM_COUNT, sizeof(int), cmp_int);
        bool found = false;
        do {
            for (op1 = 0; op1 < OPS_COUNT && !found; ++op1) {
                for (op2 = 0; op2 < OPS_COUNT && !found; ++op2) {
                    for (op3 = 0; op3 < OPS_COUNT && !found; ++op3) {
                        for (shape = 0; shape < 5 && !found; ++shape) {
                            float res;
                            if (eval_shape(nums, shape, op1, op2, op3, &res) && fabsf(res - 24.0f) < 1e-4f) {
                                char expr[EXPR_SIZE];
                                format_expr(nums, shape, op1, op2, op3, expr);
                                printf("/24 %s\n", expr);
                                found = true;
                            }
                        }
                    }
                }
            }
        } while (!found && next_permutation(nums, NUM_COUNT));
        if (!found) printf("No solution\n");
        if (!loop_mode) return 0;
    } while (loop_mode);
}