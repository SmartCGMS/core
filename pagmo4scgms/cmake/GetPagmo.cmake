include(FetchContent)

message("Fetching pagmo library...")
FetchContent_Declare(
        pagmo
        GIT_REPOSITORY https://github.com/esa/pagmo2.git
        GIT_TAG v2.19.1
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(pagmo)
