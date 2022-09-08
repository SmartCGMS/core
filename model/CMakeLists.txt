CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

SET(PROJ "model")

CONFIGURE_COMMON(${PROJ})

FILE(GLOB_RECURSE SRC_FILES "src/*.cpp" "src/*.c" "src/*.h")
SET(GEN_EXCLUDE_DIR "/win/")
FOREACH(TMP_PATH ${SRC_FILES})
    STRING(FIND ${TMP_PATH} ${GEN_EXCLUDE_DIR} EXCLUDE_DIR_FOUND)
    IF(NOT ${EXCLUDE_DIR_FOUND} EQUAL -1)
        LIST(REMOVE_ITEM SRC_FILES ${TMP_PATH})
    ENDIF()
ENDFOREACH(TMP_PATH)

IF(WIN32)
	FILE(GLOB SRC_WIN_FILES "src/win/*.cpp" "src/win/*.h" "src/win/*.def")
	SET(SRC_FILES "${SRC_FILES};${SRC_WIN_FILES}")
ENDIF()

# grouping
SET(SRC_GRPS "bergman" "bolus" "constant" "diffusion" "gct" "gct2" "ge" "neural_net" "pattern_prediction" "samadi" "sensitivity" "steil_rebrin" "uva_padova" "wma", "p559" "aim_ge", "enhacement", "cgp_pred", "bases")
FOREACH(MODEL_DIR ${SRC_GRPS})
	FILE(GLOB_RECURSE MDL_SOURCES "src/${MODEL_DIR}/*.cpp" "src/${MODEL_DIR}/*.h" "src/${MODEL_DIR}/*.c")
	SOURCE_GROUP("model\\${MODEL_DIR}" FILES ${MDL_SOURCES})
ENDFOREACH()

SCGMS_ADD_LIBRARY(${PROJ} SHARED ${SRC_FILES};${COMMON_FILES})
CONFIGURE_FILTER_OUTPUT(${PROJ})

TARGET_LINK_LIBRARIES(${PROJ} scgms)
