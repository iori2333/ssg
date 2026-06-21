///
/// Debug output functions
///
#ifndef PBGWIN_DX_ERROR_H
#define PBGWIN_DX_ERROR_H "DX_ERROR : Version 0.01 : Update 1999/11/20"

#include <string_view>

// Revision history

// 1999/12/10 : Added file error output function
// 1999/11/20 : Started

// Function prototypes
extern void DebugSetup(void);   // Prepare error output (->LogFile)
extern void DebugCleanup(void); // Close the error output file
extern void DebugLog(std::string_view s);
extern void DebugOut(std::string_view s); // Output debug message

#endif
