include(GNUInstallDirs)

set(SRC configurator.cpp task.cpp general.cpp measurement.cpp worldbuilder.cpp)

set(HEAD disturbance.h configurator.h robot.h task.h general.h affordance.h settings.h measurement.h worldbuilder.h)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(OpenCV REQUIRED)

find_package(box2d REQUIRED)

add_library(CloCK ${SRC} ${HEAD})

set_target_properties(CloCK PROPERTIES POSITION_INDEPENDENT_CODE TRUE 
PUBLIC_HEADER "${HEAD}")

#if(box2d_FOUND)
	target_link_libraries(CloCK box2d ${OpenCV_LIBS} pthread)
	message("box2d installed")
#else()
#	set(BOX2D_BIN_DIR ${"../box2d/build/bin"})
#	target_link_libraries(HICS ${BOX2D_BIN_DIR}/libbox2d.a ${OpenCV_LIBS} pthread)
#	message("fetching static library from ${BOX2D_BIN_DIR}")
#endif()
install(TARGETS CloCK 
	EXPORT CloCK-targets 
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

message("library and archive destination " ${CMAKE_INSTALL_LIBDIR})

set(CMAKE_BUILD_TYPE Debug)

add_compile_options(-Wall -pedantic)
