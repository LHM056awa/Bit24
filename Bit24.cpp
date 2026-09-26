#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>

using namespace std;

const char *ops_str[] = {"+", "-", "*", "/", "&", "|", "^", "<<", ">>"};

#define EXPR_SIZE 55 /* worst case: 4*11 digits + 3*2 op chars + 4 parens + NUL */

// Check if a float value can be safely converted to unsigned long long (non-negative integer)
static bool is_integer(float x) {
    return fabsf(x - floorf(x)) < 1e-4f && x >= 0.0f && x <= (float)ULLONG_MAX;
}

static bool apply(float a, float b, int op, float &res) {
    switch (op) {
        case 0:
            res = a + b;
            return true; // +
        case 1:
            res = a - b;
            return true; // -
        case 2:
            res = a * b;
            return true; // *
        case 3:
            if (fabsf(b) < 1e-4f) return false; // /
            res = a / b;
            return true;
        case 4: // &
            if (!is_integer(a) || !is_integer(b)) return false;
            res = (float)((unsigned long long)a & (unsigned long long)b);
            return true;
        case 5: // |
            if (!is_integer(a) || !is_integer(b)) return false;
            res = (float)((unsigned long long)a | (unsigned long long)b);
            return true;
        case 6: // ^
            if (!is_integer(a) || !is_integer(b)) return false;
            res = (float)((unsigned long long)a ^ (unsigned long long)b);
            return true;
        case 7: // <<
            if (!is_integer(a) || !is_integer(b)) return false;
            if (b < 0.0f || b >= 64.0f) return false;
            res = (float)((unsigned long long)a << (int)b);
            return true;
        case 8: // >>
            if (!is_integer(a) || !is_integer(b)) return false;
            if (b < 0.0f || b >= 64.0f) return false;
            res = (float)((unsigned long long)a >> (int)b);
            return true;
        default:
            return false;
    }
}

static bool eval_shape(const int *nums, int shape, int op1, int op2, int op3, float &result) {
    float a = (float)nums[0], b = (float)nums[1], c = (float)nums[2], d = (float)nums[3];
    float r1, r2;
    if (shape == 0) { // ((a op1 b) op2 c) op3 d
        if (!apply(a, b, op1, r1)) return false;
        if (!apply(r1, c, op2, r2)) return false;
        return apply(r2, d, op3, result);
    } else if (shape == 1) { // (a op1 (b op2 c)) op3 d
        if (!apply(b, c, op2, r1)) return false;
        if (!apply(a, r1, op1, r2)) return false;
        return apply(r2, d, op3, result);
    } else if (shape == 2) { // (a op1 b) op2 (c op3 d)
        if (!apply(a, b, op1, r1)) return false;
        if (!apply(c, d, op3, r2)) return false;
        return apply(r1, r2, op2, result);
    } else if (shape == 3) { // a op1 ((b op2 c) op3 d)
        if (!apply(b, c, op2, r1)) return false;
        if (!apply(r1, d, op3, r2)) return false;
        return apply(a, r2, op1, result);
    } else { // shape == 4 : a op1 (b op2 (c op3 d))
        if (!apply(c, d, op3, r1)) return false;
        if (!apply(b, r1, op2, r2)) return false;
        return apply(a, r2, op1, result);
    }
}

static void format_expr(const int *nums, int shape, int op1, int op2, int op3, char *expr) {
    const int a = nums[0], b = nums[1], c = nums[2], d = nums[3];
    switch (shape) {
        case 0: // ((a op1 b) op2 c) op3 d
            snprintf(expr, EXPR_SIZE, "((%d%s%d)%s%d)%s%d", a, ops_str[op1], b, ops_str[op2], c, ops_str[op3], d);
            break;
        case 1: // (a op1 (b op2 c)) op3 d
            snprintf(expr, EXPR_SIZE, "(%d%s(%d%s%d))%s%d", a, ops_str[op1], b, ops_str[op2], c, ops_str[op3], d);
            break;
        case 2: // (a op1 b) op2 (c op3 d)
            snprintf(expr, EXPR_SIZE, "(%d%s%d)%s(%d%s%d)", a, ops_str[op1], b, ops_str[op2], c, ops_str[op3], d);
            break;
        case 3: // a op1 ((b op2 c) op3 d)
            snprintf(expr, EXPR_SIZE, "%d%s((%d%s%d)%s%d)", a, ops_str[op1], b, ops_str[op2], c, ops_str[op3], d);
            break;
        default: // shape == 4 : a op1 (b op2 (c op3 d))
            snprintf(expr, EXPR_SIZE, "%d%s(%d%s(%d%s%d))", a, ops_str[op1], b, ops_str[op2], c, ops_str[op3], d);
            break;
    }
}

int main(int argc, char **argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    const bool loop_mode = (argc > 1 && strcmp(argv[1], "--loop") == 0);
    int nums[4];
    do {
        bool ok = true;
        for (int i = 0; i < 4; ++i) {
            if (!(cin >> nums[i])) {
                ok = false;
                break;
            }
        }
        if (!ok && !loop_mode) {
            cerr << "Failed to read 4 integers." << endl;
            return 1;
        }
        if (!ok) return 0;
        sort(nums, nums + 4);
        bool found = false;
        do {
            for (int op1 = 0; op1 < 9 && !found; ++op1) {
                for (int op2 = 0; op2 < 9 && !found; ++op2) {
                    for (int op3 = 0; op3 < 9 && !found; ++op3) {
                        for (int shape = 0; shape < 5 && !found; ++shape) {
                            float res;
                            if (eval_shape(nums, shape, op1, op2, op3, res) && fabsf(res - 24.0f) < 1e-4f) {
                                char expr[EXPR_SIZE];
                                format_expr(nums, shape, op1, op2, op3, expr);
                                cout << "/24 " << expr << '\n';
                                found = true;
                            }
                        }
                    }
                }
            }
        } while (!found && next_permutation(nums, nums + 4));
        if (!found) cout << "No solution\n";
        if (!loop_mode) return 0;
    } while (loop_mode);
}