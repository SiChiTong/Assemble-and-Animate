CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

PROJECT (ASE)
ADD_DEFINITIONS("-std=c99")
INCLUDE_DIRECTORIES (./include)

ADD_SUBDIRECTORY (src lib)

ADD_SUBDIRECTORY (./src/targets/dummy ${ASE_SOURCE_DIR}/lib/dummy EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY (./src/targets/atron/atron-tos/tos-api ${ASE_SOURCE_DIR}/lib/atron-tos EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY (./src/targets/atron/atron-ussr/socket-api ${ASE_SOURCE_DIR}/lib/atron-ussr EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY (./src/targets/khepera/khepera-webots ${ASE_SOURCE_DIR}/lib/khepera-webots EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY (./src/targets/roombot/roombot-webots ${ASE_SOURCE_DIR}/lib/roombot-webots EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY (./src/targets/mtran/mtran-ussr/socket-api ${ASE_SOURCE_DIR}/lib/mtran-ussr EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY (./src/targets/dynamixel/dynamixel-d2u ${ASE_SOURCE_DIR}/lib/dynamixel-d2u EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY (./src/targets/dynamixel/dynamixel-cm510 ${ASE_SOURCE_DIR}/lib/dynamixel-cm510 EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY (./src/targets/odin/odin-ussr/socket-api ${ASE_SOURCE_DIR}/lib/odin-ussr EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY (./src/targets/fable ${ASE_SOURCE_DIR}/lib/fable EXCLUDE_FROM_ALL)
#ADD_SUBDIRECTORY (./src/targets/non-public/lmf ${ASE_SOURCE_DIR}/lib/lmf-firmware EXCLUDE_FROM_ALL)
#ADD_SUBDIRECTORY (./src/targets/non-public/lpf ${ASE_SOURCE_DIR}/lib/lpf EXCLUDE_FROM_ALL)
