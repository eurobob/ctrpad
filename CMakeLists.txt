cmake_minimum_required(VERSION 3.20)
project(CTR-Native C)
include(CheckCCompilerFlag)

set(CTR_NATIVE_VERSION "0.1.0-beta.7.1")
option(CTR_NATIVE_MACOS_BUNDLE "Build CTR Native as a macOS application bundle" OFF)

if(CTR_NATIVE_MACOS_BUNDLE AND NOT APPLE)
    message(FATAL_ERROR "CTR_NATIVE_MACOS_BUNDLE is supported only on Apple platforms.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4 AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR
        "CTR Native requires a target with 32-bit or 64-bit pointers; "
        "CMAKE_SIZEOF_VOID_P=${CMAKE_SIZEOF_VOID_P} is unsupported."
    )
endif()
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "CTR Native: enabling the checked 64-bit memory model")
endif()

set(CTR_NATIVE_MSVC_FRONTEND OFF)
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC" OR CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
    set(CTR_NATIVE_MSVC_FRONTEND ON)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" CTR_NATIVE_SYSTEM_PROCESSOR_LOWER)
set(CTR_NATIVE_X86_TARGET OFF)
if(CTR_NATIVE_SYSTEM_PROCESSOR_LOWER MATCHES "^(x86|i[3-6]86|amd64|x86_64)$"
   OR CMAKE_GENERATOR_PLATFORM MATCHES "^(Win32|x64)$")
    set(CTR_NATIVE_X86_TARGET ON)
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)

execute_process(
    COMMAND git rev-parse --short=12 HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE CTR_NATIVE_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT CTR_NATIVE_GIT_HASH)
    set(CTR_NATIVE_GIT_HASH "unknown")
endif()

execute_process(
    COMMAND git diff --quiet --exit-code
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE CTR_NATIVE_GIT_DIRTY
    ERROR_QUIET
)
if(CTR_NATIVE_GIT_DIRTY EQUAL 0)
    set(CTR_NATIVE_BUILD_ID "${CTR_NATIVE_GIT_HASH}")
else()
    set(CTR_NATIVE_BUILD_ID "${CTR_NATIVE_GIT_HASH}-dirty")
endif()

function(ctr_native_add_label_warning_if_supported target warning flag_check)
    set(saved_required_flags "${CMAKE_REQUIRED_FLAGS}")
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -Werror=unknown-warning-option")
    endif()

    check_c_compiler_flag("${warning}" "${flag_check}")
    set(CMAKE_REQUIRED_FLAGS "${saved_required_flags}")

    if(${flag_check})
        target_compile_options(${target} PRIVATE "${warning}")
    endif()
endfunction()

# SDL3
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_GPU OFF CACHE BOOL "" FORCE)
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
set(SDL_DIALOG OFF CACHE BOOL "" FORCE)
set(SDL_TRAY OFF CACHE BOOL "" FORCE)
set(SDL_POWER OFF CACHE BOOL "" FORCE)
set(SDL_VULKAN OFF CACHE BOOL "" FORCE)
set(SDL_OPENGLES OFF CACHE BOOL "" FORCE)
set(SDL_OFFSCREEN OFF CACHE BOOL "" FORCE)
set(SDL_SNDIO OFF CACHE BOOL "" FORCE)
set(SDL_SNDIO_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_X11_XSCRNSAVER OFF CACHE BOOL "" FORCE)
set(SDL_X11_XTEST OFF CACHE BOOL "" FORCE)
add_subdirectory(externals/SDL)

# CTR Native
set(CTR_NATIVE_EXECUTABLE_OPTIONS)
if(CTR_NATIVE_MACOS_BUNDLE)
    list(APPEND CTR_NATIVE_EXECUTABLE_OPTIONS MACOSX_BUNDLE)
endif()

add_executable(ctr_native ${CTR_NATIVE_EXECUTABLE_OPTIONS} main.c)
set_target_properties(ctr_native PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
if(CTR_NATIVE_MACOS_BUNDLE)
    find_program(CTR_NATIVE_CODESIGN_EXECUTABLE codesign REQUIRED)
    set_target_properties(ctr_native PROPERTIES
        OUTPUT_NAME "CTRPad"
        MACOSX_BUNDLE_BUNDLE_NAME "CTRPad"
        MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.chrissotraidis.ctrpad"
        MACOSX_BUNDLE_INFO_STRING "Touch-first native Crash Team Racing runtime"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${CTR_NATIVE_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0"
        MACOSX_BUNDLE_BUNDLE_VERSION "1"
        MACOSX_BUNDLE_COPYRIGHT "GPL-3.0; retail game data is user-supplied"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/platform/apple/Info.plist.in"
    )
    add_custom_command(TARGET ctr_native POST_BUILD
        COMMAND "${CTR_NATIVE_CODESIGN_EXECUTABLE}"
            --force
            --sign -
            --identifier "io.github.chrissotraidis.ctrpad"
            "$<TARGET_BUNDLE_DIR:ctr_native>"
        COMMENT "Ad-hoc signing CTRPad.app for local macOS execution"
        VERBATIM
    )
endif()
target_include_directories(ctr_native PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(ctr_native PRIVATE
    CTR_NATIVE
    BUILD=926
    CTR_INTERNAL
    CTR_NATIVE_VERSION=\"${CTR_NATIVE_VERSION}\"
    CTR_NATIVE_BUILD_ID=\"${CTR_NATIVE_BUILD_ID}\"
)
if(CTR_NATIVE_MSVC_FRONTEND)
    target_compile_options(ctr_native PRIVATE
        /W4 #TODO(aalhendi): fix warnings
        /bigobj
        /wd4152
        /wd4244
        /wd4245
        /wd4310
        /wd5287
    )
    if(CTR_NATIVE_X86_TARGET)
        target_compile_options(ctr_native PRIVATE /arch:SSE2)
    endif()
else()
    target_compile_options(ctr_native PRIVATE
        -Wenum-conversion
        -Wenum-compare
        -Wswitch-enum
        -Wformat
        -Wparentheses
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Release>:-g>
    )

    if(CTR_NATIVE_X86_TARGET)
        target_compile_options(ctr_native PRIVATE -msse)
    endif()

    check_c_compiler_flag("-fno-strict-aliasing" CTR_HAS_FNO_STRICT_ALIASING)
    if(CTR_HAS_FNO_STRICT_ALIASING)
        target_compile_options(ctr_native PRIVATE -fno-strict-aliasing)
    endif()

    check_c_compiler_flag("-fwrapv" CTR_HAS_FWRAPV)
    if(CTR_HAS_FWRAPV)
        target_compile_options(ctr_native PRIVATE -fwrapv)
    endif()

    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(ctr_native PRIVATE -Wall -Wextra)
        ctr_native_add_label_warning_if_supported(ctr_native -Wfree-labels CTR_HAS_WFREE_LABELS)
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
        ctr_native_add_label_warning_if_supported(ctr_native -Wc23-extensions CTR_HAS_WC23_EXTENSIONS)
    endif()
endif()
target_link_libraries(ctr_native SDL3::SDL3)
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_link_options(ctr_native PRIVATE -static-libgcc)
endif()
if(MINGW)
    target_link_options(ctr_native PRIVATE -static)
endif()

include(CTest)
if(BUILD_TESTING)
    add_test(NAME ctr_native_version COMMAND $<TARGET_FILE:ctr_native> --version)
    set_tests_properties(ctr_native_version PROPERTIES PASS_REGULAR_EXPRESSION "^CTR Native ")
    add_test(NAME ctr_native_state_digest COMMAND $<TARGET_FILE:ctr_native> --self-test-state-digest)
    set_tests_properties(ctr_native_state_digest PROPERTIES PASS_REGULAR_EXPRESSION
        "^\\[CTR StateDigest\\] self-test passed: .* mutation component=drivers"
    )
    add_test(NAME ctr_native_replay_gate COMMAND $<TARGET_FILE:ctr_native> --self-test-replay-gate)
    set_tests_properties(ctr_native_replay_gate PROPERTIES PASS_REGULAR_EXPRESSION
        "\\[CTR Replay\\] self-test passed: runtime-error=1 divergence=2 mutation-frame=17 component=drivers binary-identity=checked checkpoint-start=checked complete-vsync=checked"
    )
    add_test(NAME ctr_native_guest_ref COMMAND $<TARGET_FILE:ctr_native> --self-test-guest-ref)
    set_tests_properties(ctr_native_guest_ref PROPERTIES PASS_REGULAR_EXPRESSION
        "^\\[CTR GuestRef\\] self-test passed: reference=0x03000014 overflow=out-of-range stale=unregistered-region"
    )
    add_test(NAME ctr_native_asset_relocation COMMAND $<TARGET_FILE:ctr_native> --self-test-asset-relocation)
    set_tests_properties(ctr_native_asset_relocation PROPERTIES PASS_REGULAR_EXPRESSION
        "^\\[CTR AssetRelocation\\] self-test passed: first=0x07000014 duplicate=duplicate-patch target=target-out-of-range atomic=yes model=checked override=checked"
    )
    add_test(NAME ctr_native_input COMMAND $<TARGET_FILE:ctr_native> --self-test-input)
    set_tests_properties(ctr_native_input PROPERTIES PASS_REGULAR_EXPRESSION
        "^\\[CTR Input\\] self-test passed: metadata-key=4 legacy-enter=40 migration-enter=40 live-start=retail tap-latch=c\\+right one-snapshot"
    )
    add_test(NAME ctr_native_collision_scratch COMMAND $<TARGET_FILE:ctr_native> --self-test-collision-scratch)
    set_tests_properties(ctr_native_collision_scratch PROPERTIES PASS_REGULAR_EXPRESSION
        "^\\[CTR CollScratch\\] self-test passed: guest-size=0x20c extended=0x2f8 pointer-word=(raw32|zero64) owners=isolated"
    )
    add_test(NAME ctr_native_vehicle_constants COMMAND $<TARGET_FILE:ctr_native> --self-test-vehicle-constants)
    set_tests_properties(ctr_native_vehicle_constants PROPERTIES PASS_REGULAR_EXPRESSION
        "^\\[CTR VehBirth\\] self-test passed: constants=65 classes=4 retail-offsets=translated"
    )
    add_test(NAME ctr_native_vs_quip_offsets COMMAND $<TARGET_FILE:ctr_native> --self-test-vs-quip-offsets)
    set_tests_properties(ctr_native_vs_quip_offsets PROPERTIES PASS_REGULAR_EXPRESSION
        "^\\[CTR VsQuip\\] self-test passed: metas=51 readers=46 retail-ranges=translated"
    )
    add_test(NAME ctr_native_render_lists COMMAND $<TARGET_FILE:ctr_native> --self-test-render-lists)
    set_tests_properties(ctr_native_render_lists PROPERTIES PASS_REGULAR_EXPRESSION
        "^\\[CTR RenderLists\\] self-test passed: pointer-size=(4|8) slot-size=(8|16) full-dynamic=0x(28|50) named-heads=6 dispatch=canonical-index host-cursors=4 clip-ot=rebased"
    )
    add_test(NAME ctr_native_red_beaker_layout COMMAND $<TARGET_FILE:ctr_native> --self-test-red-beaker-layout)
    set_tests_properties(ctr_native_red_beaker_layout PROPERTIES PASS_REGULAR_EXPRESSION
        "^\\[CTR RedBeaker\\] self-test passed: pointer-size=(4|8) instance=0x(74|a0) idpp=0x(88|b0) players=4 retail-alias=named"
    )
    add_test(NAME ctr_native_audio_state_alignment COMMAND $<TARGET_FILE:ctr_native> --self-test-audio-state-alignment)
    set_tests_properties(ctr_native_audio_state_alignment PROPERTIES PASS_REGULAR_EXPRESSION
        "^\\[CTR AudioState\\] self-test passed: snapshot-align=(4|8) blob-mod8=4 boundary=memcpy"
    )
    add_test(NAME ctr_native_audio_mixer COMMAND $<TARGET_FILE:ctr_native> --self-test-audio-mixer)
    set_tests_properties(ctr_native_audio_mixer PROPERTIES PASS_REGULAR_EXPRESSION
        "^\\[CTR AudioMixer\\] self-test passed: adpcm=streamed pan=left reverb=tail dry=0x132e19d77167fb3d wet=0x4bdedc91d1293ad8 tail-frames=6905"
    )
    add_test(NAME ctr_native_checkpoint_pointer_validation COMMAND $<TARGET_FILE:ctr_native> --self-test-checkpoint-pointer-validation)
    set_tests_properties(ctr_native_checkpoint_pointer_validation PROPERTIES PASS_REGULAR_EXPRESSION
        "^\\[CTR State\\] pointer-validation self-test passed: scalar=ignored relocated=reversion-checked overlap-idempotence=checked pool-allocation=free-list-complement camera-quad=checked"
    )
    add_test(NAME ctr_native_vehicle_lap_checkpoint_bounds COMMAND $<TARGET_FILE:ctr_native> --self-test-vehicle-lap-checkpoint-bounds)
    set_tests_properties(ctr_native_vehicle_lap_checkpoint_bounds PROPERTIES PASS_REGULAR_EXPRESSION
        "^\\[CTR VehLap\\] self-test passed: count=4 first=valid last=valid one-past=rejected sentinel=0xff-rejected empty=rejected"
    )
    add_test(NAME ctr_native_potion_emitter_layout COMMAND $<TARGET_FILE:ctr_native> --self-test-potion-emitter-layout)
    set_tests_properties(ctr_native_potion_emitter_layout PROPERTIES PASS_REGULAR_EXPRESSION
        "^\\[CTR PotionEmitter\\] self-test passed: pointer-size=(4|8) entries=8 lifespan=20 random-velocity=400 typed-layout=yes"
    )
    add_test(NAME ctr_native_cutscene_particle_emitter_layout COMMAND $<TARGET_FILE:ctr_native> --self-test-cutscene-particle-emitter-layout)
    set_tests_properties(ctr_native_cutscene_particle_emitter_layout PROPERTIES PASS_REGULAR_EXPRESSION
        "^\\[CTR CutsceneEmitter\\] self-test passed: pointer-size=(4|8) groups=7 observed-group=46 lifespan=15 count=1 typed-function-init=yes"
    )
endif()
