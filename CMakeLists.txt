cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(ProcessorCount)
ProcessorCount(N)
message("Number of processors: "  ${N})
if(NOT N EQUAL 0)
  set(CTEST_BUILD_FLAGS -j${N})
  set(ctest_test_args ${ctest_test_args} PARALLEL_LEVEL ${N})
  set(CMAKE_MAKE_PROGRAM "${CMAKE_MAKE_PROGRAM} -j ${N}")      
endif()
message("Cmake make program" ${CMAKE_MAKE_PROGRAM})

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(GameModelLib STATIC
	src/model/collision_detector.cpp
	src/model/collision_detector.h
	src/model/geom.h
	src/model/model.cpp
	src/model/loot_generator.cpp
	src/model/item_dog_provider.cpp
	src/model/tagged.h
)

target_link_libraries(GameModelLib PRIVATE 
	CONAN_PKG::boost
)

add_executable(game_server
	src/main.cpp
	src/ticker.cpp
	src/app/application.cpp
	src/app/game_session.cpp
	src/app/player.cpp
	src/app/use_cases.cpp
	src/game_data_store/model_serialization.h
	src/game_data_store/model_serialization.cpp
	src/json/boost_json.cpp
	src/json/json_deserializer.cpp
	src/json/json_serializer.cpp
	src/logger/logger.cpp
	src/web/api_handler.cpp
	src/web/file_handler.cpp
	src/web/http_server.cpp
	src/web/http_server.h
	src/web/logging_request_handler.h
	src/web/request_handler.cpp
	src/web/request_handler.h
	src/web/response.cpp
	src/database/database.cpp
	src/database/connection_pool.h
)

target_include_directories(game_server PRIVATE 
	src
	src/app
	src/game_data_store
	src/json
	src/logger
	src/model
	src/web
	src/database
)

target_link_libraries(game_server PRIVATE 
	Threads::Threads 
	CONAN_PKG::boost
	CONAN_PKG::libpqxx
	GameModelLib
)

SET(GCC_COVERAGE_LINK_FLAGS    "-lboost_url")

add_executable(loot_generator_tests
	tests/loot_generator_tests.cpp
	src/model/loot_generator.cpp
	src/model/loot_generator.h
)

target_link_libraries(loot_generator_tests PRIVATE 
	CONAN_PKG::catch2
)

target_include_directories(loot_generator_tests PRIVATE
	src/model
)

add_executable(collision_detector_tests	
	src/model/collision_detector.cpp
	src/model/collision_detector.h
	src/model/geom.h
	tests/collision-detector-tests.cpp
)

target_include_directories(collision_detector_tests PRIVATE
	src/model
)

target_link_libraries(collision_detector_tests PRIVATE 
	CONAN_PKG::catch2
)

add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW) 