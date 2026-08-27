# cmake/version.cmake
# Automatically generates sk265 version string based on Git topology and branch role

find_package(Git QUIET)

set(SK265_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.dev+0")

if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    # 1. Check if HEAD is exactly on a release tag (e.g. v0.1.0)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --exact-match --tags HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE SK265_EXACT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(SK265_EXACT_TAG MATCHES "^v?([0-9]+\\.[0-9]+\\.[0-9]+.*)")
        # Exactly on a release tag -> pure release version (e.g. 0.1.0)
        set(SK265_VERSION_STRING "${CMAKE_MATCH_1}")
    else()
        # 2. Get current branch name
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE SK265_BRANCH_NAME
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        # 3. Get latest reachable tag and distance
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE SK265_LATEST_TAG
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        if(SK265_LATEST_TAG)
            execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-list ${SK265_LATEST_TAG}..HEAD --count
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE SK265_TAG_DISTANCE
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
        else()
            execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE SK265_TAG_DISTANCE
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
        endif()

        if(NOT SK265_TAG_DISTANCE)
            set(SK265_TAG_DISTANCE "0")
        endif()

        # 4. Get short commit hash
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE SK265_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        if(SK265_COMMIT_HASH)
            set(SK265_GIT_HASH_STR "-g${SK265_COMMIT_HASH}")
        else()
            set(SK265_GIT_HASH_STR "")
        endif()

        # 5. Check if on a release/* branch vs master/development branch
        if(SK265_BRANCH_NAME MATCHES "^release/.*" OR SK265_BRANCH_NAME MATCHES "^release-.*")
            # Stable maintenance branch -> patch advancement (e.g. 0.1.0+2-g574e181)
            if(SK265_LATEST_TAG MATCHES "^v?([0-9]+\\.[0-9]+\\.[0-9]+)")
                set(SK265_VERSION_STRING "${CMAKE_MATCH_1}+${SK265_TAG_DISTANCE}${SK265_GIT_HASH_STR}")
            else()
                set(SK265_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.0+${SK265_TAG_DISTANCE}${SK265_GIT_HASH_STR}")
            endif()
        else()
            # Master / feature / dev branch -> target X.Y.dev (e.g. 0.1.dev+10-g574e181)
            set(SK265_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.dev+${SK265_TAG_DISTANCE}${SK265_GIT_HASH_STR}")
        endif()
    endif()
endif()

message(STATUS "sk265 configured version: ${SK265_VERSION_STRING}")

configure_file(
    ${CMAKE_SOURCE_DIR}/src/version.h.in
    ${CMAKE_BINARY_DIR}/generated/version.h
    @ONLY
)
