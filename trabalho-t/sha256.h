// sha256.h
#ifndef sha256_hpp
#define sha256_hpp

#include <stdio.h>
#include <string>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <iomanip>
#include <sstream>

std::string printSha256(const char *path);

#endif /* sha256_hpp */
