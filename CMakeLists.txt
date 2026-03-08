cmake_minimum_required(VERSION 3.10)
project(WarDolls)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(CURL REQUIRED)
find_package(Curses REQUIRED)

add_executable(WarDolls main.cpp Board.cpp Fidele.cpp GodPowerManager.cpp DeckBuilder.cpp)
target_link_libraries(WarDolls CURL::libcurl ${CURSES_LIBRARIES})
