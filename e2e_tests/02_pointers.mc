// pointers: address-of, dereference, store through pointer
int main() {
    int x = 10;
    int *p = &x;
    print(*p);
    *p = 20;
    print(x);
    print(*p);

    int y = 1;
    int *q = &y;
    *q = *p + 5;
    print(y);
    return 0;
}
