# Game Physics Template (WIP)

## IDE setup:
Add src and thirdparty to include directories. Otherwise the build will still work but not the autocomplete.

## Build
```
cmake . -B build
cmake --build build --config Release
```
## Run
- linux/macOS/MinGW
    ```
    ./build/Template
    ```
- MSVC
```
build\Release\Template.exe
```
