#! /usr/local/env python

# Example invocation: python3 ./waf distclean configure --prefix=~/Desktop/compiletools build install

import os

APPNAME = "afterburner-compiletools"

# We need to support "linux", "win32" and "darwin"
PLATFORM_CONFIG = \
{
	"common":
	{
		"DEFINES":
		[
			"STDC_HEADERS"
		]
	},

	"linux":
	{
		"DEFINES":
		[
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
			"-mfpmath=sse -g"
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

def __platformConfigAppendUnique(ctx, target, configKey=None, destOS=None):
	if configKey is None:
		configKey = target

	if destOS is None:
		destOS = ctx.env.DEST_OS

	config = PLATFORM_CONFIG[destOS]

	if configKey in config:
		ctx.env.append_unique(target, config[configKey])

def __setPlatformConfig(ctx):
	destOS = ctx.env.DEST_OS

	__platformConfigAppendUnique(ctx, "DEFINES", destOS="common")

	if isinstance(destOS, str) and destOS in PLATFORM_CONFIG:
		__platformConfigAppendUnique(ctx, "DEFINES")
		__platformConfigAppendUnique(ctx, "LINKFLAGS")
		__platformConfigAppendUnique(ctx, "CFLAGS")

		if not ctx.env.IS_32BIT:
			__platformConfigAppendUnique(ctx, "DEFINES", configKey="DEFINES_64")
	else:
		ctx.fatal(f"Platform '{destOS if isinstance(destOS, str) else '<unknown>'}' is not currently supported by this build script.")

def options(ctx):
	ctx.load("compiler_cxx")

	ctx.recurse(SUBDIRS)

def configure(ctx):
	ctx.load("compiler_cxx")
	ctx.env.IS_32BIT = ctx.env.DEST_CPU == "x86_64"

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
