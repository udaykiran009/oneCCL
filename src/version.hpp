#ifndef VERSION_HPP
#define VERSION_HPP

#define STR_HELPER(x) #x
#define STR(x)        STR_HELPER(x)

#define MLSL_PACKAGE_VERSION_PREFIX  "Intel(R) Machine Learning Scaling Library for Linux* OS, Version "
#define MLSL_PACKAGE_VERSION_POSTFIX "Copyright (c) " STR(MLSL_YEAR) ", Intel Corporation. All rights reserved."
#define MLSL_PACKAGE_VERSION         MLSL_PACKAGE_VERSION_PREFIX       \
                                     STR(MLSL_OFFICIAL_VERSION)        \
                                     " (" STR(MLSL_FULL_VERSION) ")\n" \
                                     MLSL_PACKAGE_VERSION_POSTFIX

#endif /* VERSION_HPP */
