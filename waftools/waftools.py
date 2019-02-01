import os

def get_subproject_name(ctx):
	return os.path.basename(os.path.realpath(str(ctx.path)))
