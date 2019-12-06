#! /usr/local/env python

# Example invocation: python3 ./waf distclean configure --prefix=~/Desktop/compiletools build install

import os

APPNAME = "afterburner-compiletools"

# We need to support "linux", "win32" and "darwin"
# TODO: Probably change these to compilers instead of platforms.
PLATFORM_CONFIG = \
{
	"linux":
	{
		"DEFINES":
		[
			"STDC_HEADERS",
			"SYSTEM_POSIX",
			"HAVE_SYS_TIME_H",
			"HAVE_UNISTD_H",
			"HAVE_SYS_STAT_H",
			"HAVE_FCNTL_H",
			"HAVE_SYS_RESOURCE_H",
			"_strdup=strdup",
			"_strlwr=strlwr",
			"_strupr=strupr",
			"stricmp=strcasecmp",
			"_unlink=unlink",
			"_open=open",
			"_read=read",
			"_close=close",
			"VERSION_LINUX"
		],

		"DEFINES_64":
		[
			"VERSION_64BIT"
		],

		"LINKFLAGS":
		[
			"-pthread"
		],

		"CFLAGS":
		[
			"-Werror",
			"-Wint-to-pointer-cast",
			"-Ofast",
			"-funsafe-math-optimizations",
			"-funsafe-loop-optimizations",
			"-ffast-math",
			"-fgraphite-identity",
			"-march=native",
			"-mtune=native",
			"-msse4",
			"-mavx",
			"-floop-interchange",
			"-mfpmath=sse"
		],

		"CXXFLAGS":
		[
			"-std=c++11",
			"-Werror"
		]
	},

	"darwin":
	{
		"DEFINES":
		[
			"STDC_HEADERS",
			"SYSTEM_POSIX",
			"HAVE_SYS_TIME_H",
			"HAVE_UNISTD_H",
			"HAVE_SYS_STAT_H",
			"HAVE_FCNTL_H",
			"HAVE_SYS_RESOURCE_H",
			"_strdup=strdup",
			"_strlwr=strlwr",
			"_strupr=strupr",
			"stricmp=strcasecmp",
			"_unlink=unlink",
			"_open=open",
			"_read=read",
			"_close=close",
			"VERSION_LINUX"
		],

		"DEFINES_64":
		[
			"VERSION_64BIT"
		],

		"LINKFLAGS":
		[
			"-pthread"
		],

		"CFLAGS":
		[
			"-Wint-to-pointer-cast",
			"-Ofast",
			"-funsafe-math-optimizations",
			"-funsafe-loop-optimizations",
			"-ffast-math",
			"-fgraphite-identity",
			"-march=native",
			"-mtune=native",
			"-msse4",
			"-mavx",
			"-floop-interchange",
			"-mfpmath=sse"

			# These are annoying and (for this codebase) permissible:
			"-Wno-deprecated-declarations",
			"-Wno-deprecated-register"
		],

		"CXXFLAGS":
		[
			"-std=c++11",

			# These are annoying and (for this codebase) permissible:
			"-Wno-deprecated-declarations",
			"-Wno-deprecated-register"
		]
	},

	"win32":
	{
		"DEFINES":
		[
			"WIN32",
			"_CONSOLE",
			"SYSTEM_WIN32",
			"STDC_HEADERS"
		],

		"DEFINES_32":
		[
			"VERSION_32BIT"
		],

		"DEFINES_64":
		[
			"VERSION_64BIT"
		],

		"LINKFLAGS":
		[
		],

		"LINKFLAGS_64":
		[
			"/MACHINE:X64"
		],

		"CFLAGS":
		[
			"/EHsc"
		],

		"CXXFLAGS":
		[
			"/EHsc"
		]
	}
}

# NOTE: DEFINES_32 and DEFINES_64 are not supported in these extra dicts yet!
DEBUG_SWITCHES = \
{
	"darwin":
	{
		"CFLAGS":
		[
			"-g"
		],

		"CXXFLAGS":
		[
			"-g"
		]
	},

	"linux":
	{
		"CFLAGS":
		[
			"-g"
		],

		"CXXFLAGS":
		[
			"-g"
		]
	},

	"win32":
	{
		"DEFINES":
		[
			"_DEBUG"
		],

		"CFLAGS":
		[
			"/MDd",
			"/ZI",
			"/FS"
		],

		"CXXFLAGS":
		[
			"/MDd",
			"/ZI",
			"/FS"
		],

		"LINKFLAGS":
		[
			"/DEBUG:FASTLINK"
		]
	}
}

RELEASE_SWITCHES = \
{
	"win32":
	{
		"DEFINES":
		[
			"NDEBUG"
		],

		"CFLAGS":
		[
			"/MD"
		],

		"CXXFLAGS":
		[
			"/MD"
		]
	}
}

SUBDIRS = \
[
	"hlcsg",
	"hlbsp",
	"hlvis",
	"hlrad"
]

def __platformConfigAppendUnique(ctx, key):
	commonDict = PLATFORM_CONFIG
	buildTypeDict = DEBUG_SWITCHES if ctx.env.BUILD_TYPE == "debug" else RELEASE_SWITCHES
	archSuffix = "_32" if ctx.env.IS_32BIT else "_64"
	destOS = ctx.env.DEST_OS

	itemsToAppend = []

	for config in (commonDict, buildTypeDict):
		if destOS not in config:
			continue

		osOptions = config[destOS]

		for indexKey in (key, (key + archSuffix)):
			if indexKey not in osOptions:
				continue

			itemsToAppend += osOptions[indexKey]

	ctx.msg(key, ", ".join(itemsToAppend))
	ctx.env.append_unique(key, itemsToAppend)

def __setPlatformConfig(ctx):
	destOS = ctx.env.DEST_OS

	if isinstance(destOS, str) and destOS in PLATFORM_CONFIG:
		__platformConfigAppendUnique(ctx, "DEFINES")
		__platformConfigAppendUnique(ctx, "LINKFLAGS")
		__platformConfigAppendUnique(ctx, "CFLAGS")
		__platformConfigAppendUnique(ctx, "CXXFLAGS")
	else:
		ctx.fatal(f"Platform '{destOS if isinstance(destOS, str) else '<unknown>'}' is not currently supported by this build script.")

def options(ctx):
	ctx.load("compiler_cxx")

	ctx.add_option('--build-type',
				   action='store',
				   type='string',
				   dest='BUILD_TYPE',
				   default = "release",
				   help = 'Build type: release (default) or debug.')

	ctx.recurse(SUBDIRS)

def configure(ctx):
	ctx.load("compiler_cxx")
	ctx.env.IS_32BIT = ctx.env.DEST_CPU == "x86_64"
	ctx.env.BUILD_TYPE = ctx.options.BUILD_TYPE

	__setPlatformConfig(ctx)

	# We steal this method from the Xash3D repo.
	for dirName in SUBDIRS:
		ctx.setenv(dirName, ctx.env) # derive new env from global one
		ctx.env.ENVNAME = dirName

		# configure in standalone env
		ctx.recurse(dirName)
		ctx.setenv("")

def build(bld):
	bld.recurse(SUBDIRS)
