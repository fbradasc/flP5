
PROJECT ( FLP5 )

SET ( EXECUTABLE_OUTPUT_PATH
      ${FLP5_BINARY_DIR}/bin
      CACHE PATH "install root"
)

SET ( CMAKE_BACKWARDS_COMPATIBILITY "1.2" )

    MESSAGE ("Using CMake ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
    MESSAGE ("*******************************************")
IF (WIN32)
    MESSAGE ("* Generating FLP5 win32 project files ... *")
ELSE (WIN32)
    MESSAGE ("* Generating FLP5 unix Makefiles..        *")
ENDIF (WIN32)
    MESSAGE ("*******************************************")

IF ( NOT WIN32 )
    INCLUDE ( ${CMAKE_ROOT}/Modules/CMakeBackwardCompatibilityC.cmake   )
    INCLUDE ( ${CMAKE_ROOT}/Modules/CMakeBackwardCompatibilityCXX.cmake )
    INCLUDE ( ${CMAKE_ROOT}/Modules/FindPNG.cmake                       )
ENDIF ( NOT WIN32 )
INCLUDE ( ${CMAKE_ROOT}/Modules/FindJPEG.cmake                      )
INCLUDE ( ${CMAKE_ROOT}/Modules/FindFLTK.cmake                      )
INCLUDE ( ${FLP5_SOURCE_DIR}/CMakeFlags.cfg                         )

IF ( CMAKE_CONFIGURE_INSTALL_PREFIX )
    SET( CMAKE_INSTALL_PREFIX ${CMAKE_CONFIGURE_INSTALL_PREFIX} )
ENDIF ( CMAKE_CONFIGURE_INSTALL_PREFIX )

MAKE_DIRECTORY( ${EXECUTABLE_OUTPUT_PATH} )

IF ( WIN32 )
    SET ( WIN32_EXE_TYPE
          "WIN32"
    )
    SET ( CMAKE_CXX_FLAGS
          "/GX /Zm200"
    )
    SET ( CMAKE_EXTRA_LINK_FLAGS
          "/NODEFAULTLIB:libcd"
          CACHE INTERNAL  "elflags"
    )
    IF ( ${USE_STATIC_FLTK} MATCHES "0" )
        LINK_LIBRARIES  ( ${FLTK_LIBRARY} fltkdll )
        ADD_DEFINITIONS ( /D "FL_DLL" )
    ELSE ( ${USE_STATIC_FLTK} MATCHES "0" )
        LINK_LIBRARIES  ( ${FLTK_LIBRARY} fltk )
    ENDIF ( ${USE_STATIC_FLTK} MATCHES "0" )
    LINK_DIRECTORIES( ${FLP5_SOURCE_DIR}/win32/DlPortIO )
    INCLUDE_DIRECTORIES( ${FLP5_SOURCE_DIR}/win32/DlPortIO )
    LINK_LIBRARIES( DlPortIO )
ELSE ( WIN32 )

    IF ( ${UX_REL} MATCHES "1" ) 
         SET ( CMAKE_CXX_FLAGS "-DOPTIMIZE -O2" )
    ELSE ( ${UX_REL} MATCHES "1" ) 
         SET ( CMAKE_CXX_FLAGS "-DDEBUG -g" )
    ENDIF ( ${UX_REL} MATCHES "1" )
    LINK_DIRECTORIES    ( /usr/X11R6/lib /usr/lib )
    INCLUDE_DIRECTORIES ( ${FLTK_INCLUDE_PATH} )
    IF (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)
        LINK_LIBRARIES ( ${FLTK_BASE_LIBRARY}   )
        LINK_LIBRARIES ( ${FLTK_IMAGES_LIBRARY} )
        LINK_LIBRARIES ( ${PNG_LIBRARIES}       )
        LINK_LIBRARIES ( ${JPEG_LIBRARIES}      )
    ELSE (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)
        LINK_LIBRARIES ( ${FLTK_LIBRARY} fltk fltk_images )
        LINK_LIBRARIES ( ${PNG_LIBRARIES}  )
        LINK_LIBRARIES ( ${JPEG_LIBRARIES} )
    ENDIF (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 1.4)
    ADD_DEFINITIONS     (
        -DENABLE_LINUX_PPDEV
        -DFLP5_ROOT_DIR='"${FLP5_SOURCE_DIR}"'
        -DFLP5_PREFIX='"${CMAKE_INSTALL_PREFIX}"'
    )
ENDIF ( WIN32 )

FIND_FILE (
     CHECK_REGISTRATION_CFG
     CMakeCheckRegistration.cfg
     ${FLP5_SOURCE_DIR}/utils
)
IF (CHECK_REGISTRATION_CFG)
    INCLUDE ( ${CHECK_REGISTRATION_CFG} )
ENDIF (CHECK_REGISTRATION_CFG)

IF ( CMAKE_HAS_X )
    LINK_LIBRARIES  ( dl Xt m )
    LINK_LIBRARIES  ( ${CMAKE_X_LIBS} )
    ADD_DEFINITIONS ( ${CMAKE_X_CFLAGS} )
ENDIF ( CMAKE_HAS_X )

SUBDIRS( src )

INSTALL_FILES ( /share/flP5/doc FILES
    LICENSE.txt
    README.txt
)
INSTALL_FILES ( /share/flP5/data FILES
    data/devices.prefs
    data/programmers.prefs
)
