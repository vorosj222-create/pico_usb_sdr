# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13...3.27)

include(pico_sdk_import.cmake)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_TINYUSB_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb)
project(pico_usb_sdr C CXX ASM)
pico_sdk_init()

add_executable(${CMAKE_PROJECT_NAME}
  src/main.cpp
  src/usb_descriptors.c
  src/led.c
  src/ringbuffer.c
  src/buffer_queue.cpp
  src/dma_reader.cpp
  src/i2s_master_clock.cpp
  src/i2s_master_input.cpp
)

pico_generate_pio_header(${CMAKE_PROJECT_NAME}
  ${CMAKE_CURRENT_LIST_DIR}/src/i2s.pio
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(${CMAKE_PROJECT_NAME}
  pico_stdlib
  tinyusb_device
  tinyusb_board
  pico_stdio_usb
  hardware_pio
  hardware_dma
  hardware_clocks
)
pico_add_extra_outputs(${CMAKE_PROJECT_NAME})
