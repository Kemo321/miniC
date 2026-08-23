// while + break + continue + print
int main() {
    int i = 0;
    while (i < 10) {
        i = i + 1;
        if (i == 3) {
            continue;
        }
        if (i == 8) {
            break;
        }
        print(i);
    }
    return 0;
}
