include(FetchContent)

message(STATUS "Initalizing the GetPagmo script...")

FetchContent_Declare(
        pagmo
        GIT_REPOSITORY https://github.com/esa/pagmo2.git
        GIT_TAG v2.19.1
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(pagmo)

message(STATUS "Pagmo has been initialized in directory: ${pagmo_SOURCE_DIR}")