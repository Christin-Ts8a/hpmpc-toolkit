long factorial(int n) {
    long N = 1;
    for(int i = 1; i <= n; i++) {
        N *= i;
    }
    return N;
}

long C(int t, int n) {
    return factorial(n) / (factorial(t) * factorial(n - t));
}