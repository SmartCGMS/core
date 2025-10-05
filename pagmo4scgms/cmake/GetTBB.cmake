include(FetchContent)

message(STATUS "Fetching TBB library...")
FetchContent_Declare(TBB
        GIT_REPOSITORY "https://github.com/oneapi-src/oneTBB.git"
        GIT_TAG "master"
        GIT_SHALLOW ON
)

FetchContent_MakeAvailable(TBB)
if (NOT TBB::tbb)
    message(FATAL_ERROR "Failed to add TBB through FetchContent")
endif()

