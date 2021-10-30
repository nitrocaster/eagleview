set(EV_SRC_CBF
    CBF/Board.hpp
)
source_group(src/CBF FILES ${EV_SRC_CBF})

set(EV_SRC_EAGLE
    EagleBoard.cpp
    EagleBoard.hpp
)
source_group(src/Eagle FILES ${EV_SRC_EAGLE})

set(EV_SRC_MATH
    Angle.hpp
    Box2.hpp
    Edge2.hpp
    Math.hpp
    Matrix23.hpp
    Vector2.hpp
)
source_group(src/Math FILES ${EV_SRC_MATH})

set(EV_SRC_TEBO
    Fixed32.hpp
    StreamReader.hpp
    TeboBoard.cpp
    TeboBoard.hpp
)
source_group(src/Tebo FILES ${EV_SRC_TEBO})

set(EV_SRC_TOPTEST
    ToptestBoard.cpp
    ToptestBoard.hpp
)
source_group(src/TopTest FILES ${EV_SRC_TOPTEST})

set(EV_SRC_XML
    XMLBrowser.hpp
)
source_group(src/XML FILES ${EV_SRC_XML})

set(EV_SRC
    BoardFormat.cpp
    BoardFormat.hpp
    BoardFormatRegistrator.cpp
    BoardFormatRegistrator.hpp
    Common.hpp
    DynamicConvertible.hpp
    eagleview.cpp
    OutlineBuilder.hpp
)
source_group(src FILES ${EV_SRC})

set(EV_SOURCES
    ${EV_SRC_CBF}
    ${EV_SRC_EAGLE}
    ${EV_SRC_MATH}
    ${EV_SRC_TEBO}
    ${EV_SRC_TOPTEST}
    ${EV_SRC_XML}
    ${EV_SRC}
)

set(EV_LIBRARIES
    tinyxml2
)

if(CMAKE_COMPILER_IS_GNUCXX)
    list(APPEND EV_LIBRARIES m)
endif()

add_executable(eagleview ${EV_SOURCES})
target_link_libraries(eagleview ${EV_LIBRARIES})

set_target_properties(eagleview PROPERTIES CXX_VISIBILITY_PRESET hidden)

include(GNUInstallDirs)
set_target_properties(eagleview PROPERTIES INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")

install(TARGETS eagleview)
