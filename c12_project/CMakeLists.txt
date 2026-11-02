cmake_minimum_required(VERSION 3.10)
project(c12_control)

set(CMAKE_CXX_STANDARD 17)

# OpenCV'yi sistemden bul
find_package(OpenCV REQUIRED)

# Calistirilabilir hedefi tanimla
add_executable(c12_app 
    main.cpp 
    Gimbal.cpp 
    Camera.cpp
)

# OpenCV baslik dizinlerini ekle (Hatanin cozuldugu yer burasi)
target_include_directories(c12_app PRIVATE ${OpenCV_INCLUDE_DIRS})

# OpenCV kutuphanelerini bagla
target_link_libraries(c12_app PRIVATE ${OpenCV_LIBS})
