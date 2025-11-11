#ifndef VERSION_H
#define VERSION_H

// Build timestamp
#define FW_BUILD_DATE __DATE__
#define FW_BUILD_TIME __TIME__

// Git version (injected by build system via -DFW_GIT_VERSION="...")
// Falls back to "unknown" if not provided
#ifndef FW_GIT_VERSION
  #define FW_GIT_VERSION "unknown"
#endif

#endif // VERSION_H

