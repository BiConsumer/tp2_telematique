cmake_minimum_required(VERSION 3.12.0)
project(Simulateur LANGUAGES CXX)

set(PROJECT_NAME Simulateur)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(Headers
				"Computer/Computer.h"
				"Computer/Driver/Layer/DataType.h"
				"Computer/Driver/Layer/LinkLayer.h"
				"Computer/Driver/Layer/NetworkLayer.h"
				"Computer/Driver/Layer/LinkLayerLow.h"
				"Computer/Driver/NetworkDriver.h"
				"Computer/Hardware/NetworkInterfaceCard.h"
				"DataStructures/CircularQueue.h"
				"DataStructures/DataBuffer.h"
				"DataStructures/MACAddress.h"
				"DataStructures/Utils.h"
				"General/Configuration.h"
				"General/Logger.h"
				"General/Timer.h"
				"Transmission/Cable.h"
				"Transmission/Interferences.h"
				"Transmission/Transmission.h"
)
source_group("Fichiers header" FILES ${Headers})

set(Sources
				"Computer/Computer.cpp"
				"Computer/Driver/Layer/LinkLayer.cpp"
				"Computer/Driver/Layer/NetworkLayer.cpp"
				"Computer/Driver/Layer/LinkLayerLow.cpp"
				"Computer/Driver/NetworkDriver.cpp"
				"Computer/Hardware/NetworkInterfaceCard.cpp"
				"DataStructures/CircularQueue.cpp"
				"DataStructures/DataBuffer.cpp"
				"DataStructures/MACAddress.cpp"
				"General/Configuration.cpp"
				"General/Timer.cpp"
				"test.cpp"
				"Transmission/Cable.cpp"
				"Transmission/Interferences.cpp"
				"Transmission/Transmission.cpp"
)
source_group("Fichiers sources" FILES ${Sources})

set(ALL_FILES
				${Headers}
				${Sources}
)

add_executable(${PROJECT_NAME} ${ALL_FILES})

set(ComputerConfigs
	"Computer1.txt"
	"Computer2.txt"
	"Computer3.txt"
	# "Computer1Files.txt"
	# "Computer2Files.txt"
	# "Computer3Files.txt"
	"test.txt"
	"patrick.webp"
	"config_noise.txt"
	"config_no_noise.txt"
)

foreach(config ${ComputerConfigs})
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${config}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/${config}
            ${CMAKE_CURRENT_BINARY_DIR}/${config}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${config}
    )

    list(APPEND COPIED_CONFIGS
        ${CMAKE_CURRENT_BINARY_DIR}/${config}
    )
endforeach()

add_custom_target(copy_configs ALL
    DEPENDS ${COPIED_CONFIGS}
)

add_dependencies(${PROJECT_NAME} copy_configs)
