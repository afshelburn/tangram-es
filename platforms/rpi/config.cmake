add_definitions(-DTANGRAM_RPI)

check_unsupported_compiler_version()

get_nextzen_api_key(NEXTZEN_API_KEY)
add_definitions(-DNEXTZEN_API_KEY="${NEXTZEN_API_KEY}")

# System font config
include(FindPkgConfig)
pkg_check_modules(FONTCONFIG REQUIRED "fontconfig")

add_executable(tangram
  platforms/rpi/src/context.cpp
  platforms/rpi/src/main.cpp
  platforms/rpi/src/rpiPlatform.cpp
  platforms/rpi/src/gps.cpp
  platforms/rpi/src/hud/hud.cpp
  platforms/rpi/src/hud/button.cpp
  platforms/rpi/src/hud/slideRot.cpp
  platforms/rpi/src/hud/slideZoom.cpp
  platforms/rpi/src/hud/rectangle.cpp
  platforms/rpi/src/hud/hudText.cpp
  platforms/common/urlClient.cpp
  platforms/common/linuxSystemFontHelper.cpp
  platforms/common/platform_gl.cpp
)

target_include_directories(tangram
  PRIVATE
  platforms/common
  platforms/rpi/src
  platforms/rpi/src/hud
  core/deps/isect2d/include
  core/deps/glm
  ${FONTCONFIG_INCLUDE_DIRS}
  /opt/vc/include/
  /opt/vc/include/interface/vcos/pthreads
  /opt/vc/include/interface/vmcs_host/linux
)

target_link_libraries(tangram
  PRIVATE
  tangram-core
  ${FONTCONFIG_LDFLAGS}
  curl
  pthread
  rt
  atomic
  pigpiod_if2
  ftgl
  /opt/vc/lib/libbcm_host.so
  /opt/vc/lib/libbrcmEGL.so
  /opt/vc/lib/libbrcmGLESv2.so
  /opt/vc/lib/libvchiq_arm.so
  /opt/vc/lib/libvcos.so
)

target_compile_options(tangram
  PRIVATE
  -std=c++17
  -Wall
  -fpermissive
)

add_resources(tangram "${PROJECT_SOURCE_DIR}/scenes" "res")
