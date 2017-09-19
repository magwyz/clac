cmake_minimum_required(VERSION 2.6)
project(Fingerprint)

set(SOURCES src/main.cpp
            src/encoder.cpp
            src/decoder.cpp
            src/parcorgenerator.cpp
            src/rangeCoder/rangeCoder.c
            src/rangeCoder/staticModel.c
            src/rangeCoder/quasiStaticModel.c
            src/wav/wavreader.cpp)

set(HEADERS include/main.hpp
            include/encoder.h
            include/decoder.h
            include/parcorgenerator.h
            include/rangeCoder/rangeCoder.h
            include/rangeCoder/staticModel.h
            include/rangeCoder/quasiStaticModel.h
            include/wav/wavreader.h
            include/wav/wavparameters.h)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(include include/rangeCoder include/wav)

add_executable(audioZip ${SOURCES} ${HEADERS})

target_link_libraries(audioZip ${LIBVLC_LIBRARIES} "-lpthread")

