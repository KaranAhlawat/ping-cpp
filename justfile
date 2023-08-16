project_root := "/home/karan/Secondary/dev/scratch_proj/ping-cpp"

ping: build
  ./build/ping-cpp

build:
  cmake --build build --target ping-cpp
  su -c "/sbin/setcap cap_net_raw+ep {{project_root}}/build/ping-cpp"
    
conf:
  cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/home/karan/repos/vcpkg/scripts/buildsystems/vcpkg.cmake