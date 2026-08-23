// nested calls + print
int add(int a, int b) {
    return a + b;
}

int mul(int a, int b) {
    return a * b;
}

int main() {
    print(add(2, 3));
    print(mul(4, 5));
    print(add(mul(2, 3), 4));
    print(mul(add(1, 2), add(3, 4)));
    return 0;
}
