// recursive factorial via function calls
int fact(int n) {
    if (n < 2) {
        return 1;
    }
    return n * fact(n - 1);
}

int main() {
    print(fact(1));
    print(fact(5));
    print(fact(6));
    return 0;
}
