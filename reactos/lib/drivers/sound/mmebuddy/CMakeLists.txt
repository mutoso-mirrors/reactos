
add_definitions(
    -DUNICODE -D_UNICODE
    -DNDEBUG=1)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/sound)

list(APPEND SOURCE
    capabilities.c
    devicelist.c
    deviceinstance.c
    functiontable.c
    mmewrap.c
    reentrancy.c
    utility.c
    kernel.c
    thread.c
    auxiliary/auxMessage.c
    midi/midMessage.c
    midi/modMessage.c
    mixer/mxdMessage.c
    wave/widMessage.c
    wave/wodMessage.c
    wave/format.c
    wave/header.c
    wave/streaming.c)

add_library(mmebuddy ${SOURCE})
add_pch(mmebuddy precomp.h)
add_dependencies(mmebuddy psdk)
