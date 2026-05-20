#################################################
#
#  (C) 2010-2012 Serghei Amelian
#  serghei (DOT) amelian (AT) gmail.com
#
#  (C) 2011-2012 Timothy Pearson
#  kb9vqf (AT) pearsoncomputing.net
#
#  (C) 2012-2020 Slávek Banko
#  slavek (DOT) banko (AT) axis.cz
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################


#################################################
#####
##### initialization...

if( NOT TDE_CMAKE_ROOT )
  if( "${CMAKE_VERSION}" VERSION_LESS "3.1" )
    message( FATAL_ERROR "CMake >= 3.1.0 required" )
  endif()

  if( ${CMAKE_CURRENT_LIST_DIR} STREQUAL ${CMAKE_ROOT}/Modules )

    # TDE CMake is installed in the system directory
    set( TDE_CMAKE_ROOT ${CMAKE_ROOT}
         CACHE FILEPATH "TDE CMake root" )
    set( TDE_CMAKE_MODULES ${TDE_CMAKE_ROOT}/Modules
         CACHE FILEPATH "TDE CMake modules" )
    set( TDE_CMAKE_TEMPLATES ${TDE_CMAKE_ROOT}/Templates
         CACHE FILEPATH "TDE CMake templates" )

  else()

    # TDE CMake is part of the source code
    get_filename_component( TDE_CMAKE_ROOT ${CMAKE_CURRENT_LIST_DIR} PATH )
    set( TDE_CMAKE_ROOT ${TDE_CMAKE_ROOT}
         CACHE FILEPATH "TDE CMake root" )
    set( TDE_CMAKE_MODULES ${TDE_CMAKE_ROOT}/modules
         CACHE FILEPATH "TDE CMake modules" )
    set( TDE_CMAKE_TEMPLATES ${TDE_CMAKE_ROOT}/templates
         CACHE FILEPATH "TDE CMake templates" )

  endif()


  option( FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." FALSE )
  if( ${FORCE_COLORED_OUTPUT} )
    if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
      if( NOT "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "4.9" )
        add_compile_options (-fdiagnostics-color=always)
      endif()
    elseif( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
      add_compile_options (-fcolor-diagnostics)
    endif()
  endif()

endif()


#################################################
#####
##### set necessary CMake policies

cmake_policy( PUSH )
if( POLICY CMP0057 )
  # necessary for CheckLinkerFlag
  cmake_policy( SET CMP0057 NEW )
endif()


#################################################
#####
##### necessary includes

include( CheckCXXCompilerFlag )
include( CheckCXXSourceCompiles )
include( CheckLinkerFlag OPTIONAL )
include( CheckSymbolExists )
include( CheckTypeSize )
include( TDEVersion )


#################################################
#####
##### tde_message_author_warning

macro( tde_message_author_warning )
	message( AUTHOR_WARNING
    "-------------------------------------------------\n"
    " ${ARGV}\n"
    "-------------------------------------------------" )
endmacro( tde_message_author_warning )


#################################################
#####
##### tde_message_fatal

macro( tde_message_fatal )
  message( FATAL_ERROR
    "#################################################\n"
    " ${ARGV}\n"
    "#################################################" )
endmacro( tde_message_fatal )


#################################################
#####
##### tde_get_arg( <ARG_NAME> <COUNT> <RETURN> <REST> <ARGS...> )
##### ARG_NAME(string): name of an argument to find in ARGS
##### COUNT(number): argument dimension, a number of items returned in RETURN
##### RETURN(list ref): items returned for argument as they found in ARGS
##### REST(list ref): rest of items except argument name and items returned in RETURN
##### ARGS(list): source list of arguments

macro( tde_get_arg ARG_NAME COUNT RETURN REST )
  unset( ${RETURN} )
  unset( ${REST} )
  list( APPEND ${REST} ${ARGN} )
  list( FIND ${REST} ${ARG_NAME} _arg_idx)
  if( NOT ${_arg_idx} EQUAL -1 )
    list( REMOVE_AT ${REST} ${_arg_idx} )
    set( _i 0 )
    while( ${_i} LESS ${COUNT} )
      list( GET ${REST} ${_arg_idx} _arg )
      list( REMOVE_AT ${REST} ${_arg_idx} )
      list( APPEND ${RETURN} ${_arg} )
      math( EXPR _i "${_i} + 1" )
    endwhile()
  endif()
endmacro( tde_get_arg )


################################################
#####
##### tde_execute_process( <ARGS...> [MESSAGE <MSG>] )
##### MSG: fatal error message (standard message will be written if not supplied)
##### ARGS: execute_process arguments

macro( tde_execute_process )
  tde_get_arg( MESSAGE 1 _message _rest_args ${ARGV} )
  tde_get_arg( RESULT_VARIABLE 1 _result_variable _tmp ${_rest_args} )
  tde_get_arg( COMMAND 1 _command _tmp ${_rest_args} )
  tde_get_arg( OUTPUT_VARIABLE 1 _output_variable _tmp ${_rest_args} )
  tde_get_arg( CACHE 3 _cache _rest_args2 ${_rest_args} )

  # handle optional FORCE parameter
  if( DEFINED _cache )
    list( GET _cache 2 _tmp )
    if( _tmp STREQUAL FORCE )
      set( _rest_args ${_rest_args2} )
    else()
      tde_get_arg( CACHE 2 _cache _rest_args ${_rest_args} )
    endif()
  endif()

  if( NOT DEFINED _result_variable )
    list( APPEND _rest_args RESULT_VARIABLE _exec_result )
    set( _result_variable _exec_result )
  endif()

  execute_process( ${_rest_args} )

  if( DEFINED _output_variable AND DEFINED _cache )
    set( ${_output_variable} ${${_output_variable}} CACHE ${_cache} )
  endif()

  if( ${_result_variable} )
    if( DEFINED _message )
      tde_message_fatal( ${_message} )
    else()
      if( ${${_result_variable}} MATCHES "^[0-9]+$" )
        set( ${_result_variable} "status ${${_result_variable}} returned!" )
      endif()
      tde_message_fatal( "Error executing '${_command}': ${${_result_variable}}" )
    endif()
  endif()
endmacro( tde_execute_process )


################################################
#####
##### tde_read_src_metadata

macro( tde_read_src_metadata )
  # look for SCM data if present
  if( EXISTS "${CMAKE_SOURCE_DIR}/.tdescminfo" )
    file( READ "${CMAKE_SOURCE_DIR}/.tdescminfo" TDE_SCM_INFO )
    string( REGEX MATCH "(^|\n)Name: ([^\n]*)" TDE_SCM_MODULE_NAME "${TDE_SCM_INFO}" )
    string( REGEX REPLACE "^[^:]*: " "" TDE_SCM_MODULE_NAME "${TDE_SCM_MODULE_NAME}" )
    string( REGEX MATCH "(^|\n)Revision: ([^\n]*)" TDE_SCM_MODULE_REVISION "${TDE_SCM_INFO}" )
    string( REGEX REPLACE "^[^:]*: " "" TDE_SCM_MODULE_REVISION "${TDE_SCM_MODULE_REVISION}" )
    string( REGEX MATCH "(^|\n)DateTime: ([^\n]*)" TDE_SCM_MODULE_DATETIME "${TDE_SCM_INFO}" )
    string( REGEX REPLACE "^[^:]*: " "" TDE_SCM_MODULE_DATETIME "${TDE_SCM_MODULE_DATETIME}" )
  else( )
    if( EXISTS "${CMAKE_SOURCE_DIR}/.tdescmmodule" )
      file( STRINGS "${CMAKE_SOURCE_DIR}/.tdescmmodule" TDE_SCM_MODULE_NAME )
    endif( EXISTS "${CMAKE_SOURCE_DIR}/.tdescmmodule" )
    if( EXISTS "${CMAKE_SOURCE_DIR}/.tdescmrevision" )
      file( STRINGS "${CMAKE_SOURCE_DIR}/.tdescmrevision" TDE_SCM_MODULE_REVISION )
    endif( EXISTS "${CMAKE_SOURCE_DIR}/.tdescmrevision" )
  endif( )

  # look for package data if present
  if( EXISTS "${CMAKE_SOURCE_DIR}/.tdepkginfo" )
    file( READ "${CMAKE_SOURCE_DIR}/.tdepkginfo" TDE_PKG_INFO )
  endif( )
  if( EXISTS "${CMAKE_BINARY_DIR}/.tdepkginfo" )
    file( READ "${CMAKE_BINARY_DIR}/.tdepkginfo" TDE_PKG_INFO )
  endif( )
  if( TDE_PKG_INFO )
    string( REGEX MATCH "(^|\n)Name: ([^\n]*)" TDE_PKG_NAME "${TDE_PKG_INFO}" )
    string( REGEX REPLACE "^[^:]*: " "" TDE_PKG_NAME "${TDE_PKG_NAME}" )
    string( REGEX MATCH "(^|\n)Version: ([^\n]*)" TDE_PKG_VERSION "${TDE_PKG_INFO}" )
    string( REGEX REPLACE "^[^:]*: " "" TDE_PKG_VERSION "${TDE_PKG_VERSION}" )
    string( REGEX MATCH "(^|\n)DateTime: ([^\n]*)" TDE_PKG_DATETIME "${TDE_PKG_INFO}" )
    string( REGEX REPLACE "^[^:]*: " "" TDE_PKG_DATETIME "${TDE_PKG_DATETIME}" )
  endif( )
endmacro( tde_read_src_metadata )


################################################
#####
##### finalization as a slave part

if( DEFINED MASTER_SOURCE_DIR )
  cmake_policy( POP )
  return( )
endif( )

########### slave part ends here ###############


################################################
#####
##### tde_install_icons( <icons...> THEME <svgicons> DESTINATION <destdir> )
##### default theme: hicolor
##### default destination: ${SHARE_INSTALL_DIR}/icons

macro( tde_install_icons )
  tde_get_arg( DESTINATION 1 _dest _args ${ARGV} )
  tde_get_arg( THEME 1 _req_theme _icons ${_args} )

  #defaults
  if( NOT _icons )
    set( _icons "*" )
  endif( NOT _icons )
  if( NOT _dest )
    set( _dest "${ICON_INSTALL_DIR}" )
  endif( NOT _dest )

  foreach( _icon ${_icons} )
    unset( _theme ) # clearing

    file(GLOB _icon_files *-${_icon}.png *-${_icon}.mng _icon_files *-${_icon}.svg* )
    foreach( _icon_file ${_icon_files} )
      # FIXME need a review
      string( REGEX MATCH "^.*/([a-zA-Z][a-zA-Z])([0-9a-zA-Z]+)\\-([a-z]+)\\-([^/]+)$" _dummy  "${_icon_file}" )
      set( _type  "${CMAKE_MATCH_1}" )
      set( _size  "${CMAKE_MATCH_2}" )
      set( _group "${CMAKE_MATCH_3}" )
      set( _name  "${CMAKE_MATCH_4}" )

      # we must ignore invalid icon names
      if( _type AND _size AND _group AND _name )

        # autodetect theme
        if( NOT _req_theme )
          unset( _theme )
          if( "${_type}" STREQUAL "cr" )
            set( _theme crystalsvg )
          elseif( "${_type}" STREQUAL "lo" )
            set( _theme locolor )
          endif( "${_type}" STREQUAL "cr" )
          # defaulting
          if( NOT _theme )
            set( _theme hicolor )
          endif( NOT _theme )
        else( NOT _req_theme )
          set( _theme ${_req_theme} )
        endif( NOT _req_theme )

        # fix "group" name
        if( "${_group}" STREQUAL "mime" )
          set( _group  "mimetypes" )
        endif( "${_group}" STREQUAL "mime" )
        if( "${_group}" STREQUAL "filesys" )
          set( _group  "places" )
        endif( "${_group}" STREQUAL "filesys" )
        if( "${_group}" STREQUAL "category" )
          set( _group  "categories" )
        endif( "${_group}" STREQUAL "category" )
        if( "${_group}" STREQUAL "device" )
          set( _group  "devices" )
        endif( "${_group}" STREQUAL "device" )
        if( "${_group}" STREQUAL "app" )
          set( _group  "apps" )
        endif( "${_group}" STREQUAL "app" )
        if( "${_group}" STREQUAL "action" )
          set( _group  "actions" )
        endif( "${_group}" STREQUAL "action" )

        if( "${_size}" STREQUAL "sc" )
          install( FILES ${_icon_file} DESTINATION ${_dest}/${_theme}/scalable/${_group}/ RENAME ${_name} )
        else( "${_size}" STREQUAL "sc" )
          install( FILES ${_icon_file} DESTINATION ${_dest}/${_theme}/${_size}x${_size}/${_group}/ RENAME ${_name} )
        endif( "${_size}" STREQUAL "sc" )

      endif( _type AND _size AND _group AND _name )

    endforeach( _icon_file )
  endforeach( _icon )

endmacro( tde_install_icons )


#################################################
#####
##### tde_add_lut( <source> <result> [depends] )
##### default depends: source

macro( tde_add_lut _src _lut _dep )
  set( create_hash_table ${CMAKE_SOURCE_DIR}/kjs/create_hash_table )
  if( NOT _dep )
    set( _dep ${_src} )
  endif( NOT _dep )
  add_custom_command( OUTPUT ${_lut}
    COMMAND perl ARGS ${create_hash_table} ${CMAKE_CURRENT_SOURCE_DIR}/${_src} -i > ${_lut}
    DEPENDS ${_src} )
  set_source_files_properties( ${_dep} PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_lut} )
  unset( _dep )
endmacro( tde_add_lut )


#################################################
#####
##### tde_add_luts( <sources...> )

macro( tde_add_luts )
  foreach( _src ${ARGV} )
    get_filename_component( _lut ${_src} NAME_WE )
    set( _lut "${_lut}.lut.h" )
    tde_add_lut( ${_src} ${_lut} ${_src} )
  endforeach( _src )
endmacro( tde_add_luts )


#################################################
#####
##### tde_file_to_cpp( <source> <destination> <variable> )

macro( tde_file_to_cpp _src _dst _var )
  if( IS_ABSOLUTE ${_dst} )
    set( dst ${_dst} )
  else( )
    set( dst "${CMAKE_CURRENT_BINARY_DIR}/${_dst}" )
  endif( )
  file( READ ${_src} text )
  string( REGEX REPLACE "\n" "\\\\n\"\n\"" text "${text}" )
  set( text "/* Generated by CMake */\n\nconst char *${_var} = \n\n\"${text}\";\n" )
  string( REGEX REPLACE "\n\"\";\n$" ";\n" text "${text}" )
  file( WRITE ${dst} "${text}" )
endmacro( )


#################################################
#####
##### tde_get_library_filename( <var> <target> )

function( tde_get_library_filename _filename _target )
  get_target_property( _type ${_target} TYPE )
  if( "${_type}" MATCHES "_LIBRARY" )
    get_target_property( _output_prefix ${_target} PREFIX )
    if( "${_output_prefix}" STREQUAL "_output_prefix-NOTFOUND" )
      if( "${_type}" MATCHES "STATIC_" )
        set( _output_prefix "${CMAKE_STATIC_LIBRARY_PREFIX}" )
      elseif( "${_type}" MATCHES "SHARED_" )
        set( _output_prefix "${CMAKE_SHARED_LIBRARY_PREFIX}" )
      elseif( "${_type}" MATCHES "MODULE_" )
        set( _output_prefix "${CMAKE_SHARED_MODULE_PREFIX}" )
      else( )
        set( _output_prefix "" )
      endif( )
    endif( )
    get_target_property( _output_suffix ${_target} SUFFIX )
    if( "${_output_suffix}" STREQUAL "_output_suffix-NOTFOUND" )
      if( "${_type}" MATCHES "STATIC_" )
        set( _output_suffix "${CMAKE_STATIC_LIBRARY_SUFFIX}" )
      elseif( "${_type}" MATCHES "SHARED_" )
        set( _output_suffix "${CMAKE_SHARED_LIBRARY_SUFFIX}" )
      elseif( "${_type}" MATCHES "MODULE_" )
        set( _output_suffix "${CMAKE_SHARED_MODULE_SUFFIX}" )
      else( )
        set( _output_suffix "" )
      endif( )
    endif( )
    get_target_property( _output ${_target} OUTPUT_NAME )
    set( ${_filename} "${_output_prefix}${_output}${_output_suffix}" PARENT_SCOPE )
  else( )
    set( ${_filename} "" PARENT_SCOPE )
  endif( )
endfunction( )


#################################################
#####
##### tde_install_la_file( <target> <destination> )

macro( tde_install_la_file _target _destination )

  tde_get_library_filename( _soname ${_target} )
  get_target_property( _target_release ${_target} RELEASE )
  if( _target_release )
    string( REPLACE "-${_target_release}" "" _soname_base "${_soname}" )
  else( )
    set( _soname_base ${_soname} )
  endif( )
  string( REGEX REPLACE "\\.so(\\.[0-9]+)*$" "" _laname "${_soname_base}" )
  set( _laname ${CMAKE_CURRENT_BINARY_DIR}/${_laname}.la )

  file( WRITE ${_laname}
"# ${_laname} - a libtool library file, generated by cmake
# The name that we can dlopen(3).
dlname='${_soname}'
# Names of this library
library_names='${_soname} ${_soname} ${_soname_base}'
# The name of the static archive
old_library=''
# Libraries that this one depends upon.
dependency_libs=''
# Version information.\ncurrent=0\nage=0\nrevision=0
# Is this an already installed library?\ninstalled=yes
# Should we warn about portability when linking against -modules?\nshouldnotlink=yes
# Files to dlopen/dlpreopen\ndlopen=''\ndlpreopen=''
# Directory that this library needs to be installed in:
libdir='${_destination}'
" )

  install( FILES ${_laname} DESTINATION ${_destination} )

endmacro( tde_install_la_file )


#################################################
#####
##### tde_add_ui_files

macro( tde_add_ui_files _sources )
  if( TDE_FOUND OR "${CMAKE_PROJECT_NAME}" STREQUAL "tdelibs" )
    set( HAVE_TDE 1 )
  endif()

  foreach( _ui_file ${ARGN} )

    get_filename_component( _ui_basename ${_ui_file} NAME_WE )
    get_filename_component( _ui_absolute_path ${_ui_file} ABSOLUTE )

    list( APPEND ${_sources} ${_ui_basename}.cpp )

    add_custom_command( OUTPUT ${_ui_basename}.h ${_ui_basename}.cpp
      COMMAND ${CMAKE_COMMAND}
        -DUIC_EXECUTABLE:FILEPATH=${UIC_EXECUTABLE}
        -DTQT_REPLACE_SCRIPT:FILEPATH=${TQT_REPLACE_SCRIPT}
        -DTDE_TQTPLUGINS_DIR:FILEPATH=${TDE_TQTPLUGINS_DIR}
        -DMOC_EXECUTABLE:FILEPATH=${MOC_EXECUTABLE}
        -DUI_FILE:FILEPATH=${_ui_absolute_path}
        -DMASTER_SOURCE_DIR:FILEPATH=${CMAKE_SOURCE_DIR}
        -DMASTER_BINARY_DIR:FILEPATH=${CMAKE_BINARY_DIR}
        -DTDE_FOUND=${HAVE_TDE}
        -DTQT_ONLY=${TQT_ONLY}
        -P ${TDE_CMAKE_MODULES}/tde_uic.cmake
      DEPENDS ${_ui_absolute_path} )

  endforeach( _ui_file )
endmacro( tde_add_ui_files )


#################################################
#####
##### tde_moc

macro( tde_moc _sources )
  foreach( _input_file ${ARGN} )

    get_filename_component( _input_file "${_input_file}" ABSOLUTE )
    get_filename_component( _basename ${_input_file} NAME_WE )
    set( _output_file "${_basename}.moc.cpp" )
    add_custom_command( OUTPUT ${_output_file}
      COMMAND
        ${TMOC_EXECUTABLE} ${_input_file} -o ${_output_file}
      DEPENDS
        ${_input_file} )
    list( APPEND ${_sources} ${_output_file} )

  endforeach( )
endmacro( )


#################################################
#####
##### tde_automoc

macro( tde_automoc )
  foreach( _src_file ${ARGN} )

    get_filename_component( _src_file "${_src_file}" ABSOLUTE )

    if( EXISTS "${_src_file}" )

      # read source file and check if have moc include
      file( READ "${_src_file}" _src_content )
      string( REGEX MATCHALL "#include +[^ ]+\\.moc[\">]" _moc_includes "${_src_content}" )

      # found included moc(s)?
      if( _moc_includes )
        foreach( _moc_file ${_moc_includes} )

          # extracting moc filename
          string( REGEX MATCH "[^ <\"]+\\.moc" _moc_file "${_moc_file}" )
          set( _moc_file "${CMAKE_CURRENT_BINARY_DIR}/${_moc_file}" )

          # create header filename
          get_filename_component( _src_path "${_src_file}" ABSOLUTE )
          get_filename_component( _src_path "${_src_path}" PATH )
          get_filename_component( _src_header "${_moc_file}" NAME_WE )
          set( _header_file "${_src_path}/${_src_header}.h" )

          # if header doesn't exists, check in META_INCLUDES
          if( NOT EXISTS "${_header_file}" )
            unset( _found )
            foreach( _src_path ${_meta_includes} )
              set( _header_file "${_src_path}/${_src_header}.h" )
              if( EXISTS "${_header_file}" )
                set( _found 1 )
                break( )
              endif( )
            endforeach( )
            if( NOT _found )
              get_filename_component( _moc_file "${_moc_file}" NAME )
              tde_message_fatal( "AUTOMOC error: '${_moc_file}' cannot be generated.\n Reason: '${_src_header}.h' not found." )
            endif( )
          endif( )

          # moc-ing header
          file( RELATIVE_PATH _moc_file_relative "${CMAKE_BINARY_DIR}" "${_moc_file}" )
          add_custom_command( OUTPUT ${_moc_file}
            COMMAND ${TMOC_EXECUTABLE} ${_header_file} -o ${_moc_file}
            COMMENT "Generating ${_moc_file_relative}"
            DEPENDS ${_header_file} )

          # create dependency between source file and moc file
          set_property( SOURCE ${_src_file} APPEND PROPERTY OBJECT_DEPENDS ${_moc_file} )

        endforeach( _moc_file )

      endif( _moc_includes )

    else( EXISTS "${_src_file}" )

      # for generated file, the path must match the binary directory
      string( LENGTH "${CMAKE_BINARY_DIR}" CMAKE_BINARY_DIR_LEN )
      string( LENGTH "${_src_file}" _basename_len )
      if( ${_basename_len} LESS ${CMAKE_BINARY_DIR_LEN} )
        set( _basedir_prefix "${CMAKE_SOURCE_DIR}" )
      else( )
        string( SUBSTRING "${_src_file}" 0 ${CMAKE_BINARY_DIR_LEN} _basedir_prefix )
      endif( )
      if( ${_basedir_prefix} STREQUAL "${CMAKE_BINARY_DIR}" )
        file( RELATIVE_PATH _sourcename "${CMAKE_BINARY_DIR}" "${_src_file}" )
        file( RELATIVE_PATH _basename "${CMAKE_CURRENT_BINARY_DIR}" "${_src_file}" )
      else( )
        file( RELATIVE_PATH _sourcename "${CMAKE_SOURCE_DIR}" "${_src_file}" )
        file( RELATIVE_PATH _basename "${CMAKE_CURRENT_SOURCE_DIR}" "${_src_file}" )
      endif( )

      # automocing generated file
      get_filename_component( _automoc_file "${_src_file}+automoc" NAME )
      set( _automoc_file "${CMAKE_CURRENT_BINARY_DIR}/${_automoc_file}" )

      add_custom_command( OUTPUT ${_automoc_file}
        COMMAND ${CMAKE_COMMAND}
          -DTMOC_EXECUTABLE:FILEPATH=${TMOC_EXECUTABLE}
          -DSRC_FILE:FILEPATH=${CMAKE_CURRENT_BINARY_DIR}/${_basename}
          -DMETA_INCLUDES:STRING="${_meta_includes}"
          -DMASTER_SOURCE_DIR:FILEPATH=${CMAKE_SOURCE_DIR}
          -DMASTER_BINARY_DIR:FILEPATH=${CMAKE_BINARY_DIR}
          -P ${TDE_CMAKE_MODULES}/tde_automoc.cmake
        COMMENT "Automocing ${_sourcename}"
        DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${_basename}"
      )

      # create dependency between source file and moc file
      set_property( SOURCE "${CMAKE_CURRENT_BINARY_DIR}/${_basename}"
                    APPEND PROPERTY OBJECT_DEPENDS ${_automoc_file} )

    endif( EXISTS "${_src_file}" )

  endforeach( _src_file )
endmacro( tde_automoc )


#################################################
#####
##### tde_create_dcop_kidl

macro( tde_create_dcop_kidl _kidl _kidl_source )

  get_filename_component( _kidl_source ${_kidl_source} ABSOLUTE )
  get_filename_component( _kidl_basename ${_kidl_source} NAME_WE )
  set( _kidl_output ${CMAKE_CURRENT_BINARY_DIR}/${_kidl_basename}.kidl )
  file( RELATIVE_PATH _kidl_target "${CMAKE_BINARY_DIR}" "${_kidl_output}" )
  string( REPLACE "/" "+" _kidl_target "${_kidl_target}" )

  if( NOT TARGET ${_kidl_target} )
    add_custom_command(
      OUTPUT ${_kidl_output}
      COMMAND ${KDE3_DCOPIDLNG_EXECUTABLE}
      ARGS ${_kidl_source} > ${_kidl_output}
      DEPENDS ${_kidl_source}
    )
    add_custom_target( ${_kidl_target} DEPENDS ${_kidl_output} )
  endif( )

  set( ${_kidl} ${_kidl_output} )

endmacro( tde_create_dcop_kidl )


#################################################
#####
##### tde_add_dcop_skels

macro( tde_add_dcop_skels _sources )
  foreach( _current_FILE ${ARGN} )

    get_filename_component( _tmp_FILE ${_current_FILE} ABSOLUTE )
    get_filename_component( _basename ${_tmp_FILE} NAME_WE )

    set( _skel ${CMAKE_CURRENT_BINARY_DIR}/${_basename}_skel.cpp )
    file( RELATIVE_PATH _skel_target "${CMAKE_BINARY_DIR}" "${_skel}" )
    string( REPLACE "/" "+" _skel_target "${_skel_target}" )

    tde_create_dcop_kidl( _kidl ${_tmp_FILE} )

    if( NOT TARGET ${_skel_target} )
      add_custom_command(
        OUTPUT ${_skel}
        COMMAND ${KDE3_DCOPIDL2CPP_EXECUTABLE}
        ARGS --c++-suffix cpp --no-signals --no-stub ${_kidl}
        DEPENDS ${_kidl_target}
      )
      add_custom_target( ${_skel_target} DEPENDS ${_skel} )
    endif( )

    list( APPEND ${_sources} ${_skel} )

  endforeach( _current_FILE )
endmacro( tde_add_dcop_skels )


#################################################
#####
##### tde_add_dcop_stubs

macro( tde_add_dcop_stubs _sources )
  foreach( _current_FILE ${ARGN} )

    get_filename_component( _tmp_FILE ${_current_FILE} ABSOLUTE )
    get_filename_component( _basename ${_tmp_FILE} NAME_WE )

    set( _stub_CPP ${CMAKE_CURRENT_BINARY_DIR}/${_basename}_stub.cpp )
    set( _stub_HEADER ${CMAKE_CURRENT_BINARY_DIR}/${_basename}_stub.h )
    file( RELATIVE_PATH _stub_target "${CMAKE_BINARY_DIR}" "${_stub_CPP}" )
    string( REPLACE "/" "+" _stub_target "${_stub_target}" )

    tde_create_dcop_kidl( _kidl ${_tmp_FILE} )

    if( NOT TARGET ${_stub_target} )
      add_custom_command(
        OUTPUT ${_stub_CPP} ${_stub_HEADER}
        COMMAND ${KDE3_DCOPIDL2CPP_EXECUTABLE}
        ARGS --c++-suffix cpp --no-signals --no-skel ${_kidl}
        DEPENDS ${_kidl_target}
      )
      add_custom_target( ${_stub_target} DEPENDS ${_stub_CPP} ${_stub_HEADER} )
    endif( )

    list( APPEND ${_sources} ${_stub_CPP} )

  endforeach( _current_FILE )
endmacro( tde_add_dcop_stubs )


#################################################
#####
##### tde_add_kcfg_files

macro( tde_add_kcfg_files _sources )
  foreach( _current_FILE ${ARGN} )

    get_filename_component( _tmp_FILE ${_current_FILE} ABSOLUTE )
    get_filename_component( _basename ${_tmp_FILE} NAME_WE )

    file( READ ${_tmp_FILE} _contents )
    string( REGEX REPLACE "^(.*\n)?File=([^\n]+)\n.*$" "\\2"  _kcfg_FILE "${_contents}" )

    set( _src_FILE    ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.cpp )
    set( _header_FILE ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.h )
    file( RELATIVE_PATH _kcfg_target "${CMAKE_BINARY_DIR}" "${_src_FILE}" )
    string( REPLACE "/" "+" _kcfg_target "${_kcfg_target}" )

    if( NOT TARGET ${_kcfg_target} )
      add_custom_command(
        OUTPUT ${_src_FILE} ${_header_FILE}
        COMMAND ${KDE3_KCFGC_EXECUTABLE}
        ARGS ${CMAKE_CURRENT_SOURCE_DIR}/${_kcfg_FILE} ${_tmp_FILE}
        DEPENDS ${_tmp_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/${_kcfg_FILE}
      )
      add_custom_target( ${_kcfg_target} DEPENDS ${_src_FILE} ${_header_FILE} )
    endif( )

    list( APPEND ${_sources} ${_src_FILE} )

  endforeach( _current_FILE )
endmacro( tde_add_kcfg_files )


#################################################
#####
##### __tde_internal_process_sources

macro( __tde_internal_process_sources _sources )

  unset( ${_sources} )

  foreach( _arg ${ARGN} )
    get_filename_component( _ext ${_arg} EXT )
    get_filename_component( _name ${_arg} NAME_WE )
    get_filename_component( _path ${_arg} PATH )

    # if not path, set it to "."
    if( NOT _path )
      set( _path "./" )
    endif( NOT _path )

    # handle .ui files
    if( ${_ext} STREQUAL ".ui" )
      tde_add_ui_files( ${_sources} ${_arg} )

    # handle .skel files
    elseif( ${_ext} STREQUAL ".skel" )
      tde_add_dcop_skels( ${_sources} ${_path}/${_name}.h )

    # handle .stub files
    elseif( ${_ext} STREQUAL ".stub" )
      tde_add_dcop_stubs( ${_sources} ${_path}/${_name}.h )

    # handle .kcfgc files
    elseif( ${_ext} STREQUAL ".kcfgc" )
      tde_add_kcfg_files( ${_sources} ${_arg} )

    # handle any other files
    else( ${_ext} STREQUAL ".ui" )
      list(APPEND ${_sources} ${_arg} )
    endif( ${_ext} STREQUAL ".ui" )
  endforeach( _arg )

endmacro( __tde_internal_process_sources )


#################################################
#####
##### tde_install_libtool_file

macro( tde_install_libtool_file _target _destination )

  # get .so name
  tde_get_library_filename( _soname ${_target} )
  get_target_property( _target_release ${_target} RELEASE )
  if( _target_release )
    string( REPLACE "-${_target_release}" "" _soname_base "${_soname}" )
  else( )
    set( _soname_base ${_soname} )
  endif( )

  # get .la name
  string( REGEX REPLACE "\\.so(\\.[0-9]+)*$" "" _laname "${_soname_base}" )
  set( _laname ${_laname}.la )

  # get version of target
  get_target_property( _target_version ${_target} VERSION )
  get_target_property( _target_soversion ${_target} SOVERSION )

  # we have so version
  if( _target_version )
    set( _library_name_1 "${_soname}.${_target_version}" )
    set( _library_name_2 "${_soname}.${_target_soversion}" )
    set( _library_name_3 "${_soname_base}" )

    string( REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$" _dummy  "${_target_version}" )
    set( _version_current  "${CMAKE_MATCH_1}" )
    set( _version_age  "${CMAKE_MATCH_2}" )
    set( _version_revision "${CMAKE_MATCH_3}" )

  # we have no so version (module?)
  else( _target_version )
    set( _library_name_1 "${_soname}" )
    set( _library_name_2 "${_soname}" )
    set( _library_name_3 "${_soname_base}" )
    set( _version_current  "0" )
    set( _version_age  "0" )
    set( _version_revision "0" )
  endif( _target_version )

  if( IS_ABSOLUTE ${_destination} )
    set( _libdir "${_destination}" )
  else( IS_ABSOLUTE ${_destination} )
    set( _libdir "${CMAKE_INSTALL_PREFIX}/${_destination}" )
  endif( IS_ABSOLUTE ${_destination} )

  configure_file( ${TDE_CMAKE_TEMPLATES}/tde_libtool_file.cmake "${_laname}" @ONLY )

  install( FILES "${CMAKE_CURRENT_BINARY_DIR}/${_laname}" DESTINATION ${_destination} )

endmacro( tde_install_libtool_file )


#################################################
#####
##### tde_install_export / tde_import

function( tde_install_export )
  file( GLOB export_files ${CMAKE_CURRENT_BINARY_DIR}/export-*.cmake )
  list( SORT export_files )

  set( mode "WRITE" )
  foreach( filename ${export_files} )
    file( READ ${filename} content )
    file( ${mode} "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.cmake" "${content}" )
    set( mode "APPEND" )
  endforeach( )

  install( FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.cmake" DESTINATION ${CMAKE_INSTALL_DIR} )
endfunction( )


macro( tde_import _library )
  message( STATUS "checking for '${_library}'" )
  string( TOUPPER "BUILD_${_library}" _build )
  if( ${_build} )
    message( STATUS "  ok, activated for build" )
  else()
    if( EXISTS "${TDE_CMAKE_DIR}/${_library}.cmake" )
      include( "${TDE_CMAKE_DIR}/${_library}.cmake" )
    elseif( EXISTS "${TQT_CMAKE_DIR}/${_library}.cmake" )
      include( "${TQT_CMAKE_DIR}/${_library}.cmake" )
    else()
      tde_message_fatal( "'${_library}' is required, but is not installed nor selected for build" )
    endif()
    message( STATUS "  ok, found import file" )
  endif()
endmacro()


#################################################
#####
##### tde_add_library

macro( tde_add_library _arg_target )

  unset( _target )
  unset( _type )
  unset( _static_pic )
  unset( _automoc )
  unset( _meta_includes )
  unset( _no_libtool_file )
  unset( _no_export )
  unset( _version )
  unset( _release )
  unset( _sources )
  unset( _cxx_features )
  unset( _cxx_features_private )
  unset( _destination )
  unset( _embed )
  unset( _link )
  unset( _link_private )
  unset( _dependencies )
  unset( _storage )
  unset( _exclude_from_all )

  set( _shouldnotlink no )

  # metadata
  unset( _description )
  unset( _license )
  unset( _copyright )
  unset( _authors )
  unset( _product )
  unset( _organization )
  unset( _version )
  unset( _datetime )
  unset( _notes )

  # default metadata
  set( _product "Trinity Desktop Environment" )
  tde_curdatetime( _datetime )

  foreach( _arg ${ARGV} )

    # this variable help us to skip
    # storing unapropriate values (i.e. directives)
    unset( _skip_store )

    # found one of directives: "SHARED", "STATIC", "MODULE"
    if( "+${_arg}" STREQUAL "+SHARED" OR "+${_arg}" STREQUAL "+STATIC" OR "+${_arg}" STREQUAL "+MODULE" )
      set( _skip_store 1 )
      set( _type "${_arg}" )
    endif( "+${_arg}" STREQUAL "+SHARED" OR "+${_arg}" STREQUAL "+STATIC" OR "+${_arg}" STREQUAL "+MODULE" )

    # found directive "STATIC_PIC"
    if( "+${_arg}" STREQUAL "+STATIC_PIC" )
      set( _skip_store 1 )
      set( _type "STATIC" )
      set( _static_pic 1 )
    endif( "+${_arg}" STREQUAL "+STATIC_PIC" )

    # found directive "AUTOMOC"
    if( "+${_arg}" STREQUAL "+AUTOMOC" )
      set( _skip_store 1 )
      set( _automoc 1 )
    endif( "+${_arg}" STREQUAL "+AUTOMOC" )

    # found directive "META_INCLUDES"
    if( "+${_arg}" STREQUAL "+META_INCLUDES" )
      set( _skip_store 1 )
      set( _storage "_meta_includes" )
    endif( )

    # found directive "NO_LIBTOOL_FILE"
    if( "+${_arg}" STREQUAL "+NO_LIBTOOL_FILE" )
      set( _skip_store 1 )
      set( _no_libtool_file 1 )
    endif( "+${_arg}" STREQUAL "+NO_LIBTOOL_FILE" )

    # found directive "NO_EXPORT"
    if( "+${_arg}" STREQUAL "+NO_EXPORT" )
      set( _skip_store 1 )
      set( _no_export 1 )
    endif( "+${_arg}" STREQUAL "+NO_EXPORT" )

    # found directive "VERSION"
    if( "+${_arg}" STREQUAL "+VERSION" )
      set( _skip_store 1 )
      set( _storage "_version" )
    endif( "+${_arg}" STREQUAL "+VERSION" )

    # found directive "RELEASE"
    if( "+${_arg}" STREQUAL "+RELEASE" )
      set( _skip_store 1 )
      set( _storage "_release" )
    endif( "+${_arg}" STREQUAL "+RELEASE" )

    # found directive "SOURCES"
    if( "+${_arg}" STREQUAL "+SOURCES" )
      set( _skip_store 1 )
      set( _storage "_sources" )
    endif( "+${_arg}" STREQUAL "+SOURCES" )

    # found directive "CXX_FEATURES"
    if( "+${_arg}" STREQUAL "+CXX_FEATURES" )
      set( _skip_store 1 )
      set( _storage "_cxx_features" )
    endif( "+${_arg}" STREQUAL "+CXX_FEATURES" )

    # found directive "CXX_FEATURES_PRIVATE"
    if( "+${_arg}" STREQUAL "+CXX_FEATURES_PRIVATE" )
      set( _skip_store 1 )
      set( _storage "_cxx_features_private" )
    endif( "+${_arg}" STREQUAL "+CXX_FEATURES_PRIVATE" )

    # found directive "EMBED"
    if( "+${_arg}" STREQUAL "+EMBED" )
      set( _skip_store 1 )
      set( _storage "_embed" )
    endif( "+${_arg}" STREQUAL "+EMBED" )

    # found directive "LINK"
    if( "+${_arg}" STREQUAL "+LINK" )
      set( _skip_store 1 )
      set( _storage "_link" )
    endif( "+${_arg}" STREQUAL "+LINK" )

    # found directive "LINK_PRIVATE"
    if( "+${_arg}" STREQUAL "+LINK_PRIVATE" )
      set( _skip_store 1 )
      set( _storage "_link_private" )
    endif( "+${_arg}" STREQUAL "+LINK_PRIVATE" )

    # found directive "DEPENDENCIES"
    if( "+${_arg}" STREQUAL "+DEPENDENCIES" )
      set( _skip_store 1 )
      set( _storage "_dependencies" )
    endif( "+${_arg}" STREQUAL "+DEPENDENCIES" )

    # found directive "DESTINATION"
    if( "+${_arg}" STREQUAL "+DESTINATION" )
      set( _skip_store 1 )
      set( _storage "_destination" )
      unset( ${_storage} )
    endif( "+${_arg}" STREQUAL "+DESTINATION" )

    # found directive "EXCLUDE_FROM_ALL"
    if( "+${_arg}" STREQUAL "+EXCLUDE_FROM_ALL" )
      set( _skip_store 1 )
      set( _exclude_from_all "EXCLUDE_FROM_ALL" )
    endif( "+${_arg}" STREQUAL "+EXCLUDE_FROM_ALL" )

   # metadata
    if( "+${_arg}" STREQUAL "+DESCRIPTION" )
      set( _skip_store 1 )
      set( _storage "_description" )
    endif( )
    if( "+${_arg}" STREQUAL "+LICENSE" )
      set( _skip_store 1 )
      set( _storage "_license" )
    endif( )
    if( "+${_arg}" STREQUAL "+COPYRIGHT" )
      set( _skip_store 1 )
      set( _storage "_copyright" )
    endif( )
    if( "+${_arg}" STREQUAL "+AUTHORS" )
      set( _skip_store 1 )
      set( _storage "_authors" )
    endif( )
    if( "+${_arg}" STREQUAL "+PRODUCT" )
      set( _skip_store 1 )
      set( _storage "_product" )
    endif( )
    if( "+${_arg}" STREQUAL "+ORGANIZATION" )
      set( _skip_store 1 )
      set( _storage "_organization" )
    endif( )
    if( "+${_arg}" STREQUAL "+VERSION" )
      set( _skip_store 1 )
      set( _storage "_version" )
    endif( )
    if( "+${_arg}" STREQUAL "+DATETIME" )
      set( _skip_store 1 )
      set( _storage "_datetime" )
    endif( )
    if( "+${_arg}" STREQUAL "+NOTES" )
      set( _skip_store 1 )
      set( _storage "_notes" )
    endif( )

    # storing value
    if( _storage AND NOT _skip_store )
      list( APPEND ${_storage} ${_arg} )
      list( REMOVE_DUPLICATES ${_storage} )
    endif( _storage AND NOT _skip_store )

  endforeach( _arg )

  # if no type is set, we choose one
  # based on BUILD_SHARED_LIBS
  if( NOT _type )
    if( BUILD_SHARED_LIBS )
      set( _type "SHARED" )
    else( BUILD_SHARED_LIBS )
      set( _type "STATIC" )
    endif( BUILD_SHARED_LIBS )
  endif( NOT _type )

  # change target name, based on type
  string( TOLOWER "${_type}" _type_lower )
  set( _target "${_arg_target}-${_type_lower}" )

  # create variables like "LIB_xxx" for convenience
  if( ${_type} STREQUAL "SHARED" )
    string( TOUPPER "${_arg_target}" _tmp )
    set( LIB_${_tmp} ${_target} CACHE INTERNAL LIB_${tmp} FORCE )
  endif( ${_type} STREQUAL "SHARED" )

  # disallow target without sources
  if( NOT _sources )
    message( FATAL_ERROR "\nTarget [$_target] have no sources." )
  endif( NOT _sources )

  # processing different types of sources
  __tde_internal_process_sources( _sources ${_sources} )

  # set automoc
  if( _automoc )
    tde_automoc( ${_sources} )
  endif( _automoc )

  # add target
  add_library( ${_target} ${_type} ${_exclude_from_all} ${_sources} )

  # set cxx features
  if( _cxx_features )
    target_compile_features( ${_target} PUBLIC ${_cxx_features} )
  endif( )
  if( TDE_CXX_FEATURES OR PROJECT_CXX_FEATURES OR _cxx_features_private )
    target_compile_features( ${_target} PRIVATE
      ${TDE_CXX_FEATURES} ${PROJECT_CXX_FEATURES} ${_cxx_features_private} )
  endif( )

  # we assume that modules have no prefix and no version
  # also, should not link
  if( ${_type} STREQUAL "MODULE" )
    set_target_properties( ${_target} PROPERTIES PREFIX "" )
    unset( _version )
    set( _shouldnotlink yes )
  endif( ${_type} STREQUAL "MODULE" )

  # set real name of target
  if( _release )
    # add release number to output name
    set_target_properties( ${_target} PROPERTIES RELEASE ${_release} )
    set_target_properties( ${_target} PROPERTIES OUTPUT_NAME "${_arg_target}-${_release}" )
  else( _release )
    set_target_properties( ${_target} PROPERTIES OUTPUT_NAME ${_arg_target} )
  endif( _release )

  # set -fPIC flag for static libraries
  if( _static_pic )
    if( "${CMAKE_VERSION}" VERSION_LESS "2.8.9" )
      set_target_properties( ${_target} PROPERTIES COMPILE_FLAGS -fPIC )
    else( )
      set_target_properties( ${_target} PROPERTIES POSITION_INDEPENDENT_CODE ON )
    endif( )
  endif( _static_pic )

  # set version
  if( _version )
    if( ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD" )
      # OpenBSD: _soversion and _version both contains only major and minor
      string( REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$" _dummy  "${_version}" )
      set( _version  "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}" )
      set( _soversion  "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}" )
    else( ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD" )
      # General (Linux) case: _soversion contains only the major number of version
      string( REGEX MATCH "^[0-9]+" _soversion ${_version} )
    endif( ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD" )
    set_target_properties( ${_target} PROPERTIES VERSION ${_version} SOVERSION ${_soversion} )
  endif( _version )

  # set interface libraries (only for shared)
  unset( _shared_libs )
  if( NOT ${_type} STREQUAL "STATIC" )
    foreach( _lib ${_link} )
      #get_target_property( _lib_type ${_lib} TYPE )
      #if( NOT "STATIC_LIBRARY" STREQUAL "${_lib_type}" )
      if( NOT ${_lib} MATCHES ".+-static" )
        list( APPEND _shared_libs ${_lib} )
      endif( NOT ${_lib} MATCHES ".+-static" )
      #endif( NOT "STATIC_LIBRARY" STREQUAL "${_lib_type}" )
    endforeach( _lib )
  endif( NOT ${_type} STREQUAL "STATIC" )

  # set embedded archives
  if( _embed )
    if( ${CMAKE_SYSTEM_NAME} MATCHES "SunOS" )
      list( INSERT _link_private 0 -Wl,-zallextract ${_embed} -Wl,-zdefaultextract )
    else( )
      list( INSERT _link_private 0 -Wl,-whole-archive ${_embed} -Wl,-no-whole-archive )
    endif( )
  endif( _embed )

  # set private linked libraries
  if( _link_private )
    if( NOT ${CMAKE_VERSION} VERSION_LESS "2.8.12" )
      if( _link )
        list( INSERT _link 0 "PUBLIC" )
      endif()
      list( INSERT _link_private 0 "PRIVATE" )
    endif()
    list( INSERT _link 0 ${_link_private} )
  endif( _link_private )

  # set link libraries
  if( _link )
    if( _embed AND ${CMAKE_VERSION} VERSION_EQUAL "2.8.12.0" )
      # hack for broken CMake 2.8.12.0
      set_target_properties( ${_target} PROPERTIES LINK_LIBRARIES "${_link}" )
    else( _embed AND  ${CMAKE_VERSION} VERSION_EQUAL "2.8.12.0" )
      target_link_libraries( ${_target} ${_link} )
    endif( _embed AND  ${CMAKE_VERSION} VERSION_EQUAL "2.8.12.0" )
  endif( )
  if( _shared_libs )
    string( TOUPPER "${CMAKE_BUILD_TYPE}" _build_type )
    set_target_properties( ${_target} PROPERTIES
                           LINK_INTERFACE_LIBRARIES "${_shared_libs}"
                           LINK_INTERFACE_LIBRARIES_${_build_type} "${_shared_libs}"
                           INTERFACE_LINK_LIBRARIES "${_shared_libs}"
                           INTERFACE_LINK_LIBRARIES_${_build_type} "${_shared_libs}" )
  endif( _shared_libs )

  # set dependencies
  if( _dependencies )
    add_dependencies( ${_target} ${_dependencies} )
  endif( _dependencies )

  # if destination directory is set
  if( _destination )

    # we export only shared libs (no static, no modules);
    # also, do not export targets marked as "NO_EXPORT" (usually for tdeinit)
    if( "SHARED" STREQUAL ${_type} AND NOT _no_export )

      # get target properties: output name, version, soversion
      tde_get_library_filename( _output ${_target} )
      get_target_property( _version ${_target} VERSION )
      get_target_property( _soversion ${_target} SOVERSION )

      if( _version )
        set( _location "${_destination}/${_output}.${_version}" )
        set( _soname "${_output}.${_soversion}" )
      else( )
        set( _location "${_destination}/${_output}" )
        set( _soname "${_output}" )
        unset( _version )
      endif( )

      configure_file( ${TDE_CMAKE_TEMPLATES}/tde_export_library.cmake "${PROJECT_BINARY_DIR}/export-${_target}.cmake" @ONLY )
    endif( )

    # install target
    install( TARGETS ${_target} DESTINATION ${_destination} )

    # install base soname
    if( _release AND NOT "STATIC" STREQUAL ${_type} )
      tde_get_library_filename( _soname ${_target} )
      string( REPLACE "-${_release}" "" _soname_base "${_soname}" )
      if( _version )
        get_target_property( _soversion ${_target} SOVERSION )
        set( _soname "${_soname}.${_soversion}" )
      endif( )
      if( NOT _exclude_from_all )
        add_custom_command(
          OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${_soname_base}"
          COMMAND ln -s ${_soname} "${CMAKE_CURRENT_BINARY_DIR}/${_soname_base}"
          DEPENDS ${_target}
        )
        add_custom_target(
          ${_target}+base-so ALL
          DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${_soname_base}"
        )
      else( )
        add_custom_command(
          TARGET ${_target} POST_BUILD
          COMMAND ln -s ${_soname} "${CMAKE_CURRENT_BINARY_DIR}/${_soname_base}"
        )
      endif( )
      install( FILES "${CMAKE_CURRENT_BINARY_DIR}/${_soname_base}" DESTINATION ${_destination} )
    endif( )

    # install .la files for dynamic libraries
    if( NOT "STATIC" STREQUAL ${_type} AND NOT _no_libtool_file )
      tde_install_libtool_file( ${_target} ${_destination} )
    endif( )

  endif( _destination )

  # embed name and metadata
  set( ELF_EMBEDDING_METADATA "\"${_target}\" \"${_description}\" \"${_license}\" \"${_copyright}\" \"${_authors}\" \"${_product}\" \"${_organization}\" \"${_version}\" \"${_datetime}\" \"x-sharedlib\" \"${TDE_SCM_MODULE_NAME}\" \"${TDE_SCM_MODULE_REVISION}\" \"${_notes}\"" )
  separate_arguments( ELF_EMBEDDING_METADATA )
  if( TDELFEDITOR_EXECUTABLE AND _soname )
    if( _version )
      get_filename_component( _target_lib ${CMAKE_CURRENT_BINARY_DIR}/${_soname}.${_version} ABSOLUTE )
    else( )
      get_filename_component( _target_lib ${CMAKE_CURRENT_BINARY_DIR}/${_soname} ABSOLUTE )
    endif( )
    file( RELATIVE_PATH _target_path "${CMAKE_BINARY_DIR}" "${_target_lib}" )

    if( TARGET ${TDELFEDITOR_EXECUTABLE} AND NOT _exclude_from_all )
      # create target for all metadata writes
      if( NOT TARGET tdelfeditor-write )
        add_custom_target( tdelfeditor-write
          WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
          DEPENDS ${TDELFEDITOR_EXECUTABLE}
          COMMENT "Write metadata to binaries..."
        )
      endif( )
      add_custom_target(
        ${_target}+metadata ALL
        COMMAND ${TDELFEDITOR_EXECUTABLE} -m ${_target_lib} ${ELF_EMBEDDING_METADATA} || true
        COMMAND ${TDELFEDITOR_EXECUTABLE} -e ${_target_lib} || true
        COMMENT "Storing SCM metadata in ${_target_path}"
        DEPENDS tdelfeditor-write
      )
      add_dependencies( tdelfeditor-write ${_target} )
    else( )
      add_custom_command(
        TARGET ${_target}
        POST_BUILD
        COMMAND ${TDELFEDITOR_EXECUTABLE} -m ${_target_lib} ${ELF_EMBEDDING_METADATA} || true
        COMMAND ${TDELFEDITOR_EXECUTABLE} -e ${_target_lib} || true
        COMMENT "Storing SCM metadata in ${_target_path}"
      )
      if( TARGET ${TDELFEDITOR_EXECUTABLE} )
        add_dependencies( ${_target} ${TDELFEDITOR_EXECUTABLE} )
      endif()
    endif( )
  endif( TDELFEDITOR_EXECUTABLE AND _soname )

endmacro( tde_add_library )


#################################################
#####
##### tde_add_kpart

macro( tde_add_kpart _target )
  tde_add_library( ${_target} ${ARGN} MODULE )
endmacro( tde_add_kpart )


#################################################
#####
##### tde_curdatetime

macro( tde_curdatetime result )
  if( TDE_PKG_DATETIME )
    set( ${result} ${TDE_PKG_DATETIME} )
  elseif( TDE_SCM_MODULE_DATETIME )
    set( ${result} ${TDE_SCM_MODULE_DATETIME} )
  else( )
    tde_execute_process( COMMAND "date" "-u" "+%m/%d/%Y %H:%M:%S" OUTPUT_VARIABLE ${result} )
    string( REGEX REPLACE "(..)/(..)/(....) (........).*" "\\1/\\2/\\3 \\4" ${result} ${${result}} )
  endif( )
endmacro( tde_curdatetime )


#################################################
#####
##### tde_add_executable

macro( tde_add_executable _arg_target )

  unset( _target )
  unset( _automoc )
  unset( _meta_includes )
  unset( _setuid )
  unset( _sources )
  unset( _cxx_features )
  unset( _destination )
  unset( _link )
  unset( _dependencies )
  unset( _storage )

  # metadata
  unset( _description )
  unset( _license )
  unset( _copyright )
  unset( _authors )
  unset( _product )
  unset( _organization )
  unset( _version )
  unset( _datetime )
  unset( _notes )

  # default metadata
  set( _product "Trinity Desktop Environment" )
  set( _version "${TDE_VERSION}" )
  if( TDE_PKG_VERSION )
    set( _version "${_version} (${TDE_PKG_VERSION})" )
  endif( )
  tde_curdatetime( _datetime )

  foreach( _arg ${ARGV} )

    # this variable help us to skip
    # storing unapropriate values (i.e. directives)
    unset( _skip_store )

    # found directive "AUTOMOC"
    if( "+${_arg}" STREQUAL "+AUTOMOC" )
      set( _skip_store 1 )
      set( _automoc 1 )
    endif( "+${_arg}" STREQUAL "+AUTOMOC" )

    # found directive "META_INCLUDES"
    if( "+${_arg}" STREQUAL "+META_INCLUDES" )
      set( _skip_store 1 )
      set( _storage "_meta_includes" )
    endif( )

    # found directive "SETUID"
    if( "+${_arg}" STREQUAL "+SETUID" )
      set( _skip_store 1 )
      set( _setuid 1 )
    endif( "+${_arg}" STREQUAL "+SETUID" )

    # found directive "SOURCES"
    if( "+${_arg}" STREQUAL "+SOURCES" )
      set( _skip_store 1 )
      set( _storage "_sources" )
    endif( "+${_arg}" STREQUAL "+SOURCES" )

    # found directive "CXX_FEATURES"
    if( "+${_arg}" STREQUAL "+CXX_FEATURES" )
      set( _skip_store 1 )
      set( _storage "_cxx_features" )
    endif( "+${_arg}" STREQUAL "+CXX_FEATURES" )

    # found directive "LINK"
    if( "+${_arg}" STREQUAL "+LINK" )
      set( _skip_store 1 )
      set( _storage "_link" )
    endif( "+${_arg}" STREQUAL "+LINK" )

    # found directive "DEPENDENCIES"
    if( "+${_arg}" STREQUAL "+DEPENDENCIES" )
      set( _skip_store 1 )
      set( _storage "_dependencies" )
    endif( "+${_arg}" STREQUAL "+DEPENDENCIES" )

    # found directive "DESTINATION"
    if( "+${_arg}" STREQUAL "+DESTINATION" )
      set( _skip_store 1 )
      set( _storage "_destination" )
      unset( ${_storage} )
    endif( "+${_arg}" STREQUAL "+DESTINATION" )

    # metadata
    if( "+${_arg}" STREQUAL "+DESCRIPTION" )
      set( _skip_store 1 )
      set( _storage "_description" )
    endif( )
    if( "+${_arg}" STREQUAL "+LICENSE" )
      set( _skip_store 1 )
      set( _storage "_license" )
    endif( )
    if( "+${_arg}" STREQUAL "+COPYRIGHT" )
      set( _skip_store 1 )
      set( _storage "_copyright" )
    endif( )
    if( "+${_arg}" STREQUAL "+AUTHORS" )
      set( _skip_store 1 )
      set( _storage "_authors" )
    endif( )
    if( "+${_arg}" STREQUAL "+PRODUCT" )
      set( _skip_store 1 )
      set( _storage "_product" )
    endif( )
    if( "+${_arg}" STREQUAL "+ORGANIZATION" )
      set( _skip_store 1 )
      set( _storage "_organization" )
    endif( )
    if( "+${_arg}" STREQUAL "+VERSION" )
      set( _skip_store 1 )
      set( _storage "_version" )
    endif( )
    if( "+${_arg}" STREQUAL "+DATETIME" )
      set( _skip_store 1 )
      set( _storage "_datetime" )
    endif( )
    if( "+${_arg}" STREQUAL "+NOTES" )
      set( _skip_store 1 )
      set( _storage "_notes" )
    endif( )

    # storing value
    if( _storage AND NOT _skip_store )
      #set( ${_storage} "${${_storage}} ${_arg}" )
      list( APPEND ${_storage} ${_arg} )
    endif( _storage AND NOT _skip_store )

  endforeach( _arg )

  set( _target "${_arg_target}" )

  # disallow target without sources
  if( NOT _sources )
    message( FATAL_ERROR "\nTarget [$_target] have no sources." )
  endif( NOT _sources )

  # processing different types of sources
  __tde_internal_process_sources( _sources ${_sources} )

  # set automoc
  if( _automoc )
    tde_automoc( ${_sources} )
  endif( _automoc )

  # add target
  add_executable( ${_target} ${_sources} )

  # set cxx features
  if( TDE_CXX_FEATURES OR PROJECT_CXX_FEATURES OR _cxx_features )
    target_compile_features( ${_target} PRIVATE
      ${TDE_CXX_FEATURES} ${PROJECT_CXX_FEATURES} ${_cxx_features} )
  endif( )

  # set link libraries
  if( _link )
    target_link_libraries( ${_target} ${_link} )
  endif( _link )

  # set dependencies
  if( _dependencies )
    add_dependencies( ${_target} ${_dependencies} )
  endif( _dependencies )

  # set PIE flags for setuid binaries
  if( _setuid )
    set_target_properties( ${_target} PROPERTIES COMPILE_FLAGS "${TDE_PIE_CFLAGS}" )
    set_target_properties( ${_target} PROPERTIES LINK_FLAGS "${TDE_PIE_LDFLAGS}" )
  endif( _setuid )

  # set destination directory
  if( _destination )
    if( _setuid )
      install( TARGETS ${_target} DESTINATION ${_destination} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE SETUID )
    else( _setuid )
      install( TARGETS ${_target} DESTINATION ${_destination} )
    endif( _setuid )
  endif( _destination )

  # embed icon, name, and metadata
  set( ELF_EMBEDDING_METADATA "\"${_target}\" \"${_description}\" \"${_license}\" \"${_copyright}\" \"${_authors}\" \"${_product}\" \"${_organization}\" \"${_version}\" \"${_datetime}\" \"${_target}\" \"${TDE_SCM_MODULE_NAME}\" \"${TDE_SCM_MODULE_REVISION}\" \"${_notes}\"" )
  separate_arguments( ELF_EMBEDDING_METADATA )
  if( TDELFEDITOR_EXECUTABLE )
    get_filename_component( _target_path ${CMAKE_CURRENT_BINARY_DIR}/${_target} ABSOLUTE )
    file( RELATIVE_PATH _target_path "${CMAKE_BINARY_DIR}" "${_target_path}" )
    if( TARGET ${TDELFEDITOR_EXECUTABLE} )
      # create target for all metadata writes
      if( NOT TARGET tdelfeditor-write )
        add_custom_target( tdelfeditor-write
          WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
          DEPENDS ${TDELFEDITOR_EXECUTABLE}
          COMMENT "Write metadata to binaries..."
        )
      endif( )
      add_custom_target(
        ${_target}+metadata ALL
        COMMAND ${TDELFEDITOR_EXECUTABLE} -m ${CMAKE_CURRENT_BINARY_DIR}/${_target} ${ELF_EMBEDDING_METADATA} || true
        COMMAND ${TDELFEDITOR_EXECUTABLE} -e ${CMAKE_CURRENT_BINARY_DIR}/${_target} || true
        COMMENT "Storing SCM metadata in ${_target_path}"
        DEPENDS tdelfeditor-write
      )
      add_dependencies( tdelfeditor-write ${_target} )
    else()
      add_custom_command(
        TARGET ${_target}
        POST_BUILD
        COMMAND ${TDELFEDITOR_EXECUTABLE} -m ${CMAKE_CURRENT_BINARY_DIR}/${_target} ${ELF_EMBEDDING_METADATA} || true
        COMMAND ${TDELFEDITOR_EXECUTABLE} -e ${CMAKE_CURRENT_BINARY_DIR}/${_target} || true
        COMMAND ${TDELFEDITOR_EXECUTABLE} -t ${CMAKE_CURRENT_BINARY_DIR}/${_target} ${_target} || true
        COMMENT "Storing SCM metadata in ${_target_path}"
      )
    endif()
  endif( TDELFEDITOR_EXECUTABLE )

endmacro( tde_add_executable )


#################################################
#####
##### tde_add_check_executable

macro( tde_add_check_executable _arg_target )

  unset( _target )
  unset( _automoc )
  unset( _test )
  unset( _test_args )
  unset( _meta_includes )
  unset( _sources )
  unset( _cxx_features )
  unset( _destination )
  unset( _link )
  unset( _dependencies )
  unset( _storage )

  foreach( _arg ${ARGV} )

    # this variable help us to skip
    # storing unapropriate values (i.e. directives)
    unset( _skip_store )

    # found directive "AUTOMOC"
    if( "+${_arg}" STREQUAL "+AUTOMOC" )
      set( _skip_store 1 )
      set( _automoc 1 )
    endif( "+${_arg}" STREQUAL "+AUTOMOC" )

    # found directive "TEST"
    if( "+${_arg}" STREQUAL "+TEST" )
      set( _skip_store 1 )
      set( _test 1 )
      set( _storage "_test_args" )
    endif( "+${_arg}" STREQUAL "+TEST" )

    # found directive "META_INCLUDES"
    if( "+${_arg}" STREQUAL "+META_INCLUDES" )
      set( _skip_store 1 )
      set( _storage "_meta_includes" )
    endif( )

    # found directive "SOURCES"
    if( "+${_arg}" STREQUAL "+SOURCES" )
      set( _skip_store 1 )
      set( _storage "_sources" )
    endif( "+${_arg}" STREQUAL "+SOURCES" )

    # found directive "CXX_FEATURES"
    if( "+${_arg}" STREQUAL "+CXX_FEATURES" )
      set( _skip_store 1 )
      set( _storage "_cxx_features" )
    endif( "+${_arg}" STREQUAL "+CXX_FEATURES" )

    # found directive "LINK"
    if( "+${_arg}" STREQUAL "+LINK" )
      set( _skip_store 1 )
      set( _storage "_link" )
    endif( "+${_arg}" STREQUAL "+LINK" )

    # found directive "DEPENDENCIES"
    if( "+${_arg}" STREQUAL "+DEPENDENCIES" )
      set( _skip_store 1 )
      set( _storage "_dependencies" )
    endif( "+${_arg}" STREQUAL "+DEPENDENCIES" )

    # storing value
    if( _storage AND NOT _skip_store )
      #set( ${_storage} "${${_storage}} ${_arg}" )
      list( APPEND ${_storage} ${_arg} )
    endif( _storage AND NOT _skip_store )

  endforeach( _arg )

  set( _target "${_arg_target}" )

  # try to autodetect sources
  if( NOT _sources )
    file( GLOB _sources "${_target}.cpp" "${_target}.cxx" "${_target}.c" )
    if( NOT _sources )
      message( FATAL_ERROR "\nNo sources found for test executable \"${_target}\"." )
    endif( )
  endif( NOT _sources )

  # processing different types of sources
  __tde_internal_process_sources( _sources ${_sources} )

  # set automoc
  if( _automoc )
    tde_automoc( ${_sources} )
  endif( _automoc )

  # add target
  add_executable( ${_target} EXCLUDE_FROM_ALL ${_sources} )

  # set cxx features
  if( TDE_CXX_FEATURES OR PROJECT_CXX_FEATURES OR _cxx_features )
    target_compile_features( ${_target} PRIVATE
      ${TDE_CXX_FEATURES} ${PROJECT_CXX_FEATURES} ${_cxx_features} )
  endif( )

  # set link libraries
  if( _link )
    target_link_libraries( ${_target} ${_link} )
  endif( _link )

  # set dependencies
  if( _dependencies )
    add_dependencies( ${_target} ${_dependencies} )
  endif( _dependencies )

  # create make check target
  if(NOT TARGET check)
      add_custom_target( check
        COMMAND ${CMAKE_CTEST_COMMAND}
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Running tests..." )
  endif(NOT TARGET check)

  add_dependencies( check ${_target} )

  # add test target
  if( _test )
    # get relative path to current directory and strip end tests dir
    file( RELATIVE_PATH _test_prefix ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} )
    string( REGEX REPLACE "(^\\.+/?|(^|/)tests?$|/$)" "" _test_prefix "${_test_prefix}" )
    if( _test_prefix )
        set( _test_prefix "${_test_prefix}/" )
    endif( _test_prefix )
    add_test( NAME "${_test_prefix}${_target}" COMMAND "${_target}" ${_test_args} )
  endif( _test )

endmacro( tde_add_check_executable )


#################################################
#####
##### tde_add_tdeinit_executable

macro( tde_add_tdeinit_executable _target )

  configure_file( ${TDE_CMAKE_TEMPLATES}/tde_tdeinit_executable.cmake ${_target}_tdeinit_executable.cpp COPYONLY )
  configure_file( ${TDE_CMAKE_TEMPLATES}/tde_tdeinit_module.cmake ${_target}_tdeinit_module.cpp COPYONLY )

  unset( _sources )
  unset( _runtime_destination )
  unset( _library_destination )
  unset( _plugin_destination )

  # default storage is _sources
  set( _storage _sources )

  # set default export to NO_EXPORT
  set( _export "NO_EXPORT" )

  foreach( _arg ${ARGN} )

    # this variable help us to skip
    # storing unapropriate values (i.e. directives)
    unset( _skip_store )

    # found directive "EXPORT"
    if( "+${_arg}" STREQUAL "+EXPORT" )
      set( _skip_store 1 )
      unset( _export )
    endif( "+${_arg}" STREQUAL "+EXPORT" )

    # found directive "RUNTIME_DESTINATION"
    if( "+${_arg}" STREQUAL "+RUNTIME_DESTINATION" )
      set( _skip_store 1 )
      set( _storage "_runtime_destination" )
      unset( ${_storage} )
    endif( "+${_arg}" STREQUAL "+RUNTIME_DESTINATION" )

    # found directive "LIBRARY_DESTINATION"
    if( "+${_arg}" STREQUAL "+LIBRARY_DESTINATION" )
      set( _skip_store 1 )
      set( _storage "_library_destination" )
      unset( ${_storage} )
    endif( "+${_arg}" STREQUAL "+LIBRARY_DESTINATION" )

    # found directive "PLUGIN_DESTINATION"
    if( "+${_arg}" STREQUAL "+PLUGIN_DESTINATION" )
      set( _skip_store 1 )
      set( _storage "_plugin_destination" )
      unset( ${_storage} )
    endif( "+${_arg}" STREQUAL "+PLUGIN_DESTINATION" )

    # storing value
    if( _storage AND NOT _skip_store )
      list( APPEND ${_storage} ${_arg} )
      set( _storage "_sources" )
    endif( _storage AND NOT _skip_store )

  endforeach( _arg )

  # if destinations are not set, we using some defaults
  # we assume that tdeinit executable MUST be installed
  # (otherwise why we build it?)
  if( NOT _runtime_destination )
    set( _runtime_destination ${BIN_INSTALL_DIR} )
  endif( NOT _runtime_destination )
  if( NOT _library_destination )
    set( _library_destination ${LIB_INSTALL_DIR} )
  endif( NOT _library_destination )
  if( NOT _plugin_destination )
    set( _plugin_destination ${PLUGIN_INSTALL_DIR} )
  endif( NOT _plugin_destination )

  # create the library
  tde_add_library( tdeinit_${_target} ${_sources} SHARED ${_export}
    DESTINATION ${_library_destination}
  )

  # create the executable
  tde_add_executable( ${_target}
    SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${_target}_tdeinit_executable.cpp
    LINK tdeinit_${_target}-shared
    DESTINATION ${_runtime_destination}
  )

  # create the plugin
  tde_add_kpart( ${_target}
    SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${_target}_tdeinit_module.cpp
    LINK tdeinit_${_target}-shared
    DESTINATION ${_plugin_destination}
  )

endmacro( tde_add_tdeinit_executable )


#################################################
#####
##### tde_conditional_add_project_translations
##### tde_add_project_translations
#####
##### Macro for standard processing and installation of translations.
##### This is designed for ordinary modules - as an applications, not for core modules.

function( tde_conditional_add_project_translations _cond )

  if( ${_cond} )
    tde_add_project_translations()
  endif()

endfunction()

function( tde_add_project_translations )

  if( ${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${PROJECT_SOURCE_DIR} )
    set( TRANSLATIONS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/translations/messages )
  else()
    set( TRANSLATIONS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
  endif()

  file( GLOB_RECURSE po_files RELATIVE ${TRANSLATIONS_SOURCE_DIR} ${TRANSLATIONS_SOURCE_DIR}/*.po )
  string( REGEX REPLACE "[ \r\n\t]+" ";" _linguas "$ENV{LINGUAS}" )

  foreach( _po ${po_files} )
    get_filename_component( _lang ${_po} NAME_WE )
    if( "${_linguas}" MATCHES "^;*$" OR ";${_linguas};" MATCHES ";${_lang};" )
      if( "${_po}" MATCHES "^([^/]*)/.*" )
        string( REGEX REPLACE "^([^/]*)/.*" "\\1" _component "${_po}" )
      else( )
        set( _component "${PROJECT_NAME}" )
      endif( )
      tde_create_translation( FILES ${TRANSLATIONS_SOURCE_DIR}/${_po} LANG ${_lang} OUTPUT_NAME ${_component} )
    endif( )
  endforeach( )

endfunction()


#################################################
#####
##### tde_create_translation

macro( tde_create_translation )

  unset( _srcs )
  unset( _lang )
  unset( _dest )
  unset( _out_name )
  unset( _directive )
  unset( _var )

  foreach( _arg ${ARGN} )

    # found directive "FILES"
    if( "+${_arg}" STREQUAL "+FILES" )
      unset( _srcs )
      set( _var _srcs )
      set( _directive 1 )
    endif( )

    # found directive "LANG"
    if( "+${_arg}" STREQUAL "+LANG" )
      unset( _lang )
      set( _var _lang )
      set( _directive 1 )
    endif( )

    # found directive "DESTINATION"
    if( "+${_arg}" STREQUAL "+DESTINATION" )
      unset( _dest )
      set( _var _dest )
      set( _directive 1 )
    endif( )

    # found directive "OUTPUT_NAME"
    if( "+${_arg}" STREQUAL "+OUTPUT_NAME" )
      unset( _out_name )
      set( _var _out_name )
      set( _directive 1 )
    endif( )

    # collect data
    if( _directive )
      unset( _directive )
    elseif( _var )
      list( APPEND ${_var} ${_arg} )
    endif()

  endforeach( )

  if( NOT MSGFMT_EXECUTABLE )
    tde_setup_msgfmt( )
  endif( )
  if( NOT _lang )
    tde_message_fatal( "missing LANG directive" )
  endif( )

  # if no file specified, include all *.po files
  if( NOT _srcs )
    file( GLOB _srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.po  )
  endif( )
  if( NOT _srcs )
    tde_message_fatal( "no source files" )
  endif( )

  if( NOT _lang STREQUAL "auto")
    set( _real_lang ${_lang} )

    if( NOT _dest )
      set( _dest "${LOCALE_INSTALL_DIR}/${_lang}/LC_MESSAGES" )
    endif( )

    # OUTPUT_NAME can only be used if we have only one file
    list( LENGTH _srcs _srcs_num)
    if( _out_name AND _srcs_num GREATER 1 )
      tde_message_fatal( "OUTPUT_NAME can be supplied only with single file or LANG=auto" )
    endif( )

  elseif( NOT _out_name )
    tde_message_fatal( "LANG=auto reqires OUTPUT_NAME directive to be set" )
  elseif( _dest )
    tde_message_fatal( "DESTINATION cannot be used with LANG=auto" )
  endif( )

  # generate *.mo files
  foreach( _src ${_srcs} )

    get_filename_component( _src ${_src} ABSOLUTE )

    if( _out_name )
      set( _out ${_out_name} )
      if( _lang STREQUAL "auto" )
        get_filename_component( _real_lang ${_src} NAME_WE )
        set( _dest "${LOCALE_INSTALL_DIR}/${_real_lang}/LC_MESSAGES" )
      endif( )
    else( )
      get_filename_component( _out ${_src} NAME_WE )
    endif( )

    string( REPLACE "@" "_" _target ${_real_lang} )
    set( _out_filename "${_out}-${_real_lang}.mo" )
    set( _install_filename "${_out}.mo" )

    add_custom_command(
      OUTPUT ${_out_filename}
      COMMAND ${MSGFMT_EXECUTABLE} ${_src} -o ${_out_filename}
      DEPENDS ${_src} )
    add_custom_target( "${_out}-${_target}-translation" ALL DEPENDS ${_out_filename} )
    install( FILES ${CMAKE_CURRENT_BINARY_DIR}/${_out_filename} RENAME ${_install_filename} DESTINATION ${_dest} )

  endforeach( )

endmacro( )


#################################################
#####
##### tde_create_translated_desktop
#####
##### Macro is used to merge translations into desktop file
#####
##### Syntax:
#####   tde_create_translated_desktop(
#####     [SOURCE] file_name
#####     [KEYWORDS keyword [keyword]]
#####     [FORMAT (desktop|xml)]
#####     [PO_DIR po_directory]
#####     [DESTINATION directory]
#####     [OUTPUT_NAME file_name]
#####   )

macro( tde_create_translated_desktop )

  unset( _srcs )
  unset( _arg_out_name )
  unset( _arg_po_dir )
  unset( _keywords_add )
  unset( _dest )
  unset( _directive )
  set( _var _srcs )
  set( _keywords_desktop_default
       "Name" "GenericName" "Comment" "Keywords"
       "Description" "ExtraNames" "X-TDE-Submenu" )
  set( _format "desktop" )

  foreach( _arg ${ARGN} )

    # found directive "SOURCE"
    if( "+${_arg}" STREQUAL "+SOURCE" )
      unset( _srcs )
      set( _var _srcs )
      set( _directive 1 )
    endif( )

    # found directive "KEYWORDS"
    if( "+${_arg}" STREQUAL "+KEYWORDS" )
      unset( _keywords_add )
      set( _var _keywords_add )
      set( _directive 1 )
    endif( )

    # found directive "FORMAT"
    if( "+${_arg}" STREQUAL "+FORMAT" )
      unset( _format )
      set( _var _format )
      set( _directive 1 )
    endif( )

    # found directive "PO_DIR"
    if( "+${_arg}" STREQUAL "+PO_DIR" )
      unset( _arg_po_dir )
      set( _var _arg_po_dir )
      set( _directive 1 )
    endif( )

    # found directive "DESTINATION"
    if( "+${_arg}" STREQUAL "+DESTINATION" )
      unset( _dest )
      set( _var _dest )
      set( _directive 1 )
    endif( )

    # found directive "OUTPUT_NAME"
    if( "+${_arg}" STREQUAL "+OUTPUT_NAME" )
      unset( _arg_out_name )
      set( _var _arg_out_name )
      set( _directive 1 )
    endif( )

    # collect data
    if( _directive )
      unset( _directive )
    elseif( _var )
      list( APPEND ${_var} ${_arg} )
    endif()

  endforeach( )

  # no source file specified!
  if( NOT _srcs )
    tde_message_fatal( "no source desktop file specified" )
  endif( )

  # OUTPUT_NAME can only be used if we have only one file
  list( LENGTH _srcs _srcs_num )
  if( _arg_out_name AND _srcs_num GREATER 1 )
    tde_message_fatal( "OUTPUT_NAME can be supplied only with single file" )
  endif( )

  # if no destination directory specified, install as application link
  if( NOT _dest )
    set( _dest ${XDG_APPS_INSTALL_DIR} )
  endif( )

  # select a tool for merging desktop file translations
  #
  # Because some of our desktop files contain underscores in variable
  # names (for example eventsrc), which is not an allowed character
  # for names of entries in desktop style files, we can't use msgfmt,
  # so we need intltool-merge.
  #
  #if( NOT MSGFMT_EXECUTABLE OR NOT MSGFMT_VERSION )
  #  tde_setup_msgfmt( )
  #endif( )
  #if( "${MSGFMT_VERSION}" VERSION_LESS "0.19" )
  if( TRUE )
    if( NOT PERL_EXECUTABLE )
      include( FindPerl )
    endif( )
    if( NOT INTLTOOL_MERGE_EXECUTABLE )
      find_file( INTLTOOL_MERGE_EXECUTABLE
        NAMES tde_l10n_merge.pl
        HINTS ${TDE_CMAKE_MODULES}
      )
      if( "${INTLTOOL_MERGE_EXECUTABLE}" STREQUAL "INTLTOOL_MERGE_EXECUTABLE-NOTFOUND" )
        #tde_message_fatal( "xgettext >= 0.19 or tde_l10n_merge.pl is required but not found" )
        tde_message_fatal( "tde_l10n_merge.pl is required but not found" )
      endif( )
      message( STATUS "Found intltool: ${INTLTOOL_MERGE_EXECUTABLE}" )
    endif( )
    set( DESKTOP_MERGE_INTLTOOL 1 )
  else( )
    set( DESKTOP_MERGE_MSGFMT 1 )
  endif( )

  # pick keywords
  unset( _keywords_desktop )
  foreach( _keyword ${_keywords_desktop_default} ${_keywords_add} )
    if( "${_keyword}" STREQUAL "-" )
      unset( _keywords_desktop )
      unset( _keyword )
    endif( )
    if( _keyword )
      list( APPEND _keywords_desktop "${_keyword}" )
    endif( )
  endforeach( )

  # prepare the length of the binary path prefix
  string( LENGTH "${CMAKE_BINARY_DIR}" CMAKE_BINARY_DIR_LEN )

  # process source files
  foreach( _src IN LISTS _srcs )

    # get a base name and a directory
    get_filename_component( _basename ${_src} ABSOLUTE )
    get_filename_component( _basedir ${_basename} PATH )
    file( RELATIVE_PATH _sourcename "${CMAKE_SOURCE_DIR}" "${_basename}" )
    string( LENGTH "${_basename}" _basename_len )
    if( ${_basename_len} LESS ${CMAKE_BINARY_DIR_LEN} )
      set( _basedir_prefix "${CMAKE_SOURCE_DIR}" )
    else( )
      string( SUBSTRING "${_basename}" 0 ${CMAKE_BINARY_DIR_LEN} _basedir_prefix )
    endif( )
    if( ${_basedir_prefix} STREQUAL "${CMAKE_BINARY_DIR}" )
      file( RELATIVE_PATH _basename "${CMAKE_CURRENT_BINARY_DIR}" "${_basename}" )
      set( _binsuffix ".out" )
    else( )
      file( RELATIVE_PATH _basename "${CMAKE_CURRENT_SOURCE_DIR}" "${_basename}" )
      set( _binsuffix "" )
    endif( )

    # prepare the binary directory according to source directory
    if( ${_basedir_prefix} STREQUAL "${CMAKE_BINARY_DIR}" )
      file( RELATIVE_PATH _binary_basedir "${CMAKE_CURRENT_BINARY_DIR}" "${_basedir}" )
    else( )
      file( RELATIVE_PATH _binary_basedir "${CMAKE_CURRENT_SOURCE_DIR}" "${_basedir}" )
    endif( )
    set( _binary_basedir "${CMAKE_CURRENT_BINARY_DIR}/${_binary_basedir}" )
    file( MAKE_DIRECTORY "${_binary_basedir}" )

    # process source file as a configuration file if necessary
    if( "+${_src}" MATCHES "\\.cmake$" )
      configure_file( ${_src} ${_basename} @ONLY )
      set( _src "${CMAKE_CURRENT_BINARY_DIR}/${_basename}" )
      string( REGEX REPLACE "\\.cmake$" "" _basename "${_basename}" )
    endif()

    # determine output name
    if( _arg_out_name )
      set( _out_name ${_arg_out_name} )
    else()
      get_filename_component( _out_name ${_basename} NAME )
    endif( )

    # determine po directory
    if( _arg_po_dir )
      set( _po_base ${_arg_po_dir} )
    else()
      get_filename_component( _po_base ${_basename} NAME )
    endif()
    if( IS_ABSOLUTE ${_po_base} )
      set( _po_dir ${_po_base} )
    else()
      if( EXISTS ${CMAKE_SOURCE_DIR}/translations/desktop_files/${_po_base} AND
          IS_DIRECTORY ${CMAKE_SOURCE_DIR}/translations/desktop_files/${_po_base} )
        set( _po_dir ${CMAKE_SOURCE_DIR}/translations/desktop_files/${_po_base} )

      elseif( EXISTS ${CMAKE_SOURCE_DIR}/po/desktop_files/${_po_base} AND
              IS_DIRECTORY ${CMAKE_SOURCE_DIR}/po/desktop_files/${_po_base} )
        set( _po_dir ${CMAKE_SOURCE_DIR}/po/desktop_files/${_po_base} )

      else()
        set( _po_dir ${CMAKE_SOURCE_DIR}/translations/desktop_files )
      endif( )
    endif( )

    # if the translated desktop file is not installed, generate to the specified output name
    if( "${_dest}" STREQUAL "-" )
      set( _basename "${_out_name}" )
      set( _binsuffix "" )
      get_filename_component( _out_dir "${CMAKE_CURRENT_BINARY_DIR}/${_out_name}" PATH )
      file( MAKE_DIRECTORY "${_out_dir}" )
    endif( )

    # are there any translations available?
    unset( _translations )
    if( EXISTS "${_po_dir}" AND IS_DIRECTORY "${_po_dir}" )
      file( GLOB _translations RELATIVE "${_po_dir}" "${_po_dir}/*.po" )
    endif( )

    # prepare a full name for the target
    get_filename_component( _target ${_basename} ABSOLUTE )
    file( RELATIVE_PATH _target "${CMAKE_SOURCE_DIR}" "${_target}-translated" )
    string( REPLACE "/" "+" _target "${_target}" )
    string( REPLACE "@" "_" _target "${_target}" )

    if( NOT TARGET ${_target} )

      # use absolute path for src
      get_filename_component( _src ${_src} ABSOLUTE )

      if( _translations )

        if( DESKTOP_MERGE_MSGFMT )

          # Decide which translations to build; the ones selected in the
          # LINGUAS environment variable, or all that are available.
          if( DEFINED ENV{LINGUAS} )
            set( _linguas "$ENV{LINGUAS}" )
          else( )
            string( REPLACE ".po;" " " _linguas "${_translations};" )
          endif( )

          # prepare keywords for msgfmt
          set( _keywords_arg "--keyword=" )
          foreach( _keyword ${_keywords_desktop} )
            list( APPEND _keywords_arg "--keyword=\"${_keyword}\"" )
          endforeach( )

          # merge translations command
          add_custom_command(
            OUTPUT ${_basename}${_binsuffix}
            COMMAND ${CMAKE_COMMAND} -E env "LINGUAS=${_linguas}" ${MSGFMT_EXECUTABLE} --desktop --template ${_src} -d ${_po_dir} -o ${_basename}${_binsuffix} ${_keywords_arg}
            DEPENDS ${_src}
            COMMENT "Merging translations into ${_sourcename}"
          )

        else( )

          # prepare keywords for intltool
          string( REPLACE ";" "|" _keywords_match "(${_keywords_desktop})" )

          # merge translations command
          if( _format STREQUAL "desktop" )
            add_custom_command(
              OUTPUT ${_basename}${_binsuffix}
              COMMAND ${PERL_EXECUTABLE} -p -e  "'s/^${_keywords_match}[ ]*=[ ]*/_\\1=/'" < ${_src} > ${_basename}.in
              COMMAND ${PERL_EXECUTABLE} ${INTLTOOL_MERGE_EXECUTABLE} -q -d ${_po_dir} ${_basename}.in ${_basename}${_binsuffix}
              DEPENDS ${_src}
              COMMENT "Merging translations into ${_sourcename}"
            )
          elseif( _format STREQUAL "xml" )
            add_custom_command(
              OUTPUT ${_basename}${_binsuffix}
              COMMAND ${PERL_EXECUTABLE} -p -e  "'s/(<\\/?)${_keywords_match}([ >])/\\1_\\2\\3/g'" < ${_src} > ${_basename}.in
              COMMAND ${PERL_EXECUTABLE} ${INTLTOOL_MERGE_EXECUTABLE} -q -x ${_po_dir} ${_basename}.in ${_basename}${_binsuffix}
              DEPENDS ${_src}
              COMMENT "Merging translations into ${_sourcename}"
            )
          else()
            tde_message_fatal( "Unknown file format for merging translations." )
          endif()

        endif( )


      else( )

        # just copy the original file without translations
        add_custom_command(
          OUTPUT ${_basename}${_binsuffix}
          COMMAND ${CMAKE_COMMAND} -E copy ${_src} ${_basename}${_binsuffix}
          DEPENDS ${_src}
          COMMENT "Skiping translation and copying source file to ${_sourcename}"
        )

      endif( )

      # merge translations target
      add_custom_target( "${_target}" ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_basename}${_binsuffix} )

    endif( )

    # install traslated desktop file
    if( NOT "${_dest}" STREQUAL "-" )
      install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/${_basename}${_binsuffix}
        RENAME ${_out_name}
        DESTINATION ${_dest}
      )
    endif()

  endforeach()

endmacro( )


#################################################
#####
##### tde_conditional_add_project_docs
##### tde_add_project_docs
#####
##### Macro for standard processing and installation of documentation and man pages.
##### This is designed for ordinary modules - as an applications, not for core modules.

function( tde_conditional_add_project_docs _cond )

  if( ${_cond} )
    tde_add_project_docs()
  endif()

endfunction()

function( tde_add_project_docs )

  if( ${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${PROJECT_SOURCE_DIR} )
    set( DOCS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/doc )
  else()
    set( DOCS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
  endif()

  file( GLOB_RECURSE _doc_files RELATIVE ${DOCS_SOURCE_DIR} ${DOCS_SOURCE_DIR}/* )
  foreach( _doc_file IN LISTS _doc_files )
    get_filename_component( _dir ${_doc_file} PATH )
    list( APPEND _dirs ${_dir} )
  endforeach()
  if( _dirs )
    list( SORT _dirs )
    list( REMOVE_DUPLICATES _dirs )
  endif()

  string( REGEX REPLACE "[ \r\n\t]+" ";" _linguas "$ENV{LINGUAS}" )

  unset( _skip_subdir )
  foreach( _dir IN LISTS _dirs )
    string( REGEX REPLACE "/.*" "" _lang ${_dir} )
    if( NOT ${_lang} MATCHES "^(html|man|misc|other)$"
        AND ( NOT DEFINED _skip_subdir OR
              NOT ${_dir} MATCHES "^${_skip_subdir}/" )
        AND ( ${_lang} STREQUAL "en" OR
              "${_linguas}" MATCHES "^;*$" OR
              ";${_linguas};" MATCHES ";${_lang};" ))
      if( EXISTS ${DOCS_SOURCE_DIR}/${_dir}/CMakeLists.txt )
        set( _skip_subdir ${_dir} )
        add_subdirectory( ${DOCS_SOURCE_DIR}/${_dir} )
      else()
        unset( _skip_subdir )
        if( ${_dir} MATCHES "/[^/]*/" )
          string( REGEX REPLACE "^[^/]*/(.*)" "\\1" _doc_dest "${_dir}" )
        else()
          string( REGEX REPLACE "^[^/]*/(.*)" "\\1" _doc_dest "${_dir}/${PROJECT_NAME}" )
        endif()
        file( GLOB _doc_files RELATIVE ${DOCS_SOURCE_DIR}/${_dir} ${DOCS_SOURCE_DIR}/${_dir}/*.docbook )
        if( _doc_files )
          list( FIND _doc_files "index.docbook" _find_index )
          if( -1 EQUAL _find_index )
            set( _noindex "NOINDEX" )
          else()
            unset( _noindex )
          endif()
          tde_create_handbook(
            SOURCE_BASEDIR ${DOCS_SOURCE_DIR}/${_dir}
            ${_noindex}
            LANG ${_lang}
            DESTINATION ${_doc_dest}
          )
        else()
          file( GLOB _html_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${DOCS_SOURCE_DIR}/${_dir}/*.html )
          if( _html_files )
            file( GLOB _htmldoc_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
                  ${DOCS_SOURCE_DIR}/${_dir}/*.css
                  ${DOCS_SOURCE_DIR}/${_dir}/*.gif
                  ${DOCS_SOURCE_DIR}/${_dir}/*.jpg
                  ${DOCS_SOURCE_DIR}/${_dir}/*.png
            )
            install(
              FILES ${_html_files} ${_htmldoc_files}
              DESTINATION ${HTML_INSTALL_DIR}/${_lang}/${_doc_dest}
            )
          endif()
        endif()
      endif()
    endif()
  endforeach()

  if( EXISTS ${DOCS_SOURCE_DIR}/man AND
      NOT EXISTS ${DOCS_SOURCE_DIR}/man/CMakeLists.txt )
    file( GLOB_RECURSE _man_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${DOCS_SOURCE_DIR}/man/* )
    foreach( _man_file IN LISTS _man_files )
      if( ${_man_file} MATCHES "\\.[0-9]$" )
        string( REGEX REPLACE ".*\\.([0-9])$" "\\1" _man_section "${_man_file}" )
        list( APPEND _man_files_${_man_section} "${_man_file}" )
        list( APPEND _man_sections "${_man_section}" )
      endif()
    endforeach()
    foreach( _man_section IN LISTS _man_sections )
      INSTALL(
          FILES ${_man_files_${_man_section}}
          DESTINATION ${MAN_INSTALL_DIR}/man${_man_section}
          COMPONENT doc
      )
    endforeach()
  endif()

  if( EXISTS ${DOCS_SOURCE_DIR}/misc AND
      NOT EXISTS ${DOCS_SOURCE_DIR}/misc/CMakeLists.txt )
    install(
        DIRECTORY ${DOCS_SOURCE_DIR}/misc/
        DESTINATION ${SHARE_INSTALL_PREFIX}/doc/${PROJECT_NAME}
        COMPONENT doc
        PATTERN Makefile.am EXCLUDE
    )
  endif()

  foreach( _dir html man misc other )
    if( EXISTS ${DOCS_SOURCE_DIR}/${_dir}/CMakeLists.txt )
      add_subdirectory( ${DOCS_SOURCE_DIR}/${_dir} )
    endif()
  endforeach()

endfunction( )


#################################################
#####
##### tde_create_handbook

macro( tde_create_handbook )

  unset( _target )
  unset( _dest )
  unset( _noindex )
  unset( _srcs )
  unset( _extra )
  unset( _srcdir )

  get_filename_component( _source_basedir "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE )
  set( _lang en )
  set( _first_arg 1 )
  set( _var _target )

  foreach( _arg ${ARGN} )

    # found directive "SOURCE_BASEDIR"
    if( "+${_arg}" STREQUAL "+SOURCE_BASEDIR" )
      unset( _source_basedir )
      set( _var _source_basedir )
      set( _directive 1 )
    endif()

    # found directive "NOINDEX"
    if( "+${_arg}" STREQUAL "+NOINDEX" )
      set( _noindex 1 )
      set( _directive 1 )
    endif()

    # found directive "FILES"
    if( "+${_arg}" STREQUAL "+FILES" )
      unset( _srcs )
      set( _var _srcs )
      set( _directive 1 )
    endif()

    # found directive "EXTRA"
    if( "+${_arg}" STREQUAL "+EXTRA" )
      unset( _extra )
      set( _var _extra )
      set( _directive 1 )
    endif()

    # found directive "SRCDIR"
    if( "+${_arg}" STREQUAL "+SRCDIR" )
      unset( _srcdir )
      set( _var _srcdir )
      set( _directive 1 )
    endif()

    # found directive DESTINATION
    if( "+${_arg}" STREQUAL "+DESTINATION" )
      unset( _dest )
      set( _var _dest )
      set( _directive 1 )
    endif()

    # found directive "LANG"
    if( "+${_arg}" STREQUAL "+LANG" )
      unset( _lang )
      set( _var _lang )
      set( _directive 1 )
    endif()

    # collect data
    if( _directive )
      unset( _directive )
    elseif( _var )
      if( _first_arg )
        set( _target "${_arg}" )
      else()
        list( APPEND ${_var} ${_arg} )
      endif()
    endif()

    unset( _first_arg )

  endforeach()

  # if source_basedir is relative, complete the path to absolute
  if( NOT IS_ABSOLUTE ${_source_basedir} )
    get_filename_component( _source_basedir "${_source_basedir}" ABSOLUTE )
  endif()

  # prepare the binary directory according to source_basedir
  file( RELATIVE_PATH _binary_basedir "${CMAKE_CURRENT_SOURCE_DIR}" "${_source_basedir}" )
  set( _binary_basedir "${CMAKE_CURRENT_BINARY_DIR}/${_binary_basedir}" )
  file( MAKE_DIRECTORY "${_binary_basedir}" )

  # if no target specified, try to guess it from DESTINATION
  if( NOT _target )
    if( NOT _dest )
      tde_message_fatal( "target name cannot be determined because DESTINATION is not set" )
    endif()
    string( REPLACE "/" "-" _target "${_dest}" )
  endif()

  set( _target "${_target}-${_lang}-handbook" )

  # if sources are listed, complete the path to absolute
  if( _srcs )
    foreach( _src ${_srcs} )
      if( NOT IS_ABSOLUTE ${_src} )
        list( REMOVE_ITEM _srcs ${_src} )
        get_filename_component( _src "${_source_basedir}/${_src}" ABSOLUTE )
        list( APPEND _srcs ${_src} )
      endif()
    endforeach()
  endif()

  # if no file specified, include all docbooks, stylesheets and images
  if( NOT _srcs )
    file( GLOB _srcs
      ${_source_basedir}/*.docbook
      ${_source_basedir}/*.css
      ${_source_basedir}/*.gif
      ${_source_basedir}/*.jpg
      ${_source_basedir}/*.png
    )
  endif()

  # if no destination specified, defaulting to HTML_INSTALL_DIR
  if( NOT _dest )
    set( _dest "${HTML_INSTALL_DIR}/${_lang}" )
  # if destination is NOT absolute path,
  # we assume that is relative to HTML_INSTALL_DIR
  elseif( NOT IS_ABSOLUTE ${_dest} )
    set( _dest "${HTML_INSTALL_DIR}/${_lang}/${_dest}" )
  endif()

  if( NOT _srcs )
    tde_message_fatal( "no source files" )
  endif()

  if( NOT _noindex )

    # check for index.docbook
    list( FIND _srcs "${_source_basedir}/index.docbook" _find_index )
    if( -1 EQUAL _find_index )
      tde_message_fatal( "missing index.docbook file" )
    endif()

    # check for srcdir
    if( _srcdir )
      set( _srcdir "--srcdir=${_srcdir}" )
    endif()

    add_custom_command(
      OUTPUT ${_binary_basedir}/index.cache.bz2
      COMMAND ${KDE3_MEINPROC_EXECUTABLE} ${_srcdir} --check --cache index.cache.bz2 ${_source_basedir}/index.docbook
      COMMENT "Generating ${_target}"
      DEPENDS ${_srcs}
      WORKING_DIRECTORY "${_binary_basedir}"
    )

    string( REPLACE "@" "_" _target "${_target}" )
    add_custom_target( ${_target} ALL DEPENDS ${_binary_basedir}/index.cache.bz2 )

    list( APPEND _srcs ${_binary_basedir}/index.cache.bz2 )

    if( NOT TDE_HTML_DIR )
      set( TDE_HTML_DIR ${HTML_INSTALL_DIR} )
    endif( )

    tde_install_empty_directory( ${_dest} )
    file( RELATIVE_PATH _common_link_target "${_dest}" "${TDE_HTML_DIR}/${_lang}/common" )
    tde_install_symlink( ${_common_link_target} ${_dest} )

  endif()

  install( FILES ${_srcs} ${_extra} DESTINATION ${_dest} )

endmacro( )


#################################################
#####
##### tde_create_tarball
#####
##### Macro is used to create tarball.
#####

macro( tde_create_tarball )

  unset( _target )
  unset( _files )
  unset( _destination )
  set( _sourcedir "${CMAKE_CURRENT_SOURCE_DIR}" )
  set( _compression "gzip" )
  set( _var _target )

  foreach( _arg ${ARGN} )

    # found directive "TARGET"
    if( "+${_arg}" STREQUAL "+TARGET" )
      unset( _target )
      set( _var _target )
      set( _directive 1 )
    endif( )

    # found directive "SOURCEDIR"
    if( "+${_arg}" STREQUAL "+SOURCEDIR" )
      unset( _sourcedir )
      set( _var _sourcedir )
      set( _directive 1 )
    endif( )

    # found directive "FILES"
    if( "+${_arg}" STREQUAL "+FILES" )
      unset( _files )
      set( _var _files )
      set( _directive 1 )
    endif( )

    # found directive "DESTINATION"
    if( "+${_arg}" STREQUAL "+DESTINATION" )
      unset( _destination )
      set( _var _destination )
      set( _directive 1 )
    endif( )

    # found directive "COMPRESSION"
    if( "+${_arg}" STREQUAL "+COMPRESSION" )
      unset( _compression )
      set( _var _compression )
      set( _directive 1 )
    endif( )

    # collect data
    if( _directive )
      unset( _directive )
    elseif( _var )
      list( APPEND ${_var} ${_arg} )
    endif( )

  endforeach( )

  if( NOT _target )
    tde_message_fatal( "Target tarball name not specified." )
  endif( )

  if( NOT IS_ABSOLUTE ${_sourcedir} )
    set( _sourcedir "${CMAKE_CURRENT_SOURCE_DIR}/${_sourcedir}" )
  endif( )

  if( NOT _files )
    file( GLOB_RECURSE _files RELATIVE ${_sourcedir} "${_sourcedir}/*" )
    list( SORT _files )
  endif( )

  unset( _files_deps )
  foreach( _file ${_files} )
    list( APPEND _files_deps "${_sourcedir}/${_file}" )
  endforeach( )

  if( NOT DEFINED TAR_EXECUTABLE )
    find_program( TAR_EXECUTABLE NAMES tar )
    if( "${TAR_EXECUTABLE}" STREQUAL "TAR_EXECUTABLE-NOTFOUND" )
      tde_message_fatal( "tar executable is required but not found on your system" )
    endif( )
  endif( )

  if( NOT DEFINED TAR_SETOWNER )
    execute_process(
        COMMAND ${TAR_EXECUTABLE} --version
        OUTPUT_VARIABLE TAR_VERSION
    )
    string( REGEX REPLACE "^([^\n]*)\n.*" "\\1" TAR_VERSION "${TAR_VERSION}" )
    if( "${TAR_VERSION}" MATCHES "GNU *tar" )
      set( TAR_SETOWNER "--owner=root;--group=root" )
      set( TAR_REPRODUCIBLE "--pax-option=exthdr.name=%d/PaxHeaders/%f,delete=atime,delete=ctime" )
      list( APPEND TAR_REPRODUCIBLE "--mode=u+rw,go=rX,a-s" )
      tde_read_src_metadata()
      if( TDE_PKG_DATETIME )
        list( APPEND TAR_REPRODUCIBLE --mtime "${TDE_PKG_DATETIME} UTC" )
      elseif( TDE_SCM_MODULE_DATETIME )
        list( APPEND TAR_REPRODUCIBLE --mtime "${TDE_SCM_MODULE_DATETIME} UTC" )
      endif( )
    elseif( "${TAR_VERSION}" MATCHES "bsd *tar" )
      set( TAR_SETOWNER "--uname=root;--gname=root" )
    else( )
      set( TAR_SETOWNER "" )
    endif( )
  endif( )

  if( "${_compression}" STREQUAL "-" )
    unset( _compression )
  endif( )
  if( _compression )
    if( "${_compression}" STREQUAL "gzip" )
      set( TAR_COMPRESSION "|" ${_compression} "-n" )
    else( )
      set( TAR_COMPRESSION "|" ${_compression} )
    endif( )
  endif( )

  get_filename_component( _target_path "${CMAKE_CURRENT_BINARY_DIR}/${_target}" ABSOLUTE )
  file( RELATIVE_PATH _target_path "${CMAKE_BINARY_DIR}" "${_target_path}" )
  string( REPLACE "/" "+" _target_name "${_target_path}" )
  add_custom_target( "${_target_name}-tarball" ALL
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${_target}" )

  add_custom_command(
    COMMAND ${TAR_EXECUTABLE} cf -
        ${TAR_SETOWNER} ${TAR_REPRODUCIBLE} -- ${_files}
        ${TAR_COMPRESSION} > ${CMAKE_CURRENT_BINARY_DIR}/${_target}
    WORKING_DIRECTORY "${_sourcedir}"
    OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/${_target}"
    DEPENDS ${_files_deps}
    COMMENT "Create tarball ${_target_path}"
  )

  if( _destination )
    install( FILES ${CMAKE_CURRENT_BINARY_DIR}/${_target} DESTINATION ${_destination} )
  endif( )

endmacro()


#################################################
#####
##### tde_include_tqt

macro( tde_include_tqt )
  foreach( _cpp ${ARGN} )
    set_source_files_properties( ${_cpp} PROPERTIES COMPILE_FLAGS "-include tqt.h" )
  endforeach()
endmacro( )


#################################################
#####
##### tde_install_symlink

macro( tde_install_symlink _target _link )

  # if path is relative, we must to prefix it with CMAKE_INSTALL_PREFIX
  if( IS_ABSOLUTE "${_link}" )
    set( _destination "${_link}" )
  else( IS_ABSOLUTE "${_link}" )
    set( _destination "${CMAKE_INSTALL_PREFIX}/${_link}" )
  endif( IS_ABSOLUTE "${_link}" )

  get_filename_component( _path "${_destination}" PATH )
  if( NOT IS_DIRECTORY "\$ENV{DESTDIR}${_path}" )
    install( CODE "file( MAKE_DIRECTORY \"\$ENV{DESTDIR}${_path}\" )" )
  endif( NOT IS_DIRECTORY "\$ENV{DESTDIR}${_path}" )

  install( CODE "execute_process( COMMAND ln -s ${_target} \$ENV{DESTDIR}${_destination} )" )

endmacro( tde_install_symlink )


#################################################
#####
##### tde_install_empty_directory

macro( tde_install_empty_directory _path )

  # if path is relative, we must to prefix it with CMAKE_INSTALL_PREFIX
  if( IS_ABSOLUTE "${_path}" )
    set( _destination "${_path}" )
  else( IS_ABSOLUTE "${_path}" )
    set( _destination "${CMAKE_INSTALL_PREFIX}/${_path}" )
  endif( IS_ABSOLUTE "${_path}" )

  install( CODE "file( MAKE_DIRECTORY \"\$ENV{DESTDIR}${_destination}\" )" )

endmacro( tde_install_empty_directory )


#################################################
#####
##### tde_conditional_add_subdirectory

macro( tde_conditional_add_subdirectory _cond _path )

  if( ${_cond} )
    add_subdirectory( "${_path}" )
  endif( ${_cond} )

endmacro( tde_conditional_add_subdirectory )


#################################################
#####
##### tde_auto_add_subdirectories

macro( tde_auto_add_subdirectories )
  file( GLOB _dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} * )
  foreach( _dir ${_dirs} )
    if( IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${_dir} )
      if( NOT ${_dir} STREQUAL ".svn" AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_dir}/CMakeLists.txt )
        add_subdirectory( ${_dir} )
      endif()
    endif()
  endforeach()
endmacro()


#################################################
#####
##### tde_save / tde_restore

macro( tde_save )
  foreach( _var ${ARGN} )
    set( __bak_${_var} ${${_var}} )
  endforeach()
endmacro()

macro( tde_save_and_set _var )
  set( __bak_${_var} ${${_var}} )
  set( ${_var} ${ARGN} )
endmacro( )

macro( tde_restore )
  foreach( _var ${ARGN} )
    set( ${_var} ${__bak_${_var}} )
    unset( __bak_${_var} )
  endforeach()
endmacro()


#################################################
#####
##### tde_setup_install_path

macro( tde_setup_install_path _path _default )
  if( DEFINED ${_path} )
    set( ${_path} "${${_path}}" CACHE INTERNAL "" FORCE )
  else( )
    set( ${_path} "${_default}" )
  endif( )
endmacro( )


##################################################

if( ${CMAKE_SOURCE_DIR} MATCHES ${CMAKE_BINARY_DIR} )
    tde_message_fatal( "Please use out-of-source building, like this:
 \n   rm ${CMAKE_SOURCE_DIR}/CMakeCache.txt
   mkdir /tmp/${PROJECT_NAME}.build
   cd /tmp/${PROJECT_NAME}.build
   cmake ${CMAKE_SOURCE_DIR} [arguments...]" )
endif( )

#################################################
#####
##### tde_setup_architecture_flags

macro( tde_setup_architecture_flags )
  if( NOT DEFINED LINKER_IMMEDIATE_BINDING_FLAGS )
    message( STATUS "Detected ${CMAKE_SYSTEM_PROCESSOR} CPU architecture" )
    ## Immediate symbol binding is available only for gcc but not on ARM architectures
    if( ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND NOT ${CMAKE_SYSTEM_PROCESSOR} MATCHES arm* AND NOT ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD" )
      set( LINKER_IMMEDIATE_BINDING_FLAGS "-z\ now" CACHE INTERNAL "" FORCE )
    else( )
      set( LINKER_IMMEDIATE_BINDING_FLAGS "" CACHE INTERNAL "" FORCE )
    endif( )

    check_cxx_compiler_flag( -fPIE HAVE_PIE_SUPPORT )
    if( HAVE_PIE_SUPPORT )
      set( TDE_PIE_CFLAGS -fPIE )

      if( ${CMAKE_VERSION} VERSION_LESS "3.18" )
        execute_process(COMMAND "${CMAKE_LINKER}" --help
          OUTPUT_VARIABLE __linker_help
          ERROR_VARIABLE __linker_help)
        if( "${__linker_help}" MATCHES "-pie" )
          set( LINKER_PIE_SUPPORT 1 )
        elseif( "${__linker_help}" MATCHES "type=type.*pie" )
          set( LINKER_ZTYPE_PIE_SUPPORT 1 )
        endif( )
        unset(__linker_help)
      else( )
        check_linker_flag(CXX -pie LINKER_PIE_SUPPORT)
        if( NOT LINKER_PIE_SUPPORT )
          check_linker_flag(CXX -ztype=pie LINKER_ZTYPE_PIE_SUPPORT)
        endif()
      endif()
      if( LINKER_PIE_SUPPORT )
        set( TDE_PIE_LDFLAGS -pie )
      endif( LINKER_PIE_SUPPORT )
      if( LINKER_ZTYPE_PIE_SUPPORT )
        set( TDE_PIE_LDFLAGS -ztype=pie )
      endif( LINKER_ZTYPE_PIE_SUPPORT )
    endif( HAVE_PIE_SUPPORT )

    set( _reproducible_cxxflags
         "-fdebug-prefix-map=${CMAKE_SOURCE_DIR}=."
         "-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=."
    )
    foreach( _flag ${_reproducible_cxxflags} )
      string( REGEX REPLACE "=.*" "" _flag_name "${_flag}" )
      string( REGEX REPLACE "[^a-zA-Z0-9]+" "_" _flag_var "CXXFLAG_${_flag_name}" )
      if( NOT "${CMAKE_CXX_FLAGS}" MATCHES "(^| )${_flag_name}" )
        check_cxx_compiler_flag( "${_flag}" ${_flag_var} )
        if( ${_flag_var} )
          set( CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} ${_flag}" )
          set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_flag}" )
        endif()
      endif()
    endforeach()
  endif( )
endmacro( )


#################################################
#####
##### tde_setup_gcc_visibility

macro( tde_setup_gcc_visibility )
  if( NOT DEFINED __KDE_HAVE_GCC_VISIBILITY )
    if( NOT UNIX )
      tde_message_fatal( "gcc visibility support was requested, but your system is not *NIX" )
    endif( NOT UNIX )

    if( TQT_FOUND AND NOT DEFINED HAVE_TQT_VISIBILITY )
      find_library( TQT_LIBFILE tqt-mt HINTS "${TQT_LIBRARY_DIRS}" )
      if( NOT "${TQT_LIBFILE}" STREQUAL "TQT_LIBFILE-NOTFOUND" )
        message( STATUS "Performing Test HAVE_TQT_VISIBILITY" )
        execute_process(
          COMMAND readelf --syms "${TQT_LIBFILE}"
          OUTPUT_VARIABLE HAVE_TQT_VISIBILITY
        )
        if( "${HAVE_TQT_VISIBILITY}" STREQUAL "" OR
            "${HAVE_TQT_VISIBILITY}" MATCHES "GLOBAL[\t ]+DEFAULT[^\n]+QSettingsPrivate" )
          message( STATUS "Performing Test HAVE_TQT_VISIBILITY - Failed" )
          tde_message_fatal( "gcc visibility support was requested, but not supported in tqt library" )
        endif( )
        set( HAVE_TQT_VISIBILITY 1 CACHE INTERNAL "" )
        message( STATUS "Performing Test HAVE_TQT_VISIBILITY - Success" )
      endif( )
    endif( TQT_FOUND AND NOT DEFINED HAVE_TQT_VISIBILITY )

    if( TDE_FOUND AND NOT DEFINED HAVE_TDE_VISIBILITY )
      find_file( TDEMACROS_H kdemacros.h HINTS "${TDE_INCLUDE_DIR}" )
      if( NOT "${TDEMACROS_H}" STREQUAL "TDEMACROS_H-NOTFOUND" )
        tde_save_and_set( CMAKE_REQUIRED_INCLUDES "${TDE_INCLUDE_DIR}" )
        check_cxx_source_compiles( "
          #include <${TDEMACROS_H}>
            #ifndef __KDE_HAVE_GCC_VISIBILITY
            #error gcc visibility is not enabled in tdelibs
            #endif
            int main() { return 0; } "
          HAVE_TDE_VISIBILITY
        )
        tde_restore( CMAKE_REQUIRED_INCLUDES )
        if( NOT HAVE_TDE_VISIBILITY )
          tde_message_fatal( "gcc visibility support was requested, but not supported in tdelibs" )
        endif( NOT HAVE_TDE_VISIBILITY )
      endif( )
    endif( TDE_FOUND AND NOT DEFINED HAVE_TDE_VISIBILITY )

    set( __KDE_HAVE_GCC_VISIBILITY 1 CACHE INTERNAL "" )
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
  endif( )
endmacro( )


#################################################
#####
##### tde_setup_msgfmt

macro( tde_setup_msgfmt )
  if( NOT DEFINED MSGFMT_EXECUTABLE )
    include( FindGettext )
    if( GETTEXT_FOUND )
      set( MSGFMT_EXECUTABLE ${GETTEXT_MSGFMT_EXECUTABLE}
           CACHE FILEPATH "path to msgfmt executable" )
      if( GETTEXT_VERSION_STRING )
        set( MSGFMT_VERSION ${GETTEXT_VERSION_STRING}
             CACHE STRING "version of msgfmt executable" )
      endif( )
    endif( GETTEXT_FOUND )

    if( NOT MSGFMT_EXECUTABLE )
      tde_message_fatal( "msgfmt is required but was not found on your system." )
    endif( NOT MSGFMT_EXECUTABLE )
  endif( )

  if( NOT MSGFMT_VERSION )
    execute_process(
      COMMAND ${MSGFMT_EXECUTABLE} --version
      OUTPUT_VARIABLE _msgfmt_version
      ERROR_VARIABLE _msgfmt_version
    )
    string( REGEX REPLACE "^msgfmt[^\n]* ([^ ]*)\n.*" "\\1" _msgfmt_version ${_msgfmt_version} )
    set( MSGFMT_VERSION ${_msgfmt_version}
         CACHE STRING "version of msgfmt executable" )
  endif( )
endmacro( )


################################################
#####
##### tde_setup_largefiles

macro( tde_setup_largefiles )
  if( NOT DEFINED HAVE_LARGEFILES )
    message( STATUS "Check support for large files" )
    unset( LARGEFILES_DEFINITIONS )

    # check without special definitions
    unset( HAVE_SIZEOF_T CACHE )
    check_type_size( off_t SIZEOF_OFF_T )
    if( SIZEOF_OFF_T GREATER 7 )
      set( HAVE_LARGEFILES 1 )
    endif( )

    # check with definition _FILE_OFFSET_BITS=64
    if( NOT HAVE_LARGEFILES )
      unset( HAVE_SIZEOF_OFF_T CACHE )
      tde_save_and_set( CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS} -D_FILE_OFFSET_BITS=64" )
      check_type_size( off_t SIZEOF_OFF_T )
      tde_restore( CMAKE_REQUIRED_DEFINITIONS )
      if( SIZEOF_OFF_T GREATER 7 )
        set( LARGEFILES_DEFINITIONS "-D_FILE_OFFSET_BITS=64" )
        set( HAVE_LARGEFILES 1 )
      endif( )
    endif( )

    # check with definition _LARGE_FILES
    if( NOT HAVE_LARGEFILES )
      unset( HAVE_SIZEOF_OFF_T CACHE )
      tde_save_and_set( CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS} -D_LARGE_FILES" )
      check_type_size( off_t SIZEOF_OFF_T )
      tde_restore( CMAKE_REQUIRED_DEFINITIONS )
      if( SIZEOF_OFF_T GREATER 7 )
        set( LARGEFILES_DEFINITIONS "-D_LARGE_FILES" )
        set( HAVE_LARGEFILES 1 )
      endif( )
    endif( )

    # check with definition _LARGEFILE_SOURCE
    if( NOT HAVE_LARGEFILES )
      unset( HAVE_SIZEOF_OFF_T CACHE )
      tde_save_and_set( CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS} -D_LARGEFILE_SOURCE" )
      check_type_size( off_t SIZEOF_OFF_T )
      tde_restore( CMAKE_REQUIRED_DEFINITIONS )
      if( SIZEOF_OFF_T GREATER 7 )
        set( LARGEFILES_DEFINITIONS "-D_LARGEFILE_SOURCE" )
        set( HAVE_LARGEFILES 1 )
      endif( )
    endif( )

    # check for fseeko/ftello
    if( HAVE_LARGEFILES )
      tde_save_and_set( CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS} ${LARGEFILES_DEFINITIONS}" )
      check_symbol_exists( "fseeko" "stdio.h" HAVE_FSEEKO )
      tde_restore( CMAKE_REQUIRED_DEFINITIONS )
      if( NOT HAVE_FSEEKO )
        tde_save_and_set( CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS} ${LARGEFILES_DEFINITIONS} -D_LARGEFILE_SOURCE" )
        check_symbol_exists( "fseeko" "stdio.h" HAVE_FSEEKO )
        tde_restore( CMAKE_REQUIRED_DEFINITIONS )
        if( HAVE_FSEEKO )
          set( LARGEFILES_DEFINITIONS "${LARGEFILES_DEFINITIONS} -D_LARGEFILE_SOURCE" )
        else( )
          unset( HAVE_LARGEFILES )
        endif( )
      endif( )
    endif( )

    # check results
    if( HAVE_LARGEFILES )
      if( "${LARGEFILES_DEFINITIONS}" STREQUAL "" )
        message( STATUS "Check support for large files - Success" )
      else( )
        add_definitions( ${LARGEFILES_DEFINITIONS} )
        message( STATUS "Check support for large files - Success with ${LARGEFILES_DEFINITIONS}" )
      endif( )
      set( HAVE_LARGEFILES 1 CACHE INTERNAL "Support for large files enabled" )
    else( )
      message( STATUS "Check support for large files - Failed" )
      tde_message_fatal( "Cannot find a way to enable support for large files." )
    endif( )
  endif( )
endmacro( )


#################################################
#####
##### tde_setup_dbus

macro( tde_setup_dbus )
  if( NOT DBUS_FOUND )
    pkg_search_module( DBUS dbus-1 )
    if( NOT DBUS_FOUND )
      tde_message_fatal( "dbus-1 is required, but not found on your system" )
    endif( )
  endif( )

  if( NOT DEFINED DBUS_SYSTEM_CONF_DIRECTORY )
    execute_process(
      COMMAND ${PKG_CONFIG_EXECUTABLE}
        dbus-1 --variable=sysconfdir
      OUTPUT_VARIABLE DBUS_SYSTEM_CONF_BASE
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if( DBUS_SYSTEM_CONF_BASE )
      set( DBUS_SYSTEM_CONF_DIRECTORY "${DBUS_SYSTEM_CONF_BASE}/dbus-1/system.d"
           CACHE PATH "Path for DBUS system configuration files" )
      message( STATUS "Using " ${DBUS_SYSTEM_CONF_DIRECTORY} " for DBUS system configuration files" )
    else( )
      tde_message_fatal( "Can not find the base directory for the dbus-1 configuration" )
    endif( )
  endif( )

  if( NOT DEFINED DBUS_SESSION_CONF_DIRECTORY )
    execute_process(
      COMMAND ${PKG_CONFIG_EXECUTABLE}
        dbus-1 --variable=sysconfdir
      OUTPUT_VARIABLE DBUS_SYSTEM_CONF_BASE
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if( DBUS_SYSTEM_CONF_BASE )
      set( DBUS_SESSION_CONF_DIRECTORY "${DBUS_SYSTEM_CONF_BASE}/dbus-1/session.d"
           CACHE PATH "Path for DBUS session configuration files" )
      message( STATUS "Using " ${DBUS_SESSION_CONF_DIRECTORY} " for DBUS session configuration files" )
    else( )
      tde_message_fatal( "Can not find the base directory for the dbus-1 configuration" )
    endif( )
  endif( )

  if( NOT DEFINED DBUS_SESSION_DIRECTORY )
    execute_process(
      COMMAND ${PKG_CONFIG_EXECUTABLE}
        dbus-1 --variable=session_bus_services_dir
      OUTPUT_VARIABLE DBUS_SESSION_DIRECTORY
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set( DBUS_SESSION_DIRECTORY "${DBUS_SESSION_DIRECTORY}"
         CACHE PATH "Path for DBUS session service files" )
    message( STATUS "Using " ${DBUS_SESSION_DIRECTORY} " for DBUS session service files" )
  endif( )

  if( NOT DEFINED DBUS_SERVICE_DIRECTORY )
    execute_process(
      COMMAND ${PKG_CONFIG_EXECUTABLE}
        dbus-1 --variable=system_bus_services_dir
      OUTPUT_VARIABLE DBUS_SERVICE_DIRECTORY
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if( "${DBUS_SERVICE_DIRECTORY}" STREQUAL "" )
      if( "${DBUS_SESSION_DIRECTORY}" MATCHES "/services$" )
        string( REGEX REPLACE "/services$" "/system-services"
                DBUS_SERVICE_DIRECTORY "${DBUS_SESSION_DIRECTORY}" )
      else( )
        tde_message_fatal( "Directory for DBUS system service files can not be determined." )
      endif( )
    endif( )
    set( DBUS_SERVICE_DIRECTORY "${DBUS_SERVICE_DIRECTORY}"
         CACHE PATH "Path for DBUS system service files" )
    message( STATUS "Using " ${DBUS_SERVICE_DIRECTORY} " for DBUS system service files" )
  endif( )

  if( NOT "${ARGV}" STREQUAL "" AND NOT DBUS_TQT_FOUND )
    pkg_search_module( DBUS_TQT ${ARGV} )
    if( NOT DBUS_TQT_FOUND )
      tde_message_fatal( "${ARGV} is required, but not found on your system" )
    endif( )
  endif( )
  if( "${ARGV}" STREQUAL "dbus-1-tqt" AND NOT DEFINED DBUSXML2QT3_EXECUTABLE )
    find_program( DBUSXML2QT3_EXECUTABLE
      NAMES dbusxml2qt3
      HINTS "${TDE_PREFIX}/bin" ${BIN_INSTALL_DIR}
    )
  endif( )

endmacro( )


#################################################
#####
##### tde_setup_polkit

macro( tde_setup_polkit )
  if( NOT POLKIT_GOBJECT_FOUND )
    pkg_search_module( POLKIT_GOBJECT polkit-gobject-1 )
    if( NOT POLKIT_GOBJECT_FOUND )
      tde_message_fatal( "polkit-gobject-1 is required, but not found on your system" )
    endif( )
  endif( )

  foreach( _arg ${ARGV} )
    if( NOT "${_arg}" MATCHES "^polkit-" )
      set( _arg "polkit-${_arg}" )
    endif( )
    string( TOUPPER "${_arg}" _polkit_module )
    if( "${_polkit_module}" MATCHES "-[0-9]+$" )
      string( REGEX REPLACE "-[0-9]+$" "" _polkit_module "${_polkit_module}" )
    endif( )
    string( REPLACE "-" "_" _polkit_module "${_polkit_module}" )
    if( NOT "${_arg}" MATCHES "-[0-9]+$" )
      set( _arg "${_arg}-1" )
    endif( )
    if( NOT ${_polkit_module}_FOUND )
      pkg_search_module( ${_polkit_module} ${_arg} )
      if( NOT ${_polkit_module}_FOUND )
        tde_message_fatal( "${_arg} is required, but not found on your system" )
      endif( )
    endif( )
  endforeach( )

  if( NOT DEFINED POLKIT_ACTIONS_DIRECTORY )
    execute_process(
      COMMAND ${PKG_CONFIG_EXECUTABLE}
        polkit-gobject-1 --variable=actiondir
      OUTPUT_VARIABLE POLKIT_ACTIONS_DIRECTORY
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set( POLKIT_ACTIONS_DIRECTORY "${POLKIT_ACTIONS_DIRECTORY}"
         CACHE PATH "Path for PolicyKit action files" )
    message( STATUS "Using " ${POLKIT_ACTIONS_DIRECTORY} " for PolicyKit action files" )
  endif( )

endmacro( )


#################################################
#####
##### restore CMake policies

cmake_policy( POP )


#################################################
