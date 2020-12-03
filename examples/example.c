#include <stdio.h>

int add(int n, int m) {	 
    return n + m;
}
int mult(int m, int n) {
    int res = 0, t = m + n, x = m * n;
    while(res != x) {
        res = res + m;
    }
    return res;
}
int fact(int n) {
    if (n == 0) return 1;
    else {
        return mult(n, fact(add(n, -1)));
    }
}
int main() {
    int n = 9, m = 8;
    printf("%d\n", mult(m, n));
    return 0;
}
