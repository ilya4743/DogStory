# DogStory

```
pip install conan==1.59.0
```

```
mkdir -p build
cd build
conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```
