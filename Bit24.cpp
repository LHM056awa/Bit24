#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <cmath>
#include <climits>

using namespace std;

const char* ops_str[] = {"+", "-", "*", "/", "&", "|", "^", "<<", ">>"};

// Check if a float value can be safely converted to unsigned long long (non-negative integer)
bool is_integer(float x) {
    return fabs(x - floorf(x)) < 1e-4f && x >= 0.0f && x <= (float)ULLONG_MAX;
}

bool apply(float a, float b, int op, float& res) {
    switch (op) {
        case 0: res = a + b; return true;              // +
        case 1: res = a - b; return true;              // -
        case 2: res = a * b; return true;              // *
        case 3: if (fabs(b) < 1e-4f) return false;     // /
                res = a / b; return true;
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
        default: return false;
    }
}

// Build expression string and evaluate result according to the shape.
// The expressions are constructed without an outermost pair of redundant parentheses.
bool try_shape(const vector<int>& nums, int shape, int op1, int op2, int op3,
               string& expr, float& result) {
    float a = (float)nums[0], b = (float)nums[1], c = (float)nums[2], d = (float)nums[3];
    float r1, r2, r;
    string s1, s2, s3, s4;
    s1 = to_string(nums[0]);
    s2 = to_string(nums[1]);
    s3 = to_string(nums[2]);
    s4 = to_string(nums[3]);

    if (shape == 0) { // ((a op1 b) op2 c) op3 d
        if (!apply(a, b, op1, r1)) return false;
        if (!apply(r1, c, op2, r2)) return false;
        if (!apply(r2, d, op3, r)) return false;
        expr = "((" + s1 + ops_str[op1] + s2 + ")" + ops_str[op2] + s3 + ")" + ops_str[op3] + s4;
    } else if (shape == 1) { // (a op1 (b op2 c)) op3 d
        if (!apply(b, c, op2, r1)) return false;
        if (!apply(a, r1, op1, r2)) return false;
        if (!apply(r2, d, op3, r)) return false;
        expr = "(" + s1 + ops_str[op1] + "(" + s2 + ops_str[op2] + s3 + "))" + ops_str[op3] + s4;
    } else if (shape == 2) { // (a op1 b) op2 (c op3 d)
        if (!apply(a, b, op1, r1)) return false;
        if (!apply(c, d, op3, r2)) return false;
        if (!apply(r1, r2, op2, r)) return false;
        expr = "(" + s1 + ops_str[op1] + s2 + ")" + ops_str[op2] + "(" + s3 + ops_str[op3] + s4 + ")";
    } else if (shape == 3) { // a op1 ((b op2 c) op3 d)
        if (!apply(b, c, op2, r1)) return false;
        if (!apply(r1, d, op3, r2)) return false;
        if (!apply(a, r2, op1, r)) return false;
        expr = s1 + ops_str[op1] + "((" + s2 + ops_str[op2] + s3 + ")" + ops_str[op3] + s4 + ")";
    } else { // shape == 4 : a op1 (b op2 (c op3 d))
        if (!apply(c, d, op3, r1)) return false;
        if (!apply(b, r1, op2, r2)) return false;
        if (!apply(a, r2, op1, r)) return false;
        expr = s1 + ops_str[op1] + "(" + s2 + ops_str[op2] + "(" + s3 + ops_str[op3] + s4 + "))";
    }
    result = r;
    return true;
}

int main() {
    int a, b, c, d;
    cin >> a >> b >> c >> d;
    vector<int> nums = {a, b, c, d};
    sort(nums.begin(), nums.end());

    do {
        for (int op1 = 0; op1 < 9; ++op1) {
            for (int op2 = 0; op2 < 9; ++op2) {
                for (int op3 = 0; op3 < 9; ++op3) {
                    for (int shape = 0; shape < 5; ++shape) {
                        string expr;
                        float res;
                        if (try_shape(nums, shape, op1, op2, op3, expr, res) &&
                            fabs(res - 24.0f) < 1e-4f) {
                            cout << "/24 " << expr << endl;
                            return 0;
                        }
                    }
                }
            }
        }
    } while (next_permutation(nums.begin(), nums.end()));

    cout << "No solution" << endl;
}