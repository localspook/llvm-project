target_sources(clangTidy
  PRIVATE
  CalleeNamespaceCheck.cpp
  ImplementationInNamespaceCheck.cpp
  InlineFunctionDeclCheck.cpp
  LLVMLibcTidyModule.cpp
  RestrictSystemLibcHeadersCheck.cpp
  )
