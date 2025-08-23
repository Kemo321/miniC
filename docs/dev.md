# Compile
-   cmake -DPRODUCTION=ON -DBUILD_TESTS=ON for prod
-   cmake -DPROCUCTION=OFF -DBUILD_TESTS=ON for dev

# Format code
-   clang-format -i -style=file $(find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "*.hpp" \))