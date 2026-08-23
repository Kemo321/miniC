// nested while + pointers + break/continue + print
int main() {
    int sum = 0;
    int i = 0;
    while (i < 5) {
        i = i + 1;
        int *ps = &sum;
        *ps = *ps + i;
        if (i == 3) {
            continue;
        }
        print(*ps);
        if (sum > 12) {
            break;
        }
    }
    print(sum);
    return 0;
}
