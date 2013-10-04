/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef PKGMAN_H
#define PKGMAN_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <String.h>


extern const char* kProgramName;

#define DIE(result, msg...)											\
do {																\
	fprintf(stderr, "*** " msg);									\
	fprintf(stderr, " : %s\n", strerror(result));					\
	exit(5);														\
} while(0)

#define ERROR(result, msg...)										\
do {																\
	fprintf(stderr, "*** " msg);									\
	fprintf(stderr, " : %s\n", strerror(result));					\
} while(0)

#define WARN(result, msg...)										\
do {																\
	fprintf(stderr, "* " msg);										\
	fprintf(stderr, " : %s\n", strerror(result));					\
} while(0)


void	print_usage_and_exit(bool error);


extern const BString kCommandCategoryPackages;
extern const BString kCommandCategoryRepositories;
extern const BString kCommandCategoryOther;


#endif	// PKGMAN_H
