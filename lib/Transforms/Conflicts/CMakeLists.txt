#if(WIN32 OR CYGWIN)
#  set(LLVM_LINK_COMPONENTS Core Support)
#endif()

add_llvm_library( LLVMConflicts
  Conflicts.cpp

  #ADDITIONAL_HEADER_DIRS
  #${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms

  DEPENDS
  intrinsics_gen
#  PLUGIN_TOOL
#  opt
  )
