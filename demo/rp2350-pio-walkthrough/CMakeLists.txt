# 自編練習。需要已安裝、支援 RP2350 的 Pico SDK 和相符交叉編譯器。
cmake_minimum_required(VERSION 3.13...3.27)

# 可以從 cmake -DPICO_SDK_PATH=... 或環境變數取得既有 SDK 路徑。
# 不在這裡下載 SDK，以免把考場環境設定藏在建置過程裡。
if(NOT PICO_SDK_PATH)
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
if(NOT EXISTS "${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
    message(FATAL_ERROR "Set PICO_SDK_PATH to your installed Pico SDK directory.")
endif()

# SDK 必須在 project() 前引入，才能選好對應板子的編譯工具鏈。
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
project(pico_pio_walkthrough C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(pico_pio_walkthrough main.c)
pico_generate_pio_header(pico_pio_walkthrough ${CMAKE_CURRENT_LIST_DIR}/echo.pio)
target_link_libraries(pico_pio_walkthrough pico_stdlib hardware_pio)
target_compile_options(pico_pio_walkthrough PRIVATE -Wall -Wextra)

# 用原本的 USB 線看 printf，不需要再接一個 UART 轉接器。
pico_enable_stdio_usb(pico_pio_walkthrough 1)
pico_enable_stdio_uart(pico_pio_walkthrough 0)
pico_add_extra_outputs(pico_pio_walkthrough)  # 產生 .uf2 等部署檔案。
